//======================================================================
// exhaustiva.hpp
//----------------------------------------------------------------------
// Declaraciones relacionadas con la búsqueda exhaustiva de expresiones
// sobre conjuntos. 
//
// Incluye:
//  - Generación exhaustiva de expresiones hasta profundidad k.
//  - Evaluación de dichas expresiones frente a un conjunto objetivo G.
//  - Impresión del frente de Pareto resultante.
//
// Dependencias: expr.hpp, domain.hpp, solutions.hpp
//======================================================================

#ifndef EXHAUSTIVA_HPP
#define EXHAUSTIVA_HPP

#include <vector>
#include "expr.hpp"
#include "domain.hpp"
#include "solutions.hpp"

//------------------------------------------------------------------
// Genera todas las expresiones posibles hasta profundidad k
// a partir de la familia F y el universo U.
//------------------------------------------------------------------
std::vector<SolMO> exhaustive_search(
    const std::vector<Bitset>& F,
    const Bitset& U,
    const Bitset& G,
    int k);

//------------------------------------------------------------------
// Evalúa todas las expresiones generadas respecto a un conjunto G.
// Devuelve las soluciones multiobjetivo encontradas.
//------------------------------------------------------------------
std::vector<SolMO> evaluar_subconjuntos(
    const std::vector<std::vector<Expression>>& expr,
    const Bitset& G,
    int k);

//------------------------------------------------------------------
// Muestra por consola las soluciones del frente de Pareto.
//------------------------------------------------------------------
void print_pareto_front(const std::vector<SolMO>& pareto);

#endif // EXHAUSTIVA_HPP