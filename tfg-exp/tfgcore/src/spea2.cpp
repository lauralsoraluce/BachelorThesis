#include "spea2.hpp"

#include <algorithm>
#include <cmath>
#include <chrono>
#include <numeric>
#include <unordered_set>
#include <iostream>
#include <set>

using namespace std;

// -------------------- Helpers locales --------------------
static inline std::string key_of_expr(const Expression& e){ return e.expr_str; }
static inline std::string key_of_H(const Expression& e){ return e.conjunto.to_string(); }

template <typename T>
static std::vector<T> unique_by_H(const std::vector<T>& v){
    std::vector<T> out; out.reserve(v.size());
    std::unordered_set<std::string> seen; seen.reserve(v.size()*2);
    for (const auto& x : v) if (seen.insert(key_of_H(x.expr)).second) out.push_back(x);
    return out;
}

static void print_population_summary(const std::vector<Individuo>& P){
    std::unordered_set<std::string> uniq_expr, uniq_H;
    std::vector<int> ops_hist(16,0); // hasta 15 ops por si acaso

    double minJ=1e300, maxJ=-1e300, sumJ=0.0;
    int minH=30000, maxH=-1, sumH=0;

    for (const auto& x : P){
        uniq_expr.insert(x.expr.expr_str);
        uniq_H.insert(x.expr.conjunto.to_string());
        int o = std::min( (int)ops_hist.size()-1, x.n_ops );
        ops_hist[o]++;

        minJ = std::min(minJ, x.jaccard);
        maxJ = std::max(maxJ, x.jaccard);
        sumJ += x.jaccard;

        minH = std::min(minH, x.sizeH);
        maxH = std::max(maxH, x.sizeH);
        sumH += x.sizeH;
    }
    double avgJ = sumJ / std::max<size_t>(1,P.size());
    double avgH = (double)sumH / std::max<size_t>(1,P.size());

    std::cout << "[INIT] N=" << P.size()
              << " | uniq_expr=" << uniq_expr.size()
              << " | uniq_H="    << uniq_H.size()
              << " | Jaccard[min,avg,max]=[" << minJ << "," << avgJ << "," << maxJ << "]"
              << " | |H|[min,avg,max]=[" << minH << "," << avgH << "," << maxH << "]\n";

    std::cout << "[INIT] hist n_ops:";
    for (size_t i=0;i<ops_hist.size();++i){
        if (ops_hist[i]) std::cout << " " << i << ":" << ops_hist[i];
    }
    std::cout << "\n";

    
    for (size_t i=0; i<std::min<size_t>(20, P.size()); ++i){
        const auto& s = P[i];
        std::cout << "  [" << i << "] ops=" << s.n_ops
                  << " |H|=" << s.sizeH
                  << " J=" << s.jaccard
                  << " :: " << s.expr.expr_str << "\n";
    }
}


// ---------------------------------------------------------------------------
// build_random_expr LOCAL (independiente del genetico.cpp)
//   - Marcada como static para linkage interno (no colisiona con nada externo)
// ---------------------------------------------------------------------------
static Expression build_random_expr(const std::vector<int>& conjs,
                                    const std::vector<Bitset>& F, int k, std::mt19937& rng)
{
    if (conjs.empty()) return Expression(Bitset(), "∅", {}, 0);

    struct Node { Expression e; };
    std::vector<Node> pool; pool.reserve(conjs.size());

    // hojas
    for (int idx : conjs)
        pool.push_back({ Expression(F[idx], "F" + std::to_string(idx), {idx}, 0) });

    // combina subárboles aleatoriamente mientras haya margen
    while (pool.size() > 1) {
        std::uniform_int_distribution<> pick(0, (int)pool.size() - 1);
        int a = pick(rng), b = pick(rng);
        if (a == b) continue;
        if (a > b) std::swap(a, b);

        int op = std::uniform_int_distribution<>(0, 2)(rng); 
        
        const char* op_str = (op == 0 ? " ∪ " : (op == 1 ? " ∩ " : " \\ "));

        Bitset H;
        if (op == 0) H = set_union   (pool[a].e.conjunto, pool[b].e.conjunto);
        else if (op == 1) H = set_intersect(pool[a].e.conjunto, pool[b].e.conjunto);
        else              H = set_difference(pool[a].e.conjunto, pool[b].e.conjunto);

        int ops_new = pool[a].e.n_ops + pool[b].e.n_ops + 1;
        if (ops_new > k) break; // no exceder k

        std::set<int> usados = pool[a].e.used_sets;
        usados.insert(pool[b].e.used_sets.begin(), pool[b].e.used_sets.end());
        std::string expr_str = "(" + pool[a].e.expr_str + op_str + pool[b].e.expr_str + ")";

        pool[a].e = Expression(H, expr_str, usados, ops_new);
        pool.erase(pool.begin() + b);
    }
    return pool.front().e;
}

