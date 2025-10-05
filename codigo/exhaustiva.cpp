#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <algorithm>
#include <chrono>
#include <random>
#include <fstream>
#include <sstream>
#include <filesystem>  // C++17
#include <iomanip>
#include <regex>
#include <cmath>

using namespace std;
using namespace chrono;

enum class Metric { JACCARD, PRECISION, SIZE, F1, ERROR };

static Metric parse_metric(std::string s) {
    for (auto& c : s) c = std::tolower(c);
    if (s == "jaccard") return Metric::JACCARD;
    if (s == "precision") return Metric::PRECISION;
    if (s == "size" || s == "|a|" || s == "tam") return Metric::SIZE;
    if (s == "f1") return Metric::F1;
    if (s == "error" || s == "err") return Metric::ERROR;
    return Metric::JACCARD; // por defecto
}

static inline double safe_div(double num, double den) {
    return den == 0.0 ? 0.0 : (num / den);
}

struct Confusion {
    double TP, FP, FN, TN;
    int A_size;
};

static Confusion compute_confusion(const std::set<int>& A,
                                   const std::set<int>& G,
                                   const std::set<int>& U) {
    size_t tp=0, fp=0, fn=0;
    for (int x : A) { if (G.count(x)) ++tp; else ++fp; }
    for (int x : G) { if (!A.count(x)) ++fn; }
    size_t a_size = A.size(), g_size = G.size(), u_size = U.size();
    size_t a_union_g = a_size + g_size - tp;
    size_t tn = (u_size >= a_union_g) ? (u_size - a_union_g) : 0;

    return { (double)tp, (double)fp, (double)fn, (double)tn, (int)a_size };
}

static const char* metric_name(Metric m) {
    switch(m){
        case Metric::JACCARD: return "jaccard";
        case Metric::PRECISION:  return "precision";
        case Metric::SIZE:    return "size";
        case Metric::F1: return "f1";
        case Metric::ERROR: return "error";
    }
    return "unknown";
}

template <typename T>
std::string set_to_str(const std::set<T>& s) {
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& x : s) {
        if (!first) oss << ",";
        oss << x;
        first = false;
    }
    oss << "}";
    return oss.str();
}

std::string family_to_str(const std::vector<std::set<int>>& F) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < F.size(); ++i) {
        if (i > 0) oss << " ";
        oss << "F" << i << "=" << set_to_str(F[i]);
    }
    oss << "]";
    return oss.str();
}

// Función para generar familia F de subconjuntos aleatorios
vector<set<int>> generar_F(const set<int>& U, int num_subconjuntos, int tam_min, int tam_max, int seed=123) {
    vector<set<int>> F_final;
    set<set<int>> F; // Para evitar subconjuntos duplicados
    vector<int> universo(U.begin(), U.end());
    
    mt19937 gen(seed);
    uniform_int_distribution<int> dist_tam(tam_min, tam_max);
    uniform_int_distribution<int> dist_idx(0, universo.size() - 1);
    
    // Generamos 'num_subconjuntos' subconjuntos aleatorios
    for (int i = 0; i < num_subconjuntos; i++) {
        set<int> subconjunto;
        int tam = dist_tam(gen);
        
        // Generar subconjunto único de tamaño 'tam' 
        while (subconjunto.size() < tam) {
            subconjunto.insert(universo[dist_idx(gen)]);
        }
        
        // Insertar solo si no está ya en F
        F.insert(subconjunto);
    }
    F_final = vector<set<int>>(F.begin(), F.end());
    return F_final;
}

// Función para generar conjunto objetivo G
set<int> generar_G(const set<int>& U, int tam, int seed=456) {
    set<int> G;
    vector<int> universo(U.begin(), U.end());
    
    mt19937 gen(seed);
    uniform_int_distribution<int> dist_idx(0, universo.size() - 1);
    
    // Generar 'tam' elementos aleatorios
    while (G.size() < tam) {
        G.insert(universo[dist_idx(gen)]);
    }
    
    return G;
}

