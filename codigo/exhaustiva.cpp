#include <iostream>
#include <vector>
#include <bitset>
#include <string>
#include <chrono>
#include <random>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <set>

using namespace std;
using namespace chrono;

const int MAX_N = 1024;
using Bitset = bitset<MAX_N>;

/* OPERACIONES BITSET */
inline Bitset set_union(const Bitset& A, const Bitset& B) { return A | B; }
inline Bitset set_intersect(const Bitset& A, const Bitset& B) { return A & B; }
inline Bitset set_difference(const Bitset& A, const Bitset& B) { return A & (~B); }

/* ESTRUCTURA EXPRESIÓN */
struct Expression {
    Bitset conjunto;
    string expr_str;
    
    Expression(Bitset c, string s) : conjunto(c), expr_str(s) {}
};

/* MEDIDAS */
enum class Metric { JACCARD, PRECISION, SIZE, F1, ERROR };

static Metric parse_metric(string s) {
    for (auto& c : s) c = tolower(c);
    if (s == "jaccard") return Metric::JACCARD;
    if (s == "precision") return Metric::PRECISION;
    if (s == "size") return Metric::SIZE;
    if (s == "f1") return Metric::F1;
    if (s == "error") return Metric::ERROR;
    return Metric::JACCARD;
}

static const char* metric_name(Metric m) {
    switch(m){
        case Metric::JACCARD: return "jaccard";
        case Metric::PRECISION: return "precision";
        case Metric::SIZE: return "size";
        case Metric::F1: return "f1";
        case Metric::ERROR: return "error";
    }
    return "unknown";
}

inline double safe_div(double num, double den) {
    return den == 0.0 ? 0.0 : (num / den);
}

// Función de scoring
inline double M(const Bitset& A, const Bitset& G, int n, Metric metric) {
    size_t tp = (A & G).count();
    size_t a_size = A.count();
    size_t g_size = G.count();
    size_t fp = a_size - tp;
    size_t fn = g_size - tp;
    
    double dtp = (double)tp;
    double dfp = (double)fp;
    double dfn = (double)fn;

    switch (metric) {
        case Metric::JACCARD:
            return safe_div(dtp, dtp + dfp + dfn);
        case Metric::PRECISION:
            return safe_div(dtp, dtp + dfp);
        case Metric::SIZE:
            return -static_cast<double>(fn);
        case Metric::F1: {
            double prec = safe_div(dtp, dtp + dfp);
            double rec = safe_div(dtp, dtp + dfn);
            return safe_div(2.0 * prec * rec, prec + rec);
        }
        case Metric::ERROR:
            return safe_div(dfp + dfn, (double)n);
    }
    return 0.0;
}

/* CONVERSIÓN */
string bitset_to_str(const Bitset& bs, int n) {
    ostringstream oss;
    oss << "{";
    bool first = true;
    for (int i = 0; i < n; i++) {
        if (bs[i]) {
            if (!first) oss << ",";
            oss << i;
            first = false;
        }
    }
    oss << "}";
    return oss.str();
}

string family_to_str(const vector<Bitset>& F, int n) {
    ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < F.size(); ++i) {
        if (i > 0) oss << " ";
        oss << "F" << i << "=" << bitset_to_str(F[i], n);
    }
    oss << "]";
    return oss.str();
}

/* GENERACIÓN */
vector<Bitset> generar_F(int n, int num_subconjuntos, int tam_min, int tam_max, int seed=123) {
    vector<Bitset> F;
    mt19937 gen(seed);
    uniform_int_distribution<int> dist_tam(tam_min, tam_max);
    uniform_int_distribution<int> dist_idx(0, n - 1);
    
    int intentos = 0;
    int max_intentos = num_subconjuntos * 100;
    
    while (F.size() < num_subconjuntos && intentos < max_intentos) {
        Bitset subconjunto;
        int tam = dist_tam(gen);
        int count = 0;
        
        while (count < tam && count < n) {
            int idx = dist_idx(gen);
            if (!subconjunto[idx]) {
                subconjunto[idx] = 1;
                count++;
            }
        }
        
        if (count > 0) {
            bool duplicado = false;
            for (const auto& existing : F) {
                if (existing == subconjunto) {
                    duplicado = true;
                    break;
                }
            }
            if (!duplicado) F.push_back(subconjunto);
        }
        intentos++;
    }
    
    return F;
}

Bitset generar_G(int n, int tam, int seed=456) {
    Bitset G;
    mt19937 gen(seed);
    uniform_int_distribution<int> dist_idx(0, n - 1);
    
    int count = 0;
    while (count < tam) {
        int idx = dist_idx(gen);
        if (!G[idx]) {
            G[idx] = 1;
            count++;
        }
    }
    return G;
}

