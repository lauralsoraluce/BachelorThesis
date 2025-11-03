
//======================================================================
// ground_truth.cpp
//----------------------------------------------------------------------
// Construye una instancia "ground truth":
//   - Genera F
//   - Crea una expresión aleatoria (≤ k ops) con build_random_expr
//   - Toma G como el resultado de esa expresión
//======================================================================

#include "ground_truth.hpp"
#include "generator.hpp"
#include "genetico.hpp"

#include <numeric> 
#include <random>
#include <utility>

using namespace std;

GroundTruthInstance make_groundtruth(const Bitset& U, int n, int n_min, int n_max,
                                     int tam_min, int tam_max,
                                     int k, uint64_t seed_F)
{
    // 1. Generar F
    vector<Bitset> F = generar_F(n, n_min, n_max, tam_min, tam_max, (int)seed_F);

    // 2. Construir expresión aleatoria con el generador del genético
    mt19937 rng(seed_F);
    vector<int> indices(F.size());
    iota(indices.begin(), indices.end(), 0);

    Expression gold = build_random_expr(indices, F, U, k, rng);

    // 3. G es el resultado de evaluar esa expresión
    Bitset G = gold.conjunto;

    // 4. Empaquetar todo
    GroundTruthInstance inst;
    inst.F = move(F);
    inst.G = move(G);
    inst.gold_expr = move(gold);
    inst.seed = seed_F;

    return inst;
}
