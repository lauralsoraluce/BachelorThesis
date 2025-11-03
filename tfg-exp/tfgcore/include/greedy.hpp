//======================================================================
// greedy.hpp
//----------------------------------------------------------------------
// Declaraciones del algoritmo greedy para la construcción de expresiones
// y evaluación multiobjetivo (Jaccard, tamaño, nº de operaciones).
//
// Dependencias: expr.hpp, domain.hpp, solutions.hpp
//======================================================================

#ifndef GREEDY_HPP
#define GREEDY_HPP

#include <vector>
#include "expr.hpp"
#include "domain.hpp"
#include "solutions.hpp"

std::vector<SolMO> greedy(
    const std::vector<Bitset>& F,
    const Bitset& U,
    int k);

std::vector<SolMO> greedy_multiobjective_search(
    const std::vector<Bitset>& F,
    const Bitset& U,
    const Bitset& G,
    int k);
//------------------------------------------------------------------
// Mostrar el frente de Pareto por consola
//------------------------------------------------------------------
void print_pareto_front(const std::vector<SolMO>& pareto);


#endif // GREEDY_HPP