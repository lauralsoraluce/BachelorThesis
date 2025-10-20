#include "ground_truth.hpp"
#include <numeric>  // std::iota


extern std::vector<Bitset> generar_F(int n, int n_min, int n_max,
                                     int tam_min, int tam_max, int seed);
extern Bitset generar_G(int n, int tam_min, int seed);

// -----------------------------------------------------------------------------
// Generar instancia “ground truth” coherente con el genético
// -----------------------------------------------------------------------------
GroundTruthInstance make_groundtruth(int n, int n_min, int n_max,
                                     int tam_min, int tam_max,
                                     int k, uint64_t seed_F, uint64_t seed_expr)
{
    // 1. Generar los conjuntos base F usando tu método existente
    std::vector<Bitset> F = generar_F(n, n_min, n_max, tam_min, tam_max, (int)seed_F);

    // 2. Crear una expresión aleatoria con tu propio generador del genético
    std::mt19937 rng(seed_expr);
    std::vector<int> indices(F.size());
    std::iota(indices.begin(), indices.end(), 0);

    Expression gold = build_random_expr(indices, F, k, rng);

    // 3. El resultado de esa expresión es el conjunto objetivo G
    Bitset G = gold.conjunto;

    // 4. Empaquetar todo
    GroundTruthInstance inst;
    inst.F = std::move(F);
    inst.G = std::move(G);
    inst.gold_expr = std::move(gold);
    inst.seed = seed_expr;

    return inst;
}
