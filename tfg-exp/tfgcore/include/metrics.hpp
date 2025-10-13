#pragma once
#include <string>
#include <stdexcept>
#include <algorithm>
#include "domain.hpp"
#include "expr.hpp"

// Conjunto de métricas que usarás en todos los algos
enum class Metric {
    Jaccard,
    SizeH,        // |H| (útil como objetivo a minimizar)
    OpSize       // Número de operaciones (útil como objetivo a minimizar)
};

// Parser robusto desde string (case-insensitive)
inline Metric parse_metric(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    if (s == "jaccard" || s == "iou")     return Metric::Jaccard;
    if (s == "sizeh" || s == "size")      return Metric::SizeH;
    if (s == "opsize" || s == "op_size")  return Metric::OpSize;
    throw std::invalid_argument("Metric desconocida: " + s);
}

inline const char* metric_name(Metric m) {
    switch (m) {
        case Metric::Jaccard:   return "Jaccard";
        case Metric::SizeH:     return "SizeH";
        case Metric::OpSize:    return "OpSize";
    }
    return "Unknown";
}

inline bool is_maximization(Metric m) {
    switch (m) {
        case Metric::Jaccard:
            return true;
        case Metric::SizeH:
            return false;
        case Metric::OpSize:
            return false;  
    }
    return true;
}

// Firma canónica que ya usabas: M(H, G, n, metric)
double M(const Expression& H, const Bitset& G, Metric metric);