// -------------------- Distancias para densidad SPEA2 --------------------
static std::vector<std::vector<double>> distancia_kNN(const std::vector<Individuo>& X){
    const int n = (int)X.size();
    std::vector<double> f0(n), f1(n), f2(n);
    for (int i=0;i<n;++i){ f0[i]=X[i].jaccard; f1[i]=(double)X[i].sizeH; f2[i]=(double)X[i].n_ops; }

    auto mm = [](const std::vector<double>& v){
        auto [minIt, maxIt] = std::minmax_element(v.begin(), v.end());
        double lo = *minIt, span = *maxIt - *minIt;
        if (span == 0) span = 1;
        return std::pair<double,double>(lo, span);
    };
    auto [m0,d0] = mm(f0); auto [m1,d1] = mm(f1); auto [m2,d2] = mm(f2);

    std::vector<std::vector<double>> D(n, std::vector<double>(n, 0.0));
    for (int i=0;i<n;++i){
        for (int j=i+1;j<n;++j){
            double a0=(f0[i]-m0)/d0, b0=(f0[j]-m0)/d0; // Jaccard (max)
            double a1=(f1[i]-m1)/d1, b1=(f1[j]-m1)/d1; // |H| (min)
            double a2=(f2[i]-m2)/d2, b2=(f2[j]-m2)/d2; // n_ops (min)
            double dij = std::sqrt((a0-b0)*(a0-b0) + (a1-b1)*(a1-b1) + (a2-b2)*(a2-b2));
            D[i][j]=D[j][i]=dij;
        }
    }
    return D;
}

// -------------------- Fitness SPEA2 (fuerza + densidad) --------------------
static void fitness_spea2(const std::vector<Individuo>& U, std::vector<double>& fit){
    const int n = (int)U.size();
    std::vector<int> S(n,0), R(n,0);

    // Strength S(i)
    for (int i=0;i<n;++i)
        for (int j=0;j<n;++j)
            if (i!=j && dominates(U[i],U[j])) S[i]++;

    // Raw fitness R(i) = sum S(j) que dominan a i
    for (int i=0;i<n;++i){
        int sum=0; for (int j=0;j<n;++j)
            if (i!=j && dominates(U[j],U[i])) sum+=S[j];
        R[i]=sum;
    }

    // Densidad D(i) con k = floor(sqrt(n))
    auto Dm = distancia_kNN(U);
    int k = std::max(1, (int)std::floor(std::sqrt((double)n)));
    std::vector<double> D(n,0.0);
    for (int i=0;i<n;++i){
        std::vector<double> row = Dm[i];
        std::nth_element(row.begin(), row.begin()+k, row.end());
        D[i] = 1.0 / (row[k] + 2.0);
    }

    fit.assign(n,0.0);
    for (int i=0;i<n;++i) fit[i] = (double)R[i] + D[i]; // menor es mejor
}

// -------------------- Funciones declaradas en el .hpp --------------------
void evaluar_obj(Individuo& ind, const Bitset& G){
    ind.jaccard = M(ind.expr, G, Metric::Jaccard);   // (max)
    ind.sizeH   = (int)M(ind.expr, G, Metric::SizeH); // (min)
    ind.n_ops   = ind.expr.n_ops;                     // (min)
}

static Individuo torneo_fit(const std::vector<Individuo>& pool, const std::vector<double>& fitness, int k, std::mt19937& rng){
    std::uniform_int_distribution<> pick(0, (int)pool.size()-1);
    int best_idx = pick(rng);
    for (int i=1;i<k;++i){ int j = pick(rng); if (fitness[j] < fitness[best_idx]) best_idx = j; }
    return pool[best_idx];
}

std::vector<Individuo> init_pop(const std::vector<Bitset>& F, const Bitset& G, int k, int N, std::mt19937& rng){
    std::vector<Individuo> pop; pop.reserve(N);
    std::unordered_set<std::string> seen; seen.reserve(N*2);
    while ((int)pop.size() < N){
        int max_sets = std::min<int>((int)F.size(), k+1);
        int m = std::uniform_int_distribution<>(1, max_sets)(rng);
        std::set<int> usados; for (int i=0;i<m;++i) usados.insert(std::uniform_int_distribution<>(0,(int)F.size()-1)(rng));
        std::vector<int> conjs(usados.begin(), usados.end()); if (conjs.empty()) continue;

        Expression e = build_random_expr(conjs, F, k, rng);
        auto keyE = key_of_expr(e); if (!seen.insert(keyE).second) continue;

        Individuo ind; ind.expr=std::move(e); evaluar_obj(ind,G); ind.rank=0; ind.crowd=0.0;
        pop.push_back(std::move(ind));
    }
    return pop;
}

