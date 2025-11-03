//======================================================================
// greedy.cpp
//----------------------------------------------------------------------
// Heurística greedy para construir una expresión (≤ k ops) maximizando
// Jaccard y, a igualdad, minimizando |H| y nº de operaciones.
//======================================================================

#include "metrics.hpp"
#include "greedy.hpp"
#include "solutions.hpp"

#include <algorithm>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

using namespace std;

//------------------------------------------------------------------
// Greedy principal
//------------------------------------------------------------------
vector<SolMO> greedy(
    const vector<Bitset>& F,
    const Bitset& G,
    int k)
{
    Expression curr(Bitset(), "∅", set<int>{}, 0);
    vector<SolMO> candidatos;

    unordered_set<string> seen;
    auto try_push = [&](const Expression& e) {
        string key = e.conjunto.to_string();
        if (seen.insert(key).second) {
            double j     = M(e, G, Metric::Jaccard);
            int    sizeH = (int)M(e, G, Metric::SizeH);
            int    n_ops = (int)M(e, G, Metric::OpSize);
            candidatos.emplace_back(e, n_ops, sizeH, j);   
        }
    };

    // Punto de partida: vacío
    try_push(curr);

    for (int iter = 0; iter < k; ++iter) {
        bool found = false;
        double best_j = -1.0; 
        int best_sz = 1e9; 
        int best_ops = 1e9;
        Expression bestE;

        for (int op = 0; op < 3; ++op) {
            for (int i = 0; i <= (int)F.size(); ++i) {
                bool isU = (i==(int)F.size());

                Bitset rhs;
                string rhs_name;
                if (isU) {
                    rhs.set();      // U
                    rhs_name = "U";
                }
                else {
                    rhs = F[i];     // F_i
                    rhs_name = "F" + std::to_string(i);
                }

                Bitset H_new = apply_op(op, curr.conjunto, rhs);

                set<int> sets_new = curr.used_sets; 
                if (!isU) sets_new.insert(i);

                int ops_new = (curr.expr_str == "∅") ? 0 : (curr.n_ops + 1);

                const char* opstr = (op == 0) ? " ∪ " : (op == 1) ? " ∩ " : " \\ ";
                string expr_new = (curr.expr_str == "∅")
                    ? rhs_name
                    : ("(" + curr.expr_str + opstr + rhs_name + ")");

                Expression E_new(H_new, std::move(expr_new), sets_new, ops_new);
                try_push(E_new);

                double j  = M(E_new, G, Metric::Jaccard);
                int sH    = (int)M(E_new, G, Metric::SizeH);
                int nops  = (int)M(E_new, G, Metric::OpSize);

                if (j > best_j || (j == best_j && (sH < best_sz || (sH == best_sz && nops < best_ops)))) {
                    best_j = j; best_sz = sH; best_ops = nops; bestE = std::move(E_new); found = true;
                }
            }
        }
        if (!found) break;

        curr = move(bestE);
        try_push(curr);
    }
    
    // Devuelve el frente de Pareto
    return pareto_front(candidatos);
}

vector<SolMO> greedy_multiobjective_search(
    const vector<Bitset>& F,
    const Bitset& U,
    const Bitset& G,
    int k)
{
    // Almacena los bloques de construcción (solo los no dominados globalmente)
    // para cada nivel.
    vector<vector<SolMO>> frente_nivel(k + 1);
    
    // Almacena el frente de Pareto global acumulativo
    vector<SolMO> frente_global;

    vector<SolMO> bloques_base;
    bloques_base.reserve(F.size()+1);

    // --- Nivel 0: Generar, Evaluar y FILTRAR ---
    {
        // 1.a. Generar conjuntos base (F_i)
        for (size_t i = 0; i < F.size(); i++) {
            set<int> sets = {static_cast<int>(i)};
            Expression e(F[i], "F" + to_string(i), sets, 0); 
            double j = M(e, G, Metric::Jaccard);
            int sizeH = M(e, G, Metric::SizeH);
            bloques_base.emplace_back(e, 0, sizeH, j);
        }
        
        // 1.b. Generar U
        set<int> u_set = {}; 
        Expression e_u(U, "U", u_set, 0);
        double j_u = M(e_u, G, Metric::Jaccard);
        int sizeH_u = M(e_u, G, Metric::SizeH);
        bloques_base.emplace_back(e_u, 0, sizeH_u, j_u);

        // 1.c. Generar Vacío
        /*set<int> empty_sets;
        Expression e_empty(Bitset(), "∅", empty_sets, 0);
        double j_empty = M(e_empty, G, Metric::Jaccard);
        int sizeH_empty = M(e_empty, G, Metric::SizeH);
        bloques_base.emplace_back(e_empty, 0, sizeH_empty, j_empty);*/
        
        // 2. Calcular el frente inicial
        frente_nivel[0] = pareto_front(bloques_base);
        frente_global = frente_nivel[0]; // El primer frente global
    }

    // --- Generar niveles s = 1...k ---
    for (int s = 1; s <= k; s++) {
        
        vector<SolMO> candidatos_s;
        
        for (int op = 0; op < 3; op++) {
            string op_str = (op == 0) ? " ∪ " : (op == 1) ? " ∩ " : " \\ ";
            
            // *** ¡CAMBIO DE LÓGICA! ***
            // Iteramos solo sobre el FRENTE ANTERIOR (s-1)
            for (const SolMO& sol_left : frente_nivel[s-1]) {
                for (const SolMO& sol_right : bloques_base) {
                    
                    const Expression& left = sol_left.expr;
                    const Expression& right = sol_right.expr;

                    Bitset new_set;
                    if (op == 0) new_set = set_union(left.conjunto, right.conjunto);
                    else if (op == 1) new_set = set_intersect(left.conjunto, right.conjunto);
                    else new_set = set_difference(left.conjunto, right.conjunto);
                    
                    string new_expr = "(" + left.expr_str + op_str + 
                                     right.expr_str + ")";
                    
                    set<int> combined_sets = left.used_sets;
                    combined_sets.insert(right.used_sets.begin(), right.used_sets.end());
                    
                    // El nuevo número de operaciones es s
                    Expression e_nueva(new_set, new_expr, combined_sets, s); 
                    
                    double j = M(e_nueva, G, Metric::Jaccard);
                    int sizeH = M(e_nueva, G, Metric::SizeH);
                    
                    candidatos_s.emplace_back(e_nueva, s, sizeH, j);
                }
            }
        }
        
        if (candidatos_s.empty()) {
            continue; 
        }

        // 1. Obtener el frente local de este nivel
        vector<SolMO> frente_local_s = pareto_front(candidatos_s);
        
        if (frente_local_s.empty()) {
            continue; 
        }

        // 2. *** PODA ESTRICTA ***
        // (La misma lógica de poda que implementamos antes)
        
        vector<SolMO> combined_front = frente_global;
        combined_front.insert(combined_front.end(), 
                              frente_local_s.begin(), frente_local_s.end());
        
        // 3. Calculamos el NUEVO frente global
        vector<SolMO> new_global_front = pareto_front(combined_front);

        // 4. Extraemos los bloques de construcción para el futuro
        frente_nivel[s].clear();
        for (const auto& sol : new_global_front) {
            if (sol.expr.n_ops == s) {
                frente_nivel[s].push_back(sol);
            }
        }
        
        // 5. Asignamos el nuevo frente global para la siguiente iteración
        frente_global = std::move(new_global_front);
    }
    
    // Devolvemos el frente global final
    return frente_global;
}