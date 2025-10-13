#include "metrics.hpp"
#include <unordered_set>

// Jaccard: |H ∩ G| / |H ∪ G|
inline double jaccard_coefficient(const Bitset& H, const Bitset& G) {
    int intersection = (H & G).count();
    int union_size = (H | G).count();
    
    if (union_size == 0) return 1.0;  // Ambos vacíos: similitud máxima
    return static_cast<double>(intersection) / union_size;
}

double M(const Expression& H, const Bitset& G, Metric metric) {
    switch (metric) {
        case Metric::Jaccard:
            return jaccard_coefficient(H.conjunto, G);
        
        case Metric::SizeH:
            return static_cast<int>(H.used_sets.size());

        case Metric::OpSize:
            return H.n_ops;
    }
    
    throw std::invalid_argument("Métrica no implementada");
}