//======================================================================
// genetico.cpp
//----------------------------------------------------------------------
// Implementación de NSGA-II y utilidades para generar/optimizar
// expresiones sobre conjuntos.
//======================================================================

#include "genetico.hpp"
#include "metrics.hpp"

#include<iomanip>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <random>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

using namespace std;

//------------------------------------------------------------------
// Árbol binario aleatorio (≤ k operaciones)
//------------------------------------------------------------------
Expression build_random_expr(const vector<int>& conjs, const vector<Bitset>& F, const Bitset& U, int k, mt19937& rng){
    struct Node{ Expression e; };
    vector<Node> pool; 
    pool.reserve(conjs.size());

    for (int idx : conjs) {
        if (idx == -1){
            set<int> u_set = {};
            pool.push_back({Expression(U, "U", u_set, 0)});
        }
        else if (idx>=0){
            set<int> f_set = {idx};
            pool.push_back({Expression(F[idx], "F"+to_string(idx),f_set,0)});
        }
    }

    if (pool.empty()) return Expression(Bitset(), "∅", {}, 0);
    if (pool.size()==1) return pool.front().e;

    int intentos_fallidos = 0;
    const int max_intentos = 100;

    while (pool.size() > 1 && intentos_fallidos < max_intentos) {
        uniform_int_distribution<> pick(0, (int)pool.size()-1);
        int a = pick(rng), b = pick(rng); 

        if (a != b) {
            if (a>b) swap(a,b);

            int op = uniform_int_distribution<>(0,2)(rng); 
            const char* op_str = (op==0) ?" ∪ ":(op==1)?" ∩ ":" \\ ";

            Bitset H;
            if (op==0) H = set_union (pool[a].e.conjunto, pool[b].e.conjunto);
            if (op==1) H = set_intersect(pool[a].e.conjunto, pool[b].e.conjunto);
            if (op==2) H = set_difference(pool[a].e.conjunto, pool[b].e.conjunto);

            int ops_new = pool[a].e.n_ops + pool[b].e.n_ops + 1;
            
            if (ops_new <= k) {
                // Esto solo se ejecuta si AMBAS condiciones son buenas
                set<int> usados = pool[a].e.used_sets;
                usados.insert(pool[b].e.used_sets.begin(), pool[b].e.used_sets.end());
                string expr_str = "(" + pool[a].e.expr_str + op_str + pool[b].e.expr_str + ")";

                pool[a].e = Expression(H, expr_str, usados, ops_new);
                pool.erase(pool.begin()+b);
                intentos_fallidos = 0; // Éxito
            } else {
                // (Falló la condición ops_new > k)
                intentos_fallidos++;
            }
        } else {
            // (Falló la condición a == b)
            intentos_fallidos++;
        }
    }
    return pool.front().e;
}

