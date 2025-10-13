#include "genetico.hpp"
#include "metrics.hpp"
#include <algorithm>
#include <cmath>
#include <set>
#include <unordered_set>
#include <iostream>
#include <string>
#include <random>   // por si no viene de .hpp
#include <chrono>   // por si no viene de .hpp

using namespace std;

// --- claves de deduplicación ---
static inline std::string key_of_H(const Expression& e)   { return e.conjunto.to_string(); }
static inline std::string key_of_expr(const Expression& e){ return e.expr_str; }

// dedup por H (antes de devolver/imprimir)
template <typename T>
static std::vector<T> unique_by_H(const std::vector<T>& v) {
    std::vector<T> out;
    std::unordered_set<std::string> seen;
    out.reserve(v.size());
    for (const auto& x : v) {
        auto key = key_of_H(x.expr);
        if (seen.insert(key).second) out.push_back(x);
    }
    return out;
}

// ------------ árbol binario aleatorio (≤ k ops) ------------
static Expression build_random_expr(const std::vector<int>& conjs, const std::vector<Bitset>& F, int k, std::mt19937& rng){
    if (conjs.empty()) return Expression(Bitset(), "∅", {}, 0);
    if (conjs.size() == 1) {
        int idx = conjs[0];
        return Expression(F[idx], "F"+std::to_string(idx), {idx}, 0);
    }
    struct Node{ Expression e; };
    std::vector<Node> pool; pool.reserve(conjs.size());
    for (int idx : conjs) pool.push_back({ Expression(F[idx], "F"+std::to_string(idx), {idx}, 0) });

    int intentos_fallidos = 0;
    const int max_intentos = 100;

    while (pool.size() > 1 && intentos_fallidos < max_intentos) {
        std::uniform_int_distribution<> pick(0, (int)pool.size()-1);
        int a = pick(rng), b = pick(rng); if (a==b) {intentos_fallidos++; continue;}; if (a>b) std::swap(a,b);
        int op = std::uniform_int_distribution<>(0,2)(rng); 
        
        const char* op_str = (op==0) ?" ∪ ":(op==1)?" ∩ ":" \\ ";

        Bitset H;
        if (op==0) H = set_union (pool[a].e.conjunto, pool[b].e.conjunto);
        if (op==1) H = set_intersect(pool[a].e.conjunto, pool[b].e.conjunto);
        if (op==2) H = set_difference(pool[a].e.conjunto, pool[b].e.conjunto);


        int ops_new = pool[a].e.n_ops + pool[b].e.n_ops + 1;
        if (ops_new > k) {intentos_fallidos++; continue;}; // no exceder k

        std::set<int> usados = pool[a].e.used_sets;
        usados.insert(pool[b].e.used_sets.begin(), pool[b].e.used_sets.end());
        std::string expr_str = "(" + pool[a].e.expr_str + op_str + pool[b].e.expr_str + ")";


        pool[a].e = Expression(H, expr_str, usados, ops_new);
        pool.erase(pool.begin()+b);
        intentos_fallidos = 0;
    }
    return pool.front().e;
}

