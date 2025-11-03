//======================================================================
// solutions.hpp
//----------------------------------------------------------------------
// Tipos y utilidades comunes para soluciones multiobjetivo:
//   - SolMO: solución con métricas (Jaccard, |H|, nº ops)
//   - Individuo: añade rank y crowding para NSGA-II
//   - dominates(): relación de dominancia
//   - pareto_front(): frente de Pareto para SolMO/Individuo
//   - print_pareto_front(): salida simple a stdout
//======================================================================

#pragma once

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <vector>

#include "expr.hpp"

//------------------------------------------------------------------
// Solución multiobjetivo base
//------------------------------------------------------------------
struct SolMO {
    Expression expr;
    int n_ops = 0;
    int sizeH = 0;
    double jaccard = 0.0;

    SolMO() = default;
    SolMO(const Expression& e, int ops, int size, double j)
        : expr(e), n_ops(ops), sizeH(size), jaccard(j) {}
};

//------------------------------------------------------------------
// Individuo (para GA/NSGA-II)
//------------------------------------------------------------------
struct Individuo : public SolMO{
    int rank = 0;
    double crowd = 0.0;
    Individuo() = default;
    Individuo(const Expression& e, int ops, int size, double j)
        : SolMO(e, ops, size, j), rank(0), crowd(0.0) {}
    Individuo(const SolMO& s)
        : SolMO(s), rank(0), crowd(0.0) {}
};

//------------------------------------------------------------------
// Dominancia: max Jaccard, min n_ops, min |H|
//------------------------------------------------------------------
inline bool dominates(const SolMO& a, const SolMO& b) {
    bool ge = (a.jaccard >= b.jaccard) && (a.n_ops <= b.n_ops) && (a.sizeH <= b.sizeH);
    bool gt = (a.jaccard >  b.jaccard) || (a.n_ops <  b.n_ops) || (a.sizeH <  b.sizeH);
    return ge && gt;
}

//------------------------------------------------------------------
// Frente de Pareto genérico (SolMO / Individuo)
//------------------------------------------------------------------
template<typename T>
std::vector<T> pareto_front_generic(const std::vector<T>& v) {
    // if (v.empty()) return {};
    
    // std::vector<T> nd;
    // nd.reserve(v.size());
    
    // for (const auto& sol : v) {
    //     bool is_dominated = false;
    //     for (const auto& other : v) {
    //         if (&sol == &other) continue;
    //         if (dominates(other, sol)) {
    //             is_dominated = true;
    //             break;
    //         }
    //     }
    //     if (!is_dominated) {
    //         nd.push_back(sol);
    //     }
    // }
    
    // std::stable_sort(nd.begin(), nd.end(), [](const T& a, const T& b) {
    //     if (a.jaccard != b.jaccard) return a.jaccard > b.jaccard; // descendiente
    //     if (a.sizeH != b.sizeH) return a.sizeH < b.sizeH; // ascendiente
    //     return a.n_ops < b.n_ops; // ascendiente
    // });

    if (v.empty()) return {};
    
    std::vector<T> nd; // El frente no dominado
    
    // Reserva espacio (opcional, pero puede ayudar)
    // Asumimos que el frente será mucho más pequeño que v
    nd.reserve(v.size() / 10 + 1); 

    for (const auto& sol : v) { // Bucle externo: O(N)
        bool sol_is_dominated = false;
        
        // Iteramos sobre el frente 'nd' actual usando un índice
        // (Necesario porque podemos eliminar elementos)
        int i = 0;
        while (i < nd.size()) { // Bucle interno: O(M)
            
            const auto& other = nd[i];

            if (dominates(other, sol)) {
                // 'other' (en el frente) domina a 'sol'.
                // 'sol' es inútil, dejamos de comprobar.
                sol_is_dominated = true;
                break;
            }
            
            if (dominates(sol, other)) {
                // 'sol' domina a 'other' (que estaba en el frente).
                // Eliminamos 'other' del frente.
                
                // Técnica "swap-and-pop": O(1) para eliminar
                // (rompe el orden, pero lo arreglamos al final)
                nd[i] = nd.back();
                nd.pop_back();
                
                // NO incrementamos 'i', porque necesitamos comprobar
                // el nuevo elemento que acabamos de mover a 'nd[i]'.
            } else {
                // Ni 'sol' domina 'other', ni 'other' domina 'sol'.
                // Pasamos al siguiente elemento del frente.
                i++;
            }
        }

        // Si 'sol' no fue dominado por nadie del frente actual,
        // entonces 'sol' pertenece al frente.
        if (!sol_is_dominated) {
            nd.push_back(sol);
        }
    }
    
    std::stable_sort(nd.begin(), nd.end(), [](const T& a, const T& b) {
        if (a.jaccard != b.jaccard) return a.jaccard > b.jaccard; // descendiente
        if (a.sizeH != b.sizeH) return a.sizeH < b.sizeH; // ascendiente
        return a.n_ops < b.n_ops; // ascendiente
    });
    
    return nd;
}

inline std::vector<SolMO> pareto_front(const std::vector<SolMO>& v) {
    return pareto_front_generic(v);
}

inline std::vector<Individuo> pareto_front(const std::vector<Individuo>& v) {
    return pareto_front_generic(v);
}

//------------------------------------------------------------------
// Impresión sencilla del frente de Pareto
//------------------------------------------------------------------
template<typename T>
void print_pareto_front_generic(const vector<T>& pareto) {
    std::cout << "\n=== FRENTE DE PARETO ===" << std::endl;
    std::cout << "Total soluciones no dominadas: " << pareto.size() << "\n" << std::endl;
    
    for (size_t i = 0; i < pareto.size(); i++) {
        std::cout << "Solución " << (i+1) << ":" << std::endl;
        std::cout << "  Expresión: " << pareto[i].expr.expr_str << std::endl;
        std::cout << "  Jaccard: " << pareto[i].jaccard << std::endl;
        std::cout << "  Operaciones: " << pareto[i].n_ops << std::endl;
        std::cout << "  |H|: " << pareto[i].sizeH << std::endl;
        std::cout << std::endl;
    }
}

inline void print_pareto_front(const std::vector<SolMO>& pareto) {
    print_pareto_front_generic(pareto);
}

inline void print_pareto_front(const std::vector<Individuo>& pareto) {
    print_pareto_front_generic(pareto);
}