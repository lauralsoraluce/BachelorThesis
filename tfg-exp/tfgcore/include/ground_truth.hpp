//======================================================================
// ground_truth.hpp
//----------------------------------------------------------------------
// Generación de una instancia "ground truth":
//  - Construye una expresión aleatoria (≤ k ops) a partir de F
//  - Devuelve el G resultante de evaluarla, junto con F y la expresión
//    que lo generó.
//======================================================================

#pragma once

#include <vector>
#include <cstdint>
#include "domain.hpp"
#include "expr.hpp"

//------------------------------------------------------------------
// Estructura de la instancia generada
//------------------------------------------------------------------
struct GroundTruthInstance {
    std::vector<Bitset> F;   // conjuntos base
    Bitset G;                // conjunto objetivo
    Expression gold_expr;    // expresión que genera G
    std::uint64_t seed;           // semilla usada
};

//------------------------------------------------------------------
// Construcción de la instancia ground-truth
//------------------------------------------------------------------
GroundTruthInstance make_groundtruth(const Bitset& U, 
                                     int n = 128,
                                     int n_min = 6,
                                     int n_max = 10,
                                     int tam_min = 10,
                                     int tam_max = 80,
                                     int k = 10,
                                     std::uint64_t seed_F = 123);