// Estructura para guardar conjunto + expresión que lo generó
struct Expression {
    set<int> conjunto;      // El conjunto resultante (ordenado para eficiencia)
    string expr_str;        // La expresión como string (para mostrar)
    
    Expression(set<int> c, string s) : conjunto(c), expr_str(s) {}
};

// Operaciones de conjuntos (optimizadas para set ordenado)
set<int> set_union(const set<int>& A, const set<int>& B) {
    set<int> result;
    set_union(A.begin(), A.end(), B.begin(), B.end(), 
              inserter(result, result.begin()));
    return result;
}

set<int> set_intersect(const set<int>& A, const set<int>& B) {
    set<int> result;
    set_intersection(A.begin(), A.end(), B.begin(), B.end(), 
                     inserter(result, result.begin()));
    return result;
}

set<int> set_difference(const set<int>& A, const set<int>& B) {
    set<int> result;
    set_difference(A.begin(), A.end(), B.begin(), B.end(), 
                   inserter(result, result.begin()));
    return result;
}

// Función de medida - IMPLEMENTAR LA QUE QUERAMOS
double M(const Expression& e, const std::set<int>& G, const std::set<int>& U, Metric metric) {
    const Confusion c = compute_confusion(e.conjunto, G, U);
    const double TP = c.TP, FP = c.FP, FN = c.FN, TN = c.TN;

    switch (metric) {
        case Metric::JACCARD:
            return safe_div(TP, TP + FP + FN);           // |A∩G| / |A∪G|
        case Metric::PRECISION:
            return safe_div(TP, TP + FP);                // |A∩G| / |G|
        case Metric::SIZE:
            return -static_cast<double>(set_difference(G, e.conjunto).size());       // minimizar |G\A| -> maximizamos -|G\A|
        case Metric::F1: {
            double prec = safe_div(double(TP), double(TP) + double(FP));
            double rec = safe_div(double(TP), double(TP) + double(FN));
            return safe_div(2.0 * prec * rec, prec + rec);
        }
        case Metric::ERROR:
            return safe_div(FP + FN, U.size());        // (|A∩G| + |A^c ∩ G^c|) / |U|
     }
    return 0.0;
}


vector<vector<Expression>> exhaustive_search(
    const vector<set<int>>& F,  // Familia de subconjuntos
    const set<int>& U,           // Universo
    const set<int>& G,           // Conjunto objetivo
    int k                       // Máximo de operaciones
    )
{
    // Inicializar expr[s] para s = 0 hasta k
    vector<vector<Expression>> expr(k + 1);
    
    // expr[0] = F ∪ {U} (expresiones base sin operaciones)
    for (int i = 0; i < F.size(); i++) {
        expr[0].push_back(Expression(F[i], "F" + to_string(i)));
    }
    expr[0].push_back(Expression(U, "U"));
    
    // Generar expresiones con s operaciones
    for (int s = 1; s <= k; s++) {
        // Para cada operación
        for (int op = 0; op < 3; op++) {  // 0:∪, 1:∩, 2:
            string op_str = (op == 0) ? " ∪ " : (op == 1) ? " ∩ " : " \\ ";
            
            // Para cada partición de s operaciones
            for (int a = 0; a < s; a++) {
                // Para cada par de expresiones
                for (int i = 0; i < expr[a].size(); i++) {
                    for (int j = 0; j < expr[s - a - 1].size(); j++) {
                        
                        set<int> new_set;
                        // Aplicar la operación sobre los conjuntos
                        if (op == 0) {
                            new_set = set_union(expr[a][i].conjunto, 
                                               expr[s-a-1][j].conjunto);
                        } else if (op == 1) {
                            new_set = set_intersect(expr[a][i].conjunto, 
                                                   expr[s-a-1][j].conjunto);
                        } else {
                            new_set = set_difference(expr[a][i].conjunto, 
                                                    expr[s-a-1][j].conjunto);
                        }
                        
                        // Crear string de la expresión
                        string new_expr = "(" + expr[a][i].expr_str + op_str + 
                                         expr[s-a-1][j].expr_str + ")";
                        
                        expr[s].push_back(Expression(new_set, new_expr));
                    }
                }
            }
        }
        cout << "Expresiones con " << s << " operación(es): " 
             << expr[s].size() << endl;
    }
    
    // Añadir conjunto vacío a expr[0]
    expr[0].push_back(Expression(set<int>(), "∅"));
    
    return expr;
}

