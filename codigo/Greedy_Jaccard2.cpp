#include <iostream>
#include <vector>
#include <algorithm>
//#include <set>
#include <unordered_set>
#include <random>
#include <omp.h>

using namespace std;

// Los conjuntos van a ser sets de enteros
using Conjunto = unordered_set<int>;

vector<Conjunto> generar_subconjuntos(int m, const Conjunto& U){
    vector<Conjunto> F(m);
    vector<int> universo(U.begin(), U.end());  // Convertimos U a un vector para selección rápida
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> dis(0, universo.size() - 1);
    for (int i = 0; i < m; i++) {
        int tam = dis(gen) % U.size() + 1;
        while(F[i].size() < tam) {
            F[i].insert(universo[dis(gen)]);
        }
    }
    return F;
}

Conjunto unir(const Conjunto& a, const Conjunto& b) {
    Conjunto res;
    // Iteramos sobre los elementos de a y b
    for (int elem : a) {
        res.insert(elem);
    }
    for (int elem : b) {
        res.insert(elem);
    }
    return res;
}

Conjunto intersectar(const Conjunto& a, const Conjunto& b) {
    Conjunto res;
    // Iteramos sobre los elementos de a
    for (int elem : a) {
        // Si el elemento está en b, lo agregamos al resultado
        if (b.count(elem) > 0) {
            res.insert(elem);
        }
    }
    return res;
}

Conjunto restar(const Conjunto& a, const Conjunto& b) {
    Conjunto res;
    // Iteramos sobre los elementos de a
    for (int elem : a) {
        // Si el elemento no está en b, lo agregamos al resultado
        if (b.count(elem) == 0) {
            res.insert(elem);
        }
    }
    return res;
}

/*Conjunto complementar(const Conjunto& a, int n) {
    Conjunto res;
    // Iteramos sobre los elementos de 0 a n-1
    for (int i = 0; i < n; i++) {
        // Si el elemento no está en a, lo agregamos al resultado
        if (a.count(i) == 0) {
            res.insert(i);
        }
    }
    return res;
}*/

Conjunto leerConjunto(int n) {
    Conjunto res;
    // Leemos los elementos del conjunto
    for (int i = 0; i < n; i++) {
        int elem;
        cin >> elem;
        res.insert(elem);
    }
    return res;
}

// Función para calcular el índice de Jaccard
double jaccard_index(const Conjunto& A, const Conjunto& B) {
    int interseccion = 0, union_ = A.size();
    for (int x : B) {
        if (A.count(x)) interseccion++;
        else union_++;
    }
    return (union_ == 0) ? 0.0 : (double)interseccion / union_;
}

Conjunto greedy_jaccard(const int k, const vector<Conjunto>& F, const Conjunto& G) {
    int count = 0;
    Conjunto Covered;
    // vector<bool> used(F.size(), false); // Descomentar si no queremos repetir elementos
    
    double best_jaccard = jaccard_index(Covered, G);
    double maxJaccard=0.0;

    vector<Conjunto (*)(const Conjunto&, const Conjunto&)> ops = {unir, intersectar, restar};

    while (maxJaccard<1.0 - 1e-6 && count<k){
        int bestIndex = -1;
        int bestOp = -1;
        maxJaccard = best_jaccard;
        Conjunto bestSet;
        
        #pragma omp parallel
        {
            int localBestIndex = -1;
            int localBestOp = -1;
            double localMaxJaccard = best_jaccard;
            Conjunto localBestSet;

            #pragma omp for collapse(2) nowait
            for (size_t i = 0; i < F.size(); i++){
                for (size_t j = 0; j < ops.size(); j++){
                    Conjunto newSet = ops[j](Covered, F[i]);
                    double newJaccard = jaccard_index(newSet, G);
                    
                    if (newJaccard > localMaxJaccard){
                        localMaxJaccard = newJaccard;
                        localBestIndex = i;
                        localBestOp = j;
                        localBestSet= newSet;
                    }
                }
            }
            #pragma omp critical
            {
                if (localMaxJaccard > maxJaccard){
                    maxJaccard = localMaxJaccard;
                    bestIndex = localBestIndex;
                    bestOp = localBestOp;
                    bestSet = localBestSet;
                }
            }
        }
        if (bestIndex == -1){
            break;
        }
        Covered = bestSet;
        best_jaccard = maxJaccard;
        count++;
        
        // Mostrar información de la iteración
        cout << "Iteración " << count << ": Añadimos F[" << bestIndex << "] con operación " << bestOp << endl;
        cout << "Nuevo índice de Jaccard: " << best_jaccard << endl;
    }

    return Covered;
}

