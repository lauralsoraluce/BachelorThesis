#pragma once
#include <vector>
#include <algorithm>
#include "expr.hpp"  // donde esté tu struct Expression
#include <iostream>

// === Solución multiobjetivo común ===
struct SolMO {
    Expression expr;
    int n_ops = 0;
    int sizeH = 0;
    double jaccard = 0.0;

    SolMO() = default;
    SolMO(const Expression& e, int ops, int size, double j)
        : expr(e), n_ops(ops), sizeH(size), jaccard(j) {}
    virtual ~SolMO() = default;
};

struct Individuo : public SolMO{
    int rank = 0;
    double crowd = 0.0;

    Individuo() = default;
    Individuo(const Expression& e, int ops, int size, double j)
        : SolMO(e, ops, size, j), rank(0), crowd(0.0) {}
    Individuo(const SolMO& s)
        : SolMO(s), rank(0), crowd(0.0) {}
};

// Dominancia: max Jaccard, min n_ops, min |H|
inline bool dominates(const SolMO& a, const SolMO& b) {
    bool ge = (a.jaccard >= b.jaccard) && (a.n_ops <= b.n_ops) && (a.sizeH <= b.sizeH);
    bool gt = (a.jaccard >  b.jaccard) || (a.n_ops <  b.n_ops) || (a.sizeH <  b.sizeH);
    return ge && gt;
}

// Pareto front genérico (funciona con SolMO e Individuo)
template<typename T>
std::vector<T> pareto_front_generic(const std::vector<T>& v) {
    if (v.empty()) return {};
    
    std::vector<T> nd;
    nd.reserve(v.size());
    
    for (const auto& sol : v) {
        bool is_dominated = false;
        for (const auto& other : v) {
            if (&sol == &other) continue;
            if (dominates(other, sol)) {
                is_dominated = true;
                break;
            }
        }
        if (!is_dominated) {
            nd.push_back(sol);
        }
    }
    
    // Ordenar frente de Pareto
    std::stable_sort(nd.begin(), nd.end(), [](const T& a, const T& b) {
        if (a.jaccard != b.jaccard) return a.jaccard > b.jaccard;
        if (a.sizeH != b.sizeH) return a.sizeH < b.sizeH;
        return a.n_ops < b.n_ops;
    });
    
    return nd;
}

inline std::vector<SolMO> pareto_front(const std::vector<SolMO>& v) {
    return pareto_front_generic(v);
}

inline std::vector<Individuo> pareto_front(const std::vector<Individuo>& v) {
    return pareto_front_generic(v);
}

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