#pragma once
#include <vector>
#include <random>
#include "expr.hpp"
#include "domain.hpp"
#include "solutions.hpp"  // Individuo, dominates, pareto_front
#include "metrics.hpp"    // M()

// -------------------- Parámetros del algoritmo SPEA2 --------------------
struct SPEA2_Params {
    int population_size = 200;     // N
    int archive_size    = 200;     // tamaño del archivo A
    int max_generations = 1e9;     // controlamos por tiempo
    int time_limit_sec  = 300;     // presupuesto temporal (segundos)
    double pc = 0.85;              // probabilidad de cruce
    double pm = 0.35;              // probabilidad de mutación
    int tournament_k    = 2;       // torneo binario
    uint64_t seed       = 0;       // 0 → aleatoria (reloj)
    SPEA2_Params() = default;
};

// -------------------- Funciones principales --------------------
std::vector<Individuo> spea2(
    const std::vector<Bitset>& F,
    const Bitset& G,
    int k,
    const SPEA2_Params& params);

// -------------------- Funciones auxiliares --------------------

// Evaluación de objetivos (ya definida en el .cpp)
void evaluar_obj(Individuo& ind, const Bitset& G);

// Selección ambiental (archivo externo)
std::vector<Individuo> environmental_selection(
    const std::vector<Individuo>& poblacion,
    const std::vector<Individuo>& archivo_anterior,
    int archive_size);

// Operadores genéticos (reutilizan tus funciones existentes)
Individuo crossover(
    const Individuo& p1,
    const Individuo& p2,
    const std::vector<Bitset>& F,
    int k,
    std::mt19937& rng);

void mutar(
    Individuo& individuo,
    const std::vector<Bitset>& F,
    int k,
    std::mt19937& rng);

std::vector<Individuo> init_pop(
    const std::vector<Bitset>& F,
    const Bitset& G,
    int k,
    int N,
    std::mt19937& rng);