/* BÚSQUEDA EXHAUSTIVA */
vector<vector<Expression>> exhaustive_search(
    const vector<Bitset>& F,
    const Bitset& U,
    int n,
    int k)
{
    vector<vector<Expression>> expr(k + 1);
    
    // expr[0] = F ∪ {U}
    for (size_t i = 0; i < F.size(); i++) {
        expr[0].push_back(Expression(F[i], "F" + to_string(i)));
    }
    expr[0].push_back(Expression(U, "U"));
    
    // Generar expresiones con s operaciones
    for (int s = 1; s <= k; s++) {
        for (int op = 0; op < 3; op++) {
            string op_str = (op == 0) ? " ∪ " : (op == 1) ? " ∩ " : " \\ ";
            
            for (int a = 0; a < s; a++) {
                for (size_t i = 0; i < expr[a].size(); i++) {
                    for (size_t j = 0; j < expr[s - a - 1].size(); j++) {
                        Bitset new_set;
                        
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
                        
                        string new_expr = "(" + expr[a][i].expr_str + op_str + 
                                         expr[s-a-1][j].expr_str + ")";
                        
                        expr[s].push_back(Expression(new_set, new_expr));
                    }
                }
            }
        }
        cout << "Expresiones con " << s << " operación(es): " << expr[s].size() << endl;
    }
    
    // Añadir conjunto vacío a expr[0]
    expr[0].push_back(Expression(Bitset(), "∅"));
    
    return expr;
}

Expression evaluar_subconjuntos(
    const vector<vector<Expression>>& expr,
    const Bitset& G,
    int n,
    int k,
    Metric metric)
{
    double best_score = -99999;
    Expression e_best(Bitset(), "");

    for (int s = 0; s <= k; s++) {
        for (size_t e = 0; e < expr[s].size(); e++) {
            double score = M(expr[s][e].conjunto, G, n, metric);
            
            if (score > best_score) {
                best_score = score;
                e_best = expr[s][e];
            }
        }
    }
    
    return e_best;
}

int main(int argc, char* argv[]) {
    int k = 2;
    int n = 10;
    string txt_file;
    unsigned seed = 123;
    Metric metric = Metric::JACCARD;

    for (int i=1; i<argc; ++i){
        string a = argv[i];
        auto next = [&](){ return (i+1<argc) ? string(argv[++i]) : string(); };
        if (a == "--n") n = stoi(next());
        else if (a == "--k") k = stoi(next());
        else if (a == "--txt") txt_file = next();
        else if (a == "--seed") seed = (unsigned)stoul(next());
        else if (a == "--metric") metric = parse_metric(next());
        else if (a == "--help") {
            cerr << "Uso: ./prog [--n N] [--k K] [--txt file.txt] [--seed S] [--metric M]\n";
            cerr << "Máximo N=" << MAX_N << "\n";
            return 0;
        }
    }

    if (n > MAX_N) {
        cerr << "ERROR: n=" << n << " excede MAX_N=" << MAX_N << "\n";
        return 1;
    }

    // Generar universo U como bitset con primeros n bits en 1
    Bitset U;
    for (int i = 0; i < n; i++) U[i] = 1;
    
    // Generar F y G
    int num_subconjuntos = n;
    int tam_min = max(1, n/10);
    int tam_max = max(tam_min, n/3);
    vector<Bitset> F = generar_F(n, num_subconjuntos, tam_min, tam_max, seed);
    Bitset G = generar_G(n, n/2, seed + 1);
    
    // Ejecutar búsqueda exhaustiva
    auto t0 = high_resolution_clock::now();
    auto expresion = exhaustive_search(F, U, n, k);
    auto resultado = evaluar_subconjuntos(expresion, G, n, k, metric);
    auto t1 = high_resolution_clock::now();
    double ms = duration<double, milli>(t1 - t0).count();

    // SALIDA
    ostringstream line;
    line.setf(ios::fixed);
    line << setprecision(3);
    line << "n=" << n
         << "  k=" << k
         << "  |F|=" << F.size();
    
    if (n <= 20) {
        line << "  F=" << family_to_str(F, n)
             << "  |G|=" << G.count()
             << "  G=" << bitset_to_str(G, n);
    }
    
    line << "  time_ms=" << ms
         << "  metric=" << metric_name(metric)
         << "  best_score=" << M(resultado.conjunto, G, n, metric)
         << "  best_expr=" << resultado.expr_str;
    
    if (n <= 30) {
        line << "  best_set=" << bitset_to_str(resultado.conjunto, n);
    }
    
    line << "\n";

    if (!txt_file.empty()) {
        ofstream f(txt_file, ios::app);
        if (!f) {
            cerr << "ERROR: no se pudo abrir " << txt_file << "\n";
            return 1;
        }
        f << line.str();
    } else {
        cout << line.str();
    }

    return 0;
}