// ============================================================================
// NSGA-II Principal
// ============================================================================
vector<Individuo> nsga2(
    const vector<Bitset>& F,
    const Bitset& G,
    int k,
    const GAParams& params)
{
    // Semilla: si es 0, usar tiempo actual (no determinista)
    uint64_t seed = params.seed ? params.seed : (uint64_t)std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::mt19937 rng(seed);

    auto start_time = chrono::steady_clock::now();
    auto time_limit = chrono::seconds(params.time_limit_sec);

    // 1) Población inicial 100% aleatoria
    vector<Individuo> poblacion = inicializar_poblacion(F, G, k, params.population_size, rng);

    int generation = 0;

    while (generation < params.max_generations) {
        // Verificar tiempo
        if (chrono::steady_clock::now() - start_time >= time_limit) {
            break;
        }

        // 2) Offspring con anti-duplicados por estructura
        vector<Individuo> offspring;
        offspring.reserve(params.population_size);
        
        std::unordered_set<std::string> seen_gen;
        seen_gen.reserve(params.population_size * 2);
        for (const auto& ind : poblacion) seen_gen.insert(key_of_expr(ind.expr));

        while ((int)offspring.size() < params.population_size) {
            Individuo p1 = torneo_seleccion(poblacion, params.tournament_size, rng);
            Individuo p2 = torneo_seleccion(poblacion, params.tournament_size, rng);

            Individuo hijo;
            if (uniform_real_distribution<>(0,1)(rng) < params.crossover_prob) {
                hijo = crossover(p1, p2, F, k, rng);
            } else {
                hijo = (uniform_int_distribution<>(0,1)(rng) == 0) ? p1 : p2;
            }

            if (uniform_real_distribution<>(0,1)(rng) < params.mutation_prob) {
                mutar(hijo, F, k, rng);
            }

            hijo.jaccard = M(hijo.expr, G, Metric::Jaccard);
            hijo.sizeH   = (int)M(hijo.expr, G, Metric::SizeH);
            hijo.n_ops   = hijo.expr.n_ops;

            if (seen_gen.insert(key_of_expr(hijo.expr)).second) {
                offspring.push_back(std::move(hijo));
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

        for (auto& Fi : Flist) {
            if (Fi.empty()) continue;
            
            calcular_crowding_distance(Fi);
            
            if (Pnext.size() + Fi.size() <= (size_t)params.population_size) {
                Pnext.insert(Pnext.end(), Fi.begin(), Fi.end());
            } else {
                // Ordenar por crowding distance
                sort(Fi.begin(), Fi.end(),
                    [](const Individuo& a, const Individuo& b) {
                        if (isinf(a.crowd) && !isinf(b.crowd)) return true;
                        if (!isinf(a.crowd) && isinf(b.crowd)) return false;
                        return a.crowd > b.crowd;
                    });
                
                size_t need = params.population_size - Pnext.size();
                Pnext.insert(Pnext.end(), Fi.begin(), Fi.begin() + need);
                break;
            }
        }

        poblacion = std::move(Pnext);

        generation++;
    }

    auto pareto = pareto_front(poblacion);
    return unique_by_H(pareto);
}

// ============================================================================
// Fast Non-Dominated Sort (NSGA-II)
// ============================================================================
vector<vector<Individuo>> fast_non_dominated_sort(vector<Individuo>& poblacion) {
    int n = (int)poblacion.size();
    vector<int> domination_count(n, 0);
    vector<vector<int>> dominated_set(n);
    vector<vector<Individuo>> frentes;

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (dominates(poblacion[i], poblacion[j])) {
                dominated_set[i].push_back(j);
                domination_count[j]++;
            } else if (dominates(poblacion[j], poblacion[i])) {
                dominated_set[j].push_back(i);
                domination_count[i]++;
            }
        }
    }

    vector<Individuo> frente0;
    for (int i = 0; i < n; i++) {
        if (domination_count[i] == 0) {
            poblacion[i].rank = 0;
            frente0.push_back(poblacion[i]);
        }
    }
    if (!frente0.empty()) frentes.push_back(frente0);

    int rank = 0;
    while (rank < (int)frentes.size() && !frentes[rank].empty()) {
        vector<Individuo> siguiente;
        for (const auto& ind : frentes[rank]) {
            int idx = -1;
            for (int i = 0; i < n; i++) {
                if (poblacion[i].expr.expr_str == ind.expr.expr_str) { idx = i; break; }
            }
            if (idx == -1) continue;

            for (int j : dominated_set[idx]) {
                if (--domination_count[j] == 0) {
                    poblacion[j].rank = rank + 1;
                    siguiente.push_back(poblacion[j]);
                }
            }
        }
        if (siguiente.empty()) break;
        frentes.push_back(siguiente);
        rank++;
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
                    const vector<Bitset>& F, int k, mt19937& rng) {
    set<int> usados;
    for (int idx : p1.expr.used_sets) {
        if (uniform_int_distribution<>(0,1)(rng) == 0) usados.insert(idx);
    }
    for (int idx : p2.expr.used_sets) {
        if (uniform_int_distribution<>(0,1)(rng) == 0) usados.insert(idx);
    }

    if (usados.empty()) {
        Individuo h;
        h.expr = Expression(Bitset(), "∅", {}, 0);
        h.n_ops = 0;
        h.jaccard = 0.0;
        h.sizeH = 0;
        return h;
    }
    
    vector<int> conjs(usados.begin(), usados.end());
    Expression e = build_random_expr(conjs, F, k, rng);

    Individuo h;
    h.expr = std::move(e);
    h.n_ops = h.expr.n_ops;
    return h;
}

void mutar(Individuo& ind, const std::vector<Bitset>& F, int k, std::mt19937& rng){
    int tipo = std::uniform_int_distribution<>(0,2)(rng);
    std::vector<int> conjs(ind.expr.used_sets.begin(), ind.expr.used_sets.end());
    
    if (tipo == 0) {
        // Reorganizar conjuntos existentes
        if (conjs.empty()) {
            int idx = std::uniform_int_distribution<>(0,(int)F.size()-1)(rng);
            ind.expr = Expression(F[idx], "F"+std::to_string(idx), {idx}, 0);
        } else {
            std::shuffle(conjs.begin(), conjs.end(), rng);
            ind.expr = build_random_expr(conjs, F, k, rng);
        }
    } else if (tipo == 1) {
        // Reemplazar un conjunto
        if (conjs.empty()) {
            int idx = std::uniform_int_distribution<>(0,(int)F.size()-1)(rng);
            ind.expr = Expression(F[idx], "F"+std::to_string(idx), {idx}, 0);
        } else {
            int pos = std::uniform_int_distribution<>(0,(int)conjs.size()-1)(rng);
            conjs[pos] = std::uniform_int_distribution<>(0,(int)F.size()-1)(rng);
            std::shuffle(conjs.begin(), conjs.end(), rng);
            ind.expr = build_random_expr(conjs, F, k, rng);
        }
    } else {
        // Añadir un nuevo conjunto
        conjs.push_back(std::uniform_int_distribution<>(0,(int)F.size()-1)(rng));
        std::shuffle(conjs.begin(), conjs.end(), rng);
        ind.expr = build_random_expr(conjs, F, k, rng);
    }
    
    ind.n_ops = ind.expr.n_ops;
}

// ============================================================================
// Inicialización aleatoria
// ============================================================================
vector<Individuo> inicializar_poblacion(const vector<Bitset>& F,
                                        const Bitset& G, int k, int pop_size, mt19937& rng) {
    vector<Individuo> pop;
    pop.reserve(pop_size);
    unordered_set<string> vistos_expr;
    vistos_expr.reserve(pop_size * 2);

    while ((int)pop.size() < pop_size) {
        int max_sets = std::min<int>((int)F.size(), k+1);
        int num_conjs = uniform_int_distribution<>(1, max_sets)(rng);

        set<int> usados;
        for (int i = 0; i < num_conjs; ++i)
            usados.insert(uniform_int_distribution<>(0,(int)F.size()-1)(rng));

        vector<int> conjs(usados.begin(), usados.end());
        if (conjs.empty()) continue;

        Expression e = build_random_expr(conjs, F, k, rng);
        auto keyE = key_of_expr(e);
        if (!vistos_expr.insert(keyE).second) continue;

        Individuo ind;
        ind.expr    = std::move(e);
        ind.jaccard = M(ind.expr, G, Metric::Jaccard);
        ind.sizeH   = (int)M(ind.expr, G, Metric::SizeH);
        ind.n_ops   = ind.expr.n_ops;
        ind.rank    = 0;
        ind.crowd   = 0.0;

        pop.push_back(std::move(ind));
    }

    return pop;
}