// ============================================================================
// NSGA-II Principal
// ============================================================================
vector<Individuo> nsga2(
    const vector<Bitset>& F,
    const Bitset& U,
    const Bitset& G,
    int k,
    const GAParams& params)
{
    // Semilla: si es 0, usar tiempo actual (para que sea no determinista)
    uint64_t seed = params.seed 
        ? params.seed 
        : (uint64_t)chrono::high_resolution_clock::now().time_since_epoch().count();
    mt19937 rng(seed);

    auto start_time = chrono::steady_clock::now();
    auto time_limit = chrono::seconds(params.time_limit_sec);

    vector<SolMO> bloques_base;
    for (size_t i = 0; i < F.size(); i++) {
        set<int> s = {(int)i}; 
        Expression e(F[i], "F"+to_string(i), s, 0);
        bloques_base.emplace_back(e, 0, M(e,G,Metric::SizeH), M(e,G,Metric::Jaccard));
    }
    set<int> s_u = {-1}; 
    Expression e_u(U, "U", s_u, 0);
    bloques_base.emplace_back(e_u, 0, M(e_u,G,Metric::SizeH), M(e_u,G,Metric::Jaccard));

    // 1) Población inicial
    vector<Individuo> poblacion = inicializar_poblacion(F, U, G, k, params.population_size, rng);
    
    /*Individuo mejor_global = poblacion[0];
    for (const auto& ind : poblacion) {
        if (ind.jaccard > mejor_global.jaccard) mejor_global = ind;
    }*/

    int generation = 0;
    while (generation < params.max_generations && 
           chrono::steady_clock::now() - start_time < time_limit) {
        
        // 2) Offspring con anti-duplicados por estructura
        vector<Individuo> offspring;
        offspring.reserve(params.population_size);
        
        unordered_set<string> seen_gen;
        seen_gen.reserve(params.population_size * 2);
        for (const auto& ind : poblacion) seen_gen.insert(ind.expr.expr_str);

        while ((int)offspring.size() < params.population_size) {
            Individuo p1 = torneo_seleccion(poblacion, params.tournament_size, rng);
            Individuo p2 = torneo_seleccion(poblacion, params.tournament_size, rng);

            Individuo hijo;
            if (uniform_real_distribution<>(0,1)(rng) < params.crossover_prob) {
                hijo = crossover(p1, p2, F, U, G, k, rng);
            } else {
                hijo = (uniform_int_distribution<>(0,1)(rng) == 0) ? p1 : p2;
            }

            if (uniform_real_distribution<>(0,1)(rng) < params.mutation_prob) {
                mutar(hijo, F, U, G, k, rng, bloques_base);
            }
            
            if (seen_gen.insert(hijo.expr.expr_str).second) {
                offspring.push_back(move(hijo));
            }
        }

        // 3) Combinación
        vector<Individuo> R = poblacion;
        R.insert(R.end(), offspring.begin(), offspring.end());

        // 4) NSGA-II: frentes + crowding
        auto Flist = fast_non_dominated_sort(R);

        // 5) Nueva población
        vector<Individuo> Pnext;
        Pnext.reserve(params.population_size);
        
        // Usamos un bucle 'for' normal con una condición de llenado
        for (size_t i = 0; i < Flist.size(); ++i) {
            auto& Fi = Flist[i];

            // Solo entramos al bloque de trabajo si aún hay capacidad
            if (Pnext.size() < (size_t)params.population_size) {
                
                // No necesitamos 'if (Fi.empty()) continue;'
                if (Fi.empty()) {
                    continue; // Pasa a la siguiente iteración del for
                }

                calcular_crowding_distance(Fi);
                
                size_t remaining_capacity = params.population_size - Pnext.size();
                
                // Caso A: El frente Fi cabe entero
                if (Fi.size() <= remaining_capacity) {
                    Pnext.insert(Pnext.end(), Fi.begin(), Fi.end());
                } 
                // Caso B: El frente Fi no cabe entero, debemos cortarlo
                else {
                    // Ordenar por crowding distance descendente
                    sort(Fi.begin(), Fi.end(),
                        [](const Individuo& a, const Individuo& b) {
                            if (isinf(a.crowd) && !isinf(b.crowd)) return true;
                            if (!isinf(a.crowd) && isinf(b.crowd)) return false;
                            return a.crowd > b.crowd;
                        });
                    
                    // Insertamos solo el número necesario. Esto LLENA Pnext.
                    Pnext.insert(Pnext.end(), Fi.begin(), Fi.begin() + remaining_capacity);
                }
            }
        }
        
        /*for (const auto& ind : Pnext) {
            if (ind.jaccard > mejor_global.jaccard) mejor_global = ind;
        }*/
        
        poblacion = move(Pnext);
        generation++;
    }

    return pareto_front(poblacion);
}