Conjunto greedy_jaccard_parejas(int k, const vector<Conjunto>& F, const Conjunto& G) {
    int count = 0;
    Conjunto Covered;
    double best_jaccard = jaccard_index(Covered, G);
    double maxJaccard = 0.0;

    // Operaciones posibles
    vector<Conjunto (*)(const Conjunto&, const Conjunto&)> ops = {unir, intersectar, restar};

    while (maxJaccard < 1.0 - 1e-6 && count < k) {
        int best_i = -1, best_j = -1, best_op = -1;
        maxJaccard = best_jaccard;
        Conjunto bestSet;

        #pragma omp parallel
        {
            int local_best_i = -1, local_best_j = -1, local_best_op = -1;
            double local_maxJaccard = best_jaccard;
            Conjunto local_bestSet;

            #pragma omp for nowait
            for (size_t i = 0; i < F.size(); i++) {
                for (size_t j = i; j < F.size(); j++) {  // Evitamos repeticiones
                    for (size_t op = 0; op < ops.size(); op++) {
                        Conjunto newSet = ops[op](F[i], F[j]);  // Operación entre F[i] y F[j]
                        Conjunto candidate = unir(Covered, newSet);  // Unimos con Covered
                        double newJaccard = jaccard_index(candidate, G);

                        if (newJaccard > local_maxJaccard) {
                            local_maxJaccard = newJaccard;
                            local_best_i = i;
                            local_best_j = j;
                            local_best_op = op;
                            local_bestSet = candidate;
                        }
                    }
                }
            }

            #pragma omp critical
            {
                if (local_maxJaccard > maxJaccard) {
                    maxJaccard = local_maxJaccard;
                    best_i = local_best_i;
                    best_j = local_best_j;
                    best_op = local_best_op;
                    bestSet = local_bestSet;
                }
            }
        }

        if (best_i == -1) {
            break;
        }

        Covered = bestSet;
        best_jaccard = maxJaccard;
        count++;

        // Mostrar información de la iteración
        cout << "Iteración " << count << ": Combinamos F[" << best_i << "] y F[" << best_j << "] con operación " << best_op << endl;
        cout << "Nuevo índice de Jaccard: " << best_jaccard << endl;
    }

    return Covered;
}

double cobertura_relativa(const Conjunto& Covered, const Conjunto& G, const Conjunto& nuevo) {
    int nuevos_en_G = 0, nuevos_totales = 0;
    for (int x : nuevo) {
        if (!Covered.count(x)) {
            nuevos_totales++;
            if (G.count(x)) nuevos_en_G++;
        }
    }
    return (nuevos_totales == 0) ? 0.0 : (double)nuevos_en_G / nuevos_totales;
}

