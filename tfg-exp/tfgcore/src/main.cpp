#include <iostream>
#include <vector>
#include <string>
#include "domain.hpp"
#include "generator.hpp"
#include "metrics.hpp"
#include <chrono>
#include "exhaustiva.hpp"
#include "solutions.hpp"
#include "greedy.hpp"
#include "genetico.hpp"
#include "spea2.hpp"
#include "ground_truth.hpp"

using namespace std;

template<typename T>
struct ResultadoAlgoritmo {
    string nombre;
    vector<T> soluciones;
    long long tiempo_ms;
    /*double Jaccard_promedio;
    double SizeH_promedio;
    double OpSize_promedio;*/
};

inline std::vector<SolMO> individuos_a_solmos(const std::vector<Individuo>& v) {
    std::vector<SolMO> r; r.reserve(v.size());
    for (const auto& x : v) r.emplace_back(x.expr, x.n_ops, x.sizeH, x.jaccard);
    return r;
}


static void print_set_indices(const Bitset& B, int U=128, int max_show=40) {
    int shown = 0;
    for (int i = 0; i < U; ++i) {
        if (B[i]) {
            cout << i << ",";
            if (++shown >= max_show) { cout << " ..."; break; }
            cout << " ";
        }
    }
    cout << "\n";
}

int main(int argc, char** argv) {
    bool modo_test = false;
    int seed_expr = 20251019;
    if (argc >= 2) {
        string arg1 = argv[1];
        std::transform(arg1.begin(), arg1.end(), arg1.begin(), ::toupper);
        if (arg1 == "TRUE" || arg1 == "1" || arg1 == "ON") modo_test = true;
    }
    if (argc >= 3) {
        seed_expr = std::stoi(argv[2]);
    }

    if (!modo_test){
        auto gt = make_groundtruth(128, 5, 100, 5, 100, 10, 123, 20251019);

        cout << "=== INSTANCIA ===\n";
        cout << "Semilla: " << seed_expr << "\n";
        cout << "U_size: 128\n";
        cout << "F_count: " << gt.F.size() << "\n";
        cout << "k: 10\n";
        cout << "Expresion_oro: " << gt.gold_expr.expr_str << "\n";
        cout << "Jaccard_objetivo: " << M(gt.gold_expr, gt.G, Metric::Jaccard) << "\n";
        cout << "G_indices: ";
        print_set_indices(gt.G, 128, 64);
        cout << "\n";

        GAParams ga_params;
        ga_params.population_size   = 600;
        ga_params.max_generations   = 1e9;
        ga_params.time_limit_sec    = 600;
        ga_params.crossover_prob    = 0.8;
        ga_params.mutation_prob     = 0.6;
        ga_params.tournament_size   = 2;
        ga_params.seed              = 0;

        auto t0 = chrono::steady_clock::now();
        auto pareto = nsga2(gt.F, gt.G, 10, ga_params);
        auto t1 = chrono::steady_clock::now();
        auto dur_ms = chrono::duration_cast<chrono::milliseconds>(t1 - t0).count();

        cout << "=== GENÉTICO (NSGA-II) ===\n";
        cout << "Tiempo (ms): " << dur_ms << "\n";
        cout << "Población: " << ga_params.population_size
                << " | Limite_tiempo_s: " << ga_params.time_limit_sec
                << " | p_mut: " << ga_params.mutation_prob
                << " | p_cruce: " << ga_params.crossover_prob
                << " | torneo: " << ga_params.tournament_size << "\n\n"; 

        print_pareto_front(pareto);
        bool hit = any_of(pareto.begin(), pareto.end(), [](const auto& s) {
            return s.jaccard == 1.0; });
        cout << "HIT_OBJETIVO: " << (hit ? "SI" : "NO") << "\n";
    }
    return 0;

}