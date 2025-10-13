#ifndef GREEDY_HPP
#define GREEDY_HPP

#include <vector>
#include <string>
#include "expr.hpp"
#include "domain.hpp"
#include "solutions.hpp"

std::vector<SolMO> greedy(
    const std::vector<Bitset>& F,
    const Bitset& U,
    int k);

/*std::vector<SolMO> evaluar_subconjuntos(
    const std::vector<std::vector<Expression>>& expr,
    const Bitset& G,
    int k);
*/
void print_pareto_front(const std::vector<SolMO>& pareto);


#endif