Conjunto greedy_jaccard_2(int k, const vector<Conjunto>& F, const Conjunto& G) {
    Conjunto Covered;
    int count = 0;
    double best_ratio;

    while (count < k) {
        int bestIndex = -1;
        best_ratio = 0.0;
        Conjunto bestSet;
        
        #pragma omp parallel
        {
            int localBestIndex = -1;
            double localBestRatio = 0.0;
            Conjunto localBestSet;

            #pragma omp for nowait
            for (size_t i = 0; i < F.size(); i++) {
                Conjunto newSet = unir(Covered, F[i]);
                double ratio = cobertura_relativa(Covered, G, F[i]);
                
                if (ratio > localBestRatio) {
                    localBestRatio = ratio;
                    localBestIndex = i;
                    localBestSet = newSet;
                }
            }
            
            #pragma omp critical
            {
                if (localBestRatio > best_ratio) {
                    best_ratio = localBestRatio;
                    bestIndex = localBestIndex;
                    bestSet = localBestSet;
                }
            }
        }

        if (bestIndex == -1) break;
        Covered = bestSet;
        count++;
        
        cout << "Iteración " << count << ": Añadimos F[" << bestIndex << "] con ratio " << best_ratio << endl;
    }
    return Covered;
}

double calcular_costo(const Conjunto& E, const Conjunto& G, double alpha, double beta) {
    int excess = 0;  // Elementos en E pero no en G
    int defect = 0;  // Elementos en G pero no en E

    for (int elem : E) {
        if (G.find(elem) == G.end()) excess++;
    }
    for (int elem : G) {
        if (E.find(elem) == E.end()) defect++;
    }

    return alpha * excess + beta * defect;
}

Conjunto greedy_costo_parejas(int k, const vector<Conjunto>& F, const Conjunto& G, 
    double alpha = 1.0, double beta = 1.0) {
    int count = 0;
    Conjunto Covered;
    double best_cost = calcular_costo(Covered, G, alpha, beta);
    double minCost = best_cost;

    // Operaciones posibles
    vector<Conjunto (*)(const Conjunto&, const Conjunto&)> ops = {unir, intersectar, restar};

    while (minCost > 1e-6 && count < k) {  // Queremos minimizar el costo (hasta ~0)
        int best_i = -1, best_j = -1, best_op = -1;
        minCost = best_cost;
        Conjunto bestSet;

        #pragma omp parallel
        {
            int local_best_i = -1, local_best_j = -1, local_best_op = -1;
            double local_minCost = best_cost;
            Conjunto local_bestSet;

            #pragma omp for nowait
            for (size_t i = 0; i < F.size(); i++) {
                for (size_t j = i; j < F.size(); j++) {
                    for (size_t op = 0; op < ops.size(); op++) {
                        Conjunto newSet = ops[op](F[i], F[j]);
                        Conjunto candidate = unir(Covered, newSet);
                        double newCost = calcular_costo(candidate, G, alpha, beta);

                        if (newCost < local_minCost) {
                            local_minCost = newCost;
                            local_best_i = i;
                            local_best_j = j;
                            local_best_op = op;
                            local_bestSet = candidate;
                        }
                    }
                }
            }

            #pragma omp critical
            {
                if (local_minCost < minCost) {
                    minCost = local_minCost;
                    best_i = local_best_i;
                    best_j = local_best_j;
                    best_op = local_best_op;
                    bestSet = local_bestSet;
                }
            }
        }

        if (best_i == -1) break;

        Covered = bestSet;
        best_cost = minCost;
        count++;

        cout << "Iteración " << count << ": Combinamos F[" << best_i 
        << "] y F[" << best_j << "] con operación " << best_op 
        << "\nNuevo costo: " << best_cost 
        << " (Exceso: " << alpha*(best_cost - beta*(G.size() - Covered.size()))/alpha
        << ", Defecto: " << beta*(G.size() - Covered.size())/beta << ")\n";
    }

    return Covered;
}