// ============================================================================
// Fast Non-Dominated Sort (NSGA-II)
// ============================================================================
vector<vector<Individuo>> fast_non_dominated_sort(vector<Individuo>& poblacion) {
    int n = (int)poblacion.size();
    vector<int> domination_count(n, 0);
    vector<vector<int>> dominated_set(n); // Almacena ÍNDICES
    vector<vector<int>> frentes_idx;      // Almacena ÍNDICES

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (dominates(poblacion[i], poblacion[j])) {
                dominated_set[i].push_back(j); // i domina al índice j
                domination_count[j]++;
            } else if (dominates(poblacion[j], poblacion[i])) {
                dominated_set[j].push_back(i); // j domina al índice i
                domination_count[i]++;
            }
        }
    }

    vector<int> frente0_idx;
    for (int i = 0; i < n; i++) {
        if (domination_count[i] == 0) {
            poblacion[i].rank = 0;
            frente0_idx.push_back(i); // Añade el ÍNDICE i
        }
    }
    if (!frente0_idx.empty()) frentes_idx.push_back(frente0_idx);

    int rank = 0;
    while (rank < (int)frentes_idx.size() && !frentes_idx[rank].empty()) {
        vector<int> siguiente_idx; // Siguiente frente de ÍNDICES
        
        for (int idx : frentes_idx[rank]) { // Itera sobre los índices del frente actual
            
            for (int j_idx : dominated_set[idx]) { // Itera sobre los índices que 'idx' domina
                
                domination_count[j_idx]--;
                if (domination_count[j_idx] == 0) {
                    poblacion[j_idx].rank = rank + 1;
                    siguiente_idx.push_back(j_idx);
                }
            }
        }
        
        // solo si contiene elementos.
        if (!siguiente_idx.empty()) {
             frentes_idx.push_back(siguiente_idx);
        }
        rank++; // Incrementamos el rango siempre para avanzar al siguiente frente.
    }

    // --- CONVERSIÓN FINAL ---
    // Ahora convertimos los frentes de índices a frentes de Individuos
    vector<vector<Individuo>> frentes;
    frentes.reserve(frentes_idx.size());
    for (const auto& frente_i : frentes_idx) {
        vector<Individuo> frente_ind;
        frente_ind.reserve(frente_i.size());
        for (int idx : frente_i) {
            frente_ind.push_back(poblacion[idx]);
        }
        frentes.push_back(frente_ind);
    }
    
    return frentes;
}

// ============================================================================
// Crowding Distance
// ============================================================================
void calcular_crowding_distance(vector<Individuo>& frente) {
    int n = (int)frente.size();
    if (n == 0) return;

    if (n == 1) {
        frente[0].crowd = INFINITY;
        return;
    }

    for (auto& ind : frente) ind.crowd = 0.0;

    // Jaccard (maximizar)
    sort(frente.begin(), frente.end(), [](const Individuo& a, const Individuo& b) {
        return a.jaccard > b.jaccard;
    });
    frente[0].crowd = INFINITY;
    frente[n-1].crowd = INFINITY;
    double r0 = max(1e-12, frente[0].jaccard - frente[n-1].jaccard);
    for (int i = 1; i < n - 1; i++) {
        frente[i].crowd += (frente[i - 1].jaccard - frente[i + 1].jaccard) / r0;
    }

    // SizeH (minimizar)
    sort(frente.begin(), frente.end(), [](const Individuo& a, const Individuo& b) {
        return a.sizeH < b.sizeH;
    });
    frente[0].crowd = INFINITY;
    frente[n-1].crowd = INFINITY;
    double r1 = max(1e-12, (double)(frente[n-1].sizeH - frente[0].sizeH));
    for (int i = 1; i < n - 1; i++) {
        frente[i].crowd += (double)(frente[i + 1].sizeH - frente[i - 1].sizeH) / r1;
    }

    // n_ops (minimizar)
    sort(frente.begin(), frente.end(), [](const Individuo& a, const Individuo& b) {
        return a.n_ops < b.n_ops;
    });
    frente[0].crowd = INFINITY;
    frente[n-1].crowd = INFINITY;
    double r2 = max(1e-12, (double)(frente[n-1].n_ops - frente[0].n_ops));
    for (int i = 1; i < n - 1; i++) {
        frente[i].crowd += (double)(frente[i + 1].n_ops - frente[i - 1].n_ops) / r2;
    }
}

// ============================================================================
// Operadores Genéticos
// ============================================================================
Individuo torneo_seleccion(const vector<Individuo>& poblacion, int tournament_size, mt19937& rng) {
    uniform_int_distribution<> dist(0, (int)poblacion.size() - 1);
    
    Individuo mejor = poblacion[dist(rng)];
    for (int i = 1; i < tournament_size; i++) {
        Individuo cand = poblacion[dist(rng)];
        if (cand.rank < mejor.rank || (cand.rank == mejor.rank && cand.crowd > mejor.crowd)) {
            mejor = cand;
        }
    }
    return mejor;
}