Expression evaluar_subconjuntos(const vector<vector<Expression>>& expr, 
                                                 const set<int>& G, 
                                                 const set<int>& U, 
                                                 int k, 
                                                 Metric metric) {
    // Variables para el mejor resultado
    double best_score = -99999;
    Expression e_best(set<int>(), "");

    for (int s = 0; s <= k; s++) {
        for (int e = 0; e < expr[s].size(); e++) {
            double score = M(expr[s][e], G, U, metric);
            
            if (score > best_score) {
                best_score = score;
                e_best = expr[s][e];
            }
        }
    }
        
    return e_best;
}


int main(int argc, char* argv[]) {
    // ---- DEFAULTS ----
    int k = 2; // Número máximo de operaciones 
    int n = 10; // Tamaño del universo 
    string txt_file; 
    unsigned seed = 123;
    Metric metric = Metric::JACCARD;

    // ---- PARSE RÁPIDO ----
    for (int i=1; i<argc; ++i){
        string a = argv[i];
        auto next = [&](){ return (i+1<argc) ? string(argv[++i]) : string(); };
        if (a == "--n")        n = stoi(next());
        else if (a == "--k")   k = stoi(next());
        else if (a == "--txt") txt_file = next();
        else if (a == "--seed") seed = (unsigned)stoul(next());
        else if (a == "--help") {
            cerr << "Uso: ./prog [--n N] [--k K] [--txt fichero.txt] [--seed S]\n";
            return 0;
        }
        else if (a == "--metric") metric = parse_metric(next());
    }

    // Generar universo U = {1, 2, ..., n}
    set<int> U;
    for (int i = 1; i <= n; i++) {
        U.insert(i);
    }
    
    // Generar familia F de subconjuntos aleatorios
    int num_subconjuntos = n;  // Número de subconjuntos en F
    int tam_min = max(1, n/10);           // Tamaño mínimo de cada subconjunto
    int tam_max = n / 3;       // Tamaño máximo de cada subconjunto
    vector<set<int>> F = generar_F(U, num_subconjuntos, tam_min, tam_max, seed);
    
    // Generar conjunto objetivo G
    set<int> G = generar_G(U, n/2, seed + 1);
    
    // ---- Ejecutar para este k (una vez) ----
    auto t0 = chrono::high_resolution_clock::now();
    auto expresion = exhaustive_search(F, U, G, k);
    auto resultado = evaluar_subconjuntos(expresion, G, U, k, metric);
    auto t1 = chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    const string& e_best = resultado.expr_str;
    const set<int>& G_best = resultado.conjunto;

    // ---- SALIDA TXT (si se indica) o stdout ----
    double best_score = M(resultado, G, U, metric);

    // Construir la línea de resultados
    std::ostringstream line;
    line.setf(std::ios::fixed);
    line << std::setprecision(3);
    line << "n=" << n
        << "  k=" << k
        << "  |F|=" << F.size()
        << "  F=" << family_to_str(F)
        << "  |G|=" << G.size()
        << "  G=" << set_to_str(G)
        << "  time_ms=" << ms
        << "  metric=" << metric_name(metric)
        << "  best_score=" << std::setprecision(3) << best_score
        << "  best_expr=" << e_best
        << "  best_set=" << set_to_str(G_best)
        << "\n"; 

    // Si se pasa --txt, añadir al archivo
    if (!txt_file.empty()) {  
        std::ofstream f(txt_file, std::ios::app);
        if (!f) {
            std::cerr << "ERROR: no se pudo abrir " << txt_file << " para escribir.\n";
            return 1;
        }
        f << line.str();
    } else {
        // Si no se pasa, imprimir por pantalla
        std::cout << line.str();
    }

    return 0;

}