Conjunto greedy_costo(const int k, const vector<Conjunto>& F, const Conjunto& G, 
    double alpha = 1.0, double beta = 1.0) {
    int count = 0;
    Conjunto Covered;
    double best_cost = calcular_costo(Covered, G, alpha, beta);
    double minCost = best_cost;

    vector<Conjunto (*)(const Conjunto&, const Conjunto&)> ops = {unir, intersectar, restar};

    while (minCost > 1e-6 && count < k) {  // Continuar mientras el costo sea significativo
        int bestIndex = -1;
        int bestOp = -1;
        minCost = best_cost;
        Conjunto bestSet;

        #pragma omp parallel
        {
            int localBestIndex = -1;
            int localBestOp = -1;
            double localMinCost = best_cost;
            Conjunto localBestSet;

            #pragma omp for collapse(2) nowait
            for (size_t i = 0; i < F.size(); i++) {
                for (size_t j = 0; j < ops.size(); j++) {
                    Conjunto newSet = ops[j](Covered, F[i]);
                    double current_cost = calcular_costo(newSet, G, alpha, beta);
                    
                    if (current_cost < localMinCost) {
                        localMinCost = current_cost;
                        localBestIndex = i;
                        localBestOp = j;
                        localBestSet = newSet;
                    }
                }
            }

            #pragma omp critical
            {
                if (localMinCost < minCost) {
                    minCost = localMinCost;
                    bestIndex = localBestIndex;
                    bestOp = localBestOp;
                    bestSet = localBestSet;
                }
            }
        }

        if (bestIndex == -1) {
            break;
        }

        Covered = bestSet;
        best_cost = minCost;
        count++;

        // Calcular métricas para mostrar
        int excess = 0, defect = 0;
        for (const auto& elem : Covered) {
            if (G.find(elem) == G.end()) excess++;
        }
        for (const auto& elem : G) {
            if (Covered.find(elem) == Covered.end()) defect++;
        }

        cout << "Iteración " << count << ": Operación " << bestOp 
        << " con F[" << bestIndex << "]\n"
        << "Costo total: " << best_cost 
        << " (Exceso: " << excess << ", Defecto: " << defect << ")\n";
    }

    return Covered;
}

int main() {
    omp_set_num_threads(4);

    // Universo U
    int n = 10000;
    int m = 200;
    int k = 50;

    Conjunto U;
    for (int i = 1; i <= n; i++) U.insert(i);

    // Familia de subconjuntos F
    vector<Conjunto> F = generar_subconjuntos(m,U);

    // Conjunto objetivo G
    Conjunto G = generar_subconjuntos(1,U)[0];

    vector<Conjunto> F_util;
    for (const auto& f : F) {
        if (!f.empty() && !f.count(*G.begin())) { // Solo dejamos subconjuntos con elementos de G
            F_util.push_back(f);
        }
    }
    F = F_util; // Reemplazamos F con la versión filtrada

    // Conjunto result = greedy_jaccard_parejas(k, F, G);
    // cout << "Índice de Jaccard final: " << jaccard_index(result, G) << endl;

    cout << "Total elementos U: " << U.size() << endl;
    cout << "Total elementos G: " << G.size() << endl;
    cout << "Total subconjuntos F: " << F.size() << endl;

    cout << "Greedy_costo_parejas" << endl;
    Conjunto result = greedy_costo_parejas(k, F, G, 1.0, 1.0); // 0.22
    cout << "Índice de Jaccard final: " << jaccard_index(result, G) << endl;

    cout << "Greedy_costo" << endl;
    result = greedy_costo(k, F, G, 1.0, 1.0); // 0.08
    cout << "Índice de Jaccard final: " << jaccard_index(result, G) << endl;

    cout << "Greedy_costo_parejas" << endl;
    result = greedy_costo_parejas(k, F, G, 2.0, 1.0);
    cout << "Índice de Jaccard final: " << jaccard_index(result, G) << endl;

    cout << "Greedy_costo" << endl;
    result = greedy_costo(k, F, G, 2.0, 1.0);
    cout << "Índice de Jaccard final: " << jaccard_index(result, G) << endl;

    cout << "Greedy_costo_parejas" << endl;
    result = greedy_costo_parejas(k, F, G, 1.5, 1.0);
    cout << "Índice de Jaccard final: " << jaccard_index(result, G) << endl;

    cout << "Greedy_costo" << endl;
    result = greedy_costo(k, F, G, 1.5, 1.0);
    cout << "Índice de Jaccard final: " << jaccard_index(result, G) << endl;

    return 0;
}