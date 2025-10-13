#pragma once
#include <string>
#include "domain.hpp"
#include <set>

using namespace std;

// Estructura de expresión común a todos los módulos
struct Expression {
    Bitset conjunto;
    string expr_str;
    set<int> used_sets;            // Conjuntos base usados (para métricas)
    int n_ops = 0;                 // Número de operaciones (para métricas)

    Expression() = default;
    Expression(Bitset c, std::string s)
        : conjunto(std::move(c)), expr_str(std::move(s)) {}
    
    // Constructor con conjuntos usados
    Expression(const Bitset& c, const std::string& s, const std::set<int>& sets, int ops=0)
        : conjunto(c), expr_str(s), used_sets(sets), n_ops(ops) {}
};
