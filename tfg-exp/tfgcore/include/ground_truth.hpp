#pragma once
#include "genetico.hpp"
#include <random>
#include <vector>

// ============================================================================
// Módulo de generación "ground truth"
// Crea una expresión aleatoria (≤k ops) a partir de F y devuelve el G resultante
// ============================================================================

struct GroundTruthInstance {
    std::vector<Bitset> F;   // conjuntos base
    Bitset G;                // conjunto objetivo
    Expression gold_expr;    // expresión que genera G
    uint64_t seed;           // semilla usada
};

// 
// y el G resultante de evaluarla.
GroundTruthInstance make_groundtruth(int n = 128,
                                     int n_min = 6,
                                     int n_max = 10,
                                     int tam_min = 10,
                                     int tam_max = 80,
                                     int k = 10,
                                     uint64_t seed_F = 123,
                                     uint64_t seed_expr = 20251019);