Individuo crossover(const Individuo& p1, const Individuo& p2,
                    const vector<Bitset>& F, const Bitset& U, const Bitset& G,
                    int k, mt19937& rng) {
    // set<int> usados;
    // for (int idx : p1.expr.used_sets) {
    //     if (uniform_int_distribution<>(0,1)(rng) == 0) usados.insert(idx);
    // }
    // for (int idx : p2.expr.used_sets) {
    //     if (uniform_int_distribution<>(0,1)(rng) == 0) usados.insert(idx);
    // }

    // if (usados.empty()) {
    //     set<int> s = {-2};
    //     Expression e(Bitset(), "∅", s, 0);
    //     Individuo h;
    //     h.expr = e;
    //     h.n_ops = 0;
    //     h.jaccard = M(h.expr, G, Metric::Jaccard);
    //     h.sizeH = (int)M(h.expr, G, Metric::SizeH);
    //     return h;
    // }
    
    // vector<int> conjs(usados.begin(), usados.end());
    // Expression e = build_random_expr(conjs, F, U, k, rng);

    // Individuo h;
    // h.expr = move(e);

    // h.jaccard = M(h.expr, G, Metric::Jaccard);
    // h.sizeH = (int)M(h.expr, G, Metric::SizeH);
    // h.n_ops = h.expr.n_ops;
    // return h;

    const Individuo* left_parent;
    const Individuo* right_parent;
    if (uniform_int_distribution<>(0,1)(rng)==0){
        left_parent = &p1;
        right_parent = &p2;
    }
    else {
        left_parent = &p2;
        right_parent = &p1;
    }

    int new_ops = left_parent->n_ops+right_parent->n_ops+1;
    if(new_ops>k){
        return(p1.jaccard>p2.jaccard)?p1:p2;
    }

    int op = uniform_int_distribution<>(0,2)(rng);
    string op_str = (op == 0) ? " ∪ " : (op == 1) ? " ∩ " : " \\ ";

    Bitset new_set;
    if (op == 0) new_set = set_union(left_parent->expr.conjunto, right_parent->expr.conjunto);
    else if (op == 1) new_set = set_intersect(left_parent->expr.conjunto, right_parent->expr.conjunto);
    else new_set = set_difference(left_parent->expr.conjunto, right_parent->expr.conjunto);

    // 4. Construir la expresión y los sets
    string new_expr = "(" + left_parent->expr.expr_str + op_str + right_parent->expr.expr_str + ")";
    
    set<int> combined_sets = left_parent->expr.used_sets;
    combined_sets.insert(right_parent->expr.used_sets.begin(), right_parent->expr.used_sets.end());

    Expression e_hijo(new_set, new_expr, combined_sets, new_ops);
    
    // 5. Evaluar y devolver
    double j = M(e_hijo, G, Metric::Jaccard);
    int sizeH = M(e_hijo, G, Metric::SizeH); // Usamos la métrica corregida que ignora U/∅
    
    return Individuo(e_hijo, new_ops, sizeH, j);
}