std::vector<Individuo> environmental_selection(const std::vector<Individuo>& P, const std::vector<Individuo>& Aprev, int Amax){
    // U = P ∪ Aprev
    std::vector<Individuo> U = Aprev; U.insert(U.end(), P.begin(), P.end());

    // Fitness SPEA2
    std::vector<double> fit; fit.reserve(U.size());
    fitness_spea2(U, fit);

    // No dominados
    std::vector<int> nd_idx; nd_idx.reserve(U.size());
    for (int i=0;i<(int)U.size();++i){
        bool dom=false; for (int j=0;j<(int)U.size();++j){ if (j==i) continue; if (dominates(U[j],U[i])){ dom=true; break; } }
        if (!dom) nd_idx.push_back(i);
    }

    std::vector<Individuo> A;
    if ((int)nd_idx.size() >= Amax){
        // Truncado por densidad (kNN)
        std::vector<int> S = nd_idx; // índices en U
        auto Dm = distancia_kNN(U);
        while ((int)S.size() > Amax){
            int pos=0; double best = 1e300;
            for (int p=0;p<(int)S.size();++p){
                double dmin = 1e300;
                for (int q=0;q<(int)S.size();++q){ if (p==q) continue; dmin = std::min(dmin, Dm[S[p]][S[q]]); }
                if (dmin < best){ best=dmin; pos=p; }
            }
            S.erase(S.begin()+pos);
        }
        for (int idx : S) A.push_back(U[idx]);
    }else{
        for (int idx : nd_idx) A.push_back(U[idx]);
        std::vector<std::pair<double,int>> rest; rest.reserve(U.size());
        for (int i=0;i<(int)U.size();++i){
            bool in_nd = std::find(nd_idx.begin(), nd_idx.end(), i) != nd_idx.end();
            if (!in_nd) rest.emplace_back(fit[i], i);
        }
        std::sort(rest.begin(), rest.end(), [](auto&a, auto&b){ return a.first < b.first; });
        for (auto &pr : rest){ if ((int)A.size()>=Amax) break; A.push_back(U[pr.second]); }
    }

    A = unique_by_H(A);
    if ((int)A.size() > Amax) A.resize(Amax);
    return A;
}

// -------------------- Principal SPEA2 --------------------
std::vector<Individuo> spea2(const std::vector<Bitset>& F, const Bitset& G, int k, const SPEA2_Params& p){
    uint64_t seed = (uint64_t)std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::mt19937 rng(seed);

    auto t0 = std::chrono::steady_clock::now();
    auto limit = std::chrono::seconds(p.time_limit_sec);

    // Población inicial y archivo
    std::vector<Individuo> P = init_pop(F,G,k,p.population_size,rng);
    //print_population_summary(P);
    std::vector<Individuo> A; A.reserve(p.archive_size);

    int gen=0;
    for (;;){
        if (std::chrono::steady_clock::now() - t0 >= limit) break;

        // Selección ambiental → A_{t+1}
        A = environmental_selection(P, A, p.archive_size);

        // Seguridad: si A queda vacío, reinyectamos P
        if (A.empty()) A = P;

        // Selección por torneo de fitness en el archivo y variación
        std::vector<double> fitA; fitness_spea2(A, fitA);
        std::vector<Individuo> Q; Q.reserve(p.population_size);
        std::unordered_set<std::string> seen; for (auto& x:A) seen.insert(key_of_expr(x.expr));

        while ((int)Q.size() < p.population_size){
            Individuo p1 = torneo_fit(A, fitA, p.tournament_k, rng);
            Individuo p2 = torneo_fit(A, fitA, p.tournament_k, rng);

            Individuo h;
            if (std::uniform_real_distribution<>(0,1)(rng) < p.pc)
                h = crossover(p1,p2,F,k,rng);
            else
                h = (std::uniform_int_distribution<>(0,1)(rng)==0? p1 : p2);

            if (std::uniform_real_distribution<>(0,1)(rng) < p.pm)
                mutar(h,F,k,rng);

            evaluar_obj(h,G);
            if (seen.insert(key_of_expr(h.expr)).second)
                Q.push_back(std::move(h));
        }

        P.swap(Q);
        gen++;
    }

    // Resultado: archivo A como frente aproximado
    A = unique_by_H(A);
    if ((int)A.size() > p.archive_size) A.resize(p.archive_size);
    return pareto_front(A);
}