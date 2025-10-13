#pragma once
#include <vector>
#include <chrono>
#include <random>
#include "expr.hpp"
#include "domain.hpp"
#include "solutions.hpp"  // Reutiliza SolMO, dominates, pareto_front

// Parámetros del GA
struct GAParams {
    int population_size = 200;
    int max_generations = 1e9;  // Alto, pero el límite real es el tiempo
    double crossover_prob = 0.8;
    double mutation_prob = 0.4;
    int tournament_size = 2;
    int time_limit_sec = 300;  // Límite de tiempo en segundos
    uint64_t seed = 0; // 0 = semilla aleatoria
    GAParams() = default;
};

// Funciones principales
std::vector<Individuo> nsga2(
    const std::vector<Bitset>& F,
    const Bitset& G,
    int k,
    const GAParams& params);

// Funciones auxiliares para NSGA-II
std::vector<std::vector<Individuo>> fast_non_dominated_sort(std::vector<Individuo>& poblacion);
void calcular_crowding_distance(std::vector<Individuo>& frente);
Individuo torneo_seleccion(const std::vector<Individuo>& poblacion, int tournament_size, std::mt19937& rng);
Individuo crossover(const Individuo& padre1, const Individuo& padre2, 
                   const std::vector<Bitset>& F, int k, std::mt19937& rng);
void mutar(Individuo& individuo, const std::vector<Bitset>& F, int k, std::mt19937& rng);
std::vector<Individuo> inicializar_poblacion(const std::vector<Bitset>& F, const Bitset& G, int k, int pop_size, std::mt19937& rng);