void mutar(Individuo& ind, const vector<Bitset>& F, const Bitset& U, const Bitset& G, int k, mt19937& rng, const vector<SolMO>& bloques_base)
{
    // Damos un 80% de probabilidad a un "ajuste fino" (crecimiento)
    // y un 20% a una "demolición" (reconstrucción aleatoria)
    std::uniform_real_distribution<double> dist_tipo(0.0, 1.0);
    int tipo = (dist_tipo(rng) < 0.80) ? 1 : 0; // 80% tipo 1, 20% tipo 0

    if (tipo == 0) {
        // --- Tipo 0: Mutación Destructiva (Exploración) ---
        // Útil para saltar fuera de un óptimo local si nos atascamos.
        
        vector<int> conjs(ind.expr.used_sets.begin(), ind.expr.used_sets.end());
        
        // Rango de índices completo: F (0..N-1), U (-1), ∅ (-2)
        uniform_int_distribution<> dist_idx(-1, (int)F.size() - 1);

        if (conjs.empty()) {
            conjs.push_back(dist_idx(rng));
        } else {
            // Añadir o reemplazar un gen aleatoriamente
            if (uniform_int_distribution<>(0,1)(rng) == 0 && !conjs.empty()) {
                // Reemplazar
                int pos = uniform_int_distribution<>(0,(int)conjs.size()-1)(rng);
                conjs[pos] = dist_idx(rng);
            } else {
                // Añadir
                conjs.push_back(dist_idx(rng));
            }
        }
        
        shuffle(conjs.begin(), conjs.end(), rng);
        ind.expr = build_random_expr(conjs, F, U, k, rng);

    } else {
        // --- Tipo 1: Mutación de Crecimiento (Ajuste Fino) ---
        // Coge al individuo y le aplica una operación con un bloque base.
        // No destruye la estructura, solo la hace crecer.
        
        int new_ops = ind.n_ops + 1; // +1 porque el bloque base es n_ops=0
        if (new_ops > k) {
            // Demasiado profundo, abortamos la mutación
            return; 
        }

        std::uniform_int_distribution<int> dist_op(0, 2);
        std::uniform_int_distribution<int> dist_base(0, bloques_base.size() - 1);
    
        int op = dist_op(rng);
        const SolMO& right = bloques_base[dist_base(rng)];
        string op_str = (op == 0) ? " ∪ " : (op == 1) ? " ∩ " : " \\ ";

        Bitset new_set;
        string new_expr;
        
        // Aplicar la operación (50/50 de qué lado va)
        if (uniform_int_distribution<>(0,1)(rng) == 0) {
            // (Individuo op BloqueBase)
            if (op == 0) new_set = set_union(ind.expr.conjunto, right.expr.conjunto);
            else if (op == 1) new_set = set_intersect(ind.expr.conjunto, right.expr.conjunto);
            else new_set = set_difference(ind.expr.conjunto, right.expr.conjunto);
            new_expr = "(" + ind.expr.expr_str + op_str + right.expr.expr_str + ")";
        } else {
            // (BloqueBase op Individuo)
            if (op == 0) new_set = set_union(right.expr.conjunto, ind.expr.conjunto);
            else if (op == 1) new_set = set_intersect(right.expr.conjunto, ind.expr.conjunto);
            else new_set = set_difference(right.expr.conjunto, ind.expr.conjunto);
            new_expr = "(" + right.expr.expr_str + op_str + ind.expr.expr_str + ")";
        }

        set<int> combined_sets = ind.expr.used_sets;
        combined_sets.insert(right.expr.used_sets.begin(), right.expr.used_sets.end());

        ind.expr = Expression(new_set, new_expr, combined_sets, new_ops);
    }
    
    // --- RE-EVALUAR (Común a ambos tipos) ---
    ind.jaccard = M(ind.expr, G, Metric::Jaccard);
    ind.sizeH   = (int)M(ind.expr, G, Metric::SizeH);
    ind.n_ops   = ind.expr.n_ops;
}

// ============================================================================
// Inicialización aleatoria
// ============================================================================
vector<Individuo> inicializar_poblacion(const vector<Bitset>& F, const Bitset& U,
                                        const Bitset& G, int k, int pop_size, mt19937& rng) {
    vector<Individuo> pop;
    pop.reserve(pop_size);

    unordered_set<string> vistos_expr;
    vistos_expr.reserve(pop_size * 2);

    while ((int)pop.size() < pop_size) {
        int max_sets = min<int>((int)F.size()+1, k+1);
        int num_conjs = uniform_int_distribution<>(1, max_sets)(rng);

        set<int> usados;

        uniform_int_distribution<> dist_idx(-1, (int)F.size()-1);

        for (int i = 0; i < num_conjs; ++i)
            usados.insert(dist_idx(rng));

        vector<int> conjs(usados.begin(), usados.end());
        
        if (!conjs.empty()) { 
            
            Expression e = build_random_expr(conjs, F, U, k, rng);
            auto keyE = e.expr_str;
            
            // Solo continuamos si la expresión no ha sido vista antes
            if (vistos_expr.insert(keyE).second) { 
                // Todo es válido, procedemos a construir y añadir el individuo
                Individuo ind;
                ind.expr    = move(e);
                ind.jaccard = M(ind.expr, G, Metric::Jaccard);
                ind.sizeH   = (int)M(ind.expr, G, Metric::SizeH);
                ind.n_ops   = ind.expr.n_ops;
                ind.rank    = 0;
                ind.crowd   = 0.0;

                pop.push_back(move(ind));
            }
        }
    }

    return pop;
}
