//======================================================================
// genetico.hpp
//----------------------------------------------------------------------
// Algoritmo Genético NSGA-II para generar y optimizar expresiones
// sobre conjuntos, con límites por generación o tiempo.
//======================================================================

#pragma once

#include <vector>
#include <random>
#include "expr.hpp"
#include "domain.hpp"
#include "solutions.hpp"

//------------------------------------------------------------------
// Parámetros del algoritmo genético
//------------------------------------------------------------------
struct GAParams {
    int population_size = 200;
    int max_generations = 1e9;      // el límite real es el tiempo
    double crossover_prob = 0.8;
    double mutation_prob = 0.4;
    int tournament_size = 2;
    int time_limit_sec = 300;       // Límite de tiempo en segundos
    uint64_t seed = 0;              // 0 => semilla aleatoria
    GAParams() = default;
};

//------------------------------------------------------------------
// Algoritmo principal (NSGA-II)
//------------------------------------------------------------------
std::vector<Individuo> nsga2(
    const std::vector<Bitset>& F,
    const Bitset& U,
    const Bitset& G,
    int k,
    const GAParams& params);
    
//------------------------------------------------------------------
// Construcción aleatoria de expresiones (individuos)
//------------------------------------------------------------------
Expression build_random_expr(
    const std::vector<int>& available_sets,
    const std::vector<Bitset>& F,
    const Bitset& U,
    int k,
    std::mt19937& rng);

//------------------------------------------------------------------
// Utilidades NSGA-II
//------------------------------------------------------------------
std::vector<std::vector<Individuo>> fast_non_dominated_sort(std::vector<Individuo>& poblacion);
void calcular_crowding_distance(std::vector<Individuo>& frente);
Individuo torneo_seleccion(const std::vector<Individuo>& poblacion, int tournament_size, std::mt19937& rng);
Individuo crossover(const Individuo& p1, const Individuo& p2,
                    const vector<Bitset>& F, const Bitset& U, const Bitset& G,
                    int k, mt19937& rng);
void mutar(Individuo& individuo, const std::vector<Bitset>& F, const Bitset& U, const Bitset& G, int k, std::mt19937& rng, const vector<SolMO>& bloques_base);
std::vector<Individuo> inicializar_poblacion(const vector<Bitset>& F, const Bitset& U,
                                        const Bitset& G, int k, int pop_size, mt19937& rng);
Expression build_random_expr_v2(const vector<int>& conjs, const vector<Bitset>& F, int k, mt19937& rng);
Individuo crossover_v2(const Individuo& p1, const Individuo& p2,
                       const vector<Bitset>& F, int k, mt19937& rng);
void mutar_adaptativa(Individuo& ind, const vector<Bitset>& F, int k, 
                      mt19937& rng, double jaccard_actual);
vector<Individuo> nsga2_mejorado(
    const vector<Bitset>& F,
    const Bitset& G,
    int k,
    const GAParams& params);