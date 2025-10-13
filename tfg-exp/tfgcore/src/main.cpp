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

int main(int argc, char* argv[]) {
    int n= U_size; 
    int G_size_min= 10;
    int F_n_min= 10;
    int F_n_max= 100;
    int Fi_size_min= 1;
    int Fi_size_max= 64;
    int k= 3;
    int seed= 1002;
    bool ejecutar_exhaustiva= true;
    bool ejecutar_greedy= true;
    bool ejecutar_genetico= true;
    int pop_size= 200;
    double mutation_prob= 0.6;      
    double crossover_prob= 0.8;     
    int tournament_size= 2;         
    int max_generations= 1e9;       
    double time_limit= 150;         

    for (int i = 1; i < argc; i++) {
        string a = argv[i];

        if (a == "--G") G_size_min=stoi(argv[++i]);
        else if (a == "--Fmin") F_n_min=stoi(argv[++i]);
        else if (a == "--Fmax") F_n_max=stoi(argv[++i]);
        else if (a == "--FsizeMin") Fi_size_min=stoi(argv[++i]);
        else if (a == "--FsizeMax") Fi_size_max=stoi(argv[++i]);
        else if (a == "--k") k=stoi(argv[++i]);
        else if (a == "--seed") seed=stoi(argv[++i]);
        else if (a == "--pop_size") pop_size=stoi(argv[++i]);
        else if (a == "--mutation_prob") {mutation_prob=stod(argv[++i]);} // ahora en genetico.hpp
        else if (a == "--crossover_prob") {crossover_prob=stod(argv[++i]);} // ahora en genetico.hpp
        else if (a == "--tournament_size") {tournament_size=stoi(argv[++i]);} // ahora en genetico.hpp
        else if (a == "--max_generations") {max_generations=stoi(argv[++i]);} // ahora en genetico.hpp
        else if (a == "--time_limit") {time_limit=stoi(argv[++i]);} // ahora en genetico.hpp
        else if (a == "--algo") {
            string algo = argv[++i];
            ejecutar_exhaustiva = (algo == "exhaustiva" || algo == "all");
            ejecutar_greedy = (algo == "greedy" || algo == "all");
            ejecutar_genetico = (algo == "genetico" || algo == "all");
        }
        else {
            cerr << "Parámetro desconocido: " << a << endl;
            return 1;
        }
    }
    
    // Generar el univero U
    Bitset U;
    for (int i = 0; i < n; i++) U[i] = 1;
    
    // Generar G y F
    Bitset G = generar_G(U.size(), G_size_min, seed);
    vector<Bitset> F = generar_F(U.size(), F_n_min, F_n_max, Fi_size_min, Fi_size_max, seed);
    
    // Imprimir información de la instancia
    cout << "=== INSTANCIA ===" << endl;
    cout << "Semilla: " << seed << endl;
    cout << "U_size: " << n << endl;
    cout << "G_size: " << G.count() << endl;
    cout << "F_count: " << F.size() << endl;
    cout << "k: " << k << endl;
    cout << endl;

    vector<ResultadoAlgoritmo<SolMO>> resultados;

    if (ejecutar_exhaustiva) {
        // ========================= EXHAUSTIVA =========================
        cout << "=== EXHAUSTIVA ===" << endl;
        auto start = chrono::high_resolution_clock::now();
        auto expr = exhaustive_search(F, U, k);
        auto soluciones = evaluar_subconjuntos(expr, G, k);
        auto end = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        cout << "Tiempo_ejecucion_ms: " << duration << endl << endl;
        print_pareto_front(soluciones);

        resultados.push_back({"Exhaustiva", soluciones, duration /*, ...*/});
            
    } 
    
    if (ejecutar_greedy) {
        // ========================= GREEDY =========================
        cout << "=== GREEDY ===" << endl;
        auto start = chrono::high_resolution_clock::now();
        auto soluciones = greedy(F, G, k);
        auto end = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        cout << "Tiempo_ejecucion_ms: " << duration << endl << endl;
        print_pareto_front(soluciones);

        resultados.push_back({"Greedy", soluciones, duration /*, ...*/});
        
    }

    if (ejecutar_genetico) {
        // ========================= GENETICO =========================
        cout << "=== GENETICO (NSGA-II) ===" << endl;
        GAParams params;
        params.time_limit_sec = time_limit; // 5 minutos
        params.seed = random_device{}();
        params.population_size = pop_size;
        params.mutation_prob = mutation_prob;
        params.crossover_prob = crossover_prob;
        params.tournament_size = tournament_size;
        params.max_generations = max_generations; // Alto, pero el límite real es
        cout << "Semilla GA: " << params.seed << endl;
        cout << "Población: " << pop_size << endl;
        cout << "Límite tiempo (s): " << params.time_limit_sec << endl;
        cout << "Prob. mutación: " << params.mutation_prob << endl;
        cout << "Prob. cruce: " << params.crossover_prob << endl;
        cout << "Tamaño torneo: " << params.tournament_size << endl;
        cout << "Generaciones máximas: " << params.max_generations << endl;

        /*SPEA2_Params params;
        params.population_size = 600;
        params.archive_size = 600;
        params.time_limit_sec = 300; // 5 minutos
        params.pc = 0.85;
        params.pm = 0.35;
        params.tournament_k = 2;
        params.seed = random_device{}(); // aleatoria*/
        auto start = chrono::high_resolution_clock::now();
        auto soluciones = nsga2(F, G, k, params);
        auto end = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        cout << "Tiempo_ejecucion_ms: " << duration << endl << endl;
        print_pareto_front(soluciones);

        resultados.push_back({"Genético", individuos_a_solmos(soluciones), duration /*, ...*/});
        
    }

    // NUEVO: Sección de conjuntos para reproducibilidad
    cout << "=== CONJUNTOS ===" << endl;

    cout << "CONJUNTO_G: ";
    for (int i = 0; i < n; i++) {
        if (G[i]) cout << i << ",";
    }
    cout << endl;

    cout << "NUM_CONJUNTOS_F: " << F.size() << endl;
    for (size_t i = 0; i < F.size(); i++) {
        cout << "F" << i << ": ";
        for (int j = 0; j < n; j++) {
            if (F[i][j]) cout << j << ",";
        }
        cout << endl;
    }

    return 0;
}