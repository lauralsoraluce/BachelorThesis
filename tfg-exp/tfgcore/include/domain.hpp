#pragma once
#include <bitset>
#include <string>
#include <stdexcept>

// Universo fijo a 128 (como acordamos)
#ifndef U_size
#define U_size 128
#endif

using Bitset = std::bitset<U_size>;

// Operaciones de conjuntos (bitset)
inline Bitset set_union(const Bitset& A, const Bitset& B)      { return A | B; }
inline Bitset set_intersect(const Bitset& A, const Bitset& B)  { return A & B; }
inline Bitset set_difference(const Bitset& A, const Bitset& B) { return A & (~B); }
inline Bitset apply_op(const int op, const Bitset& A, const Bitset& B) {
    if (op == 0) return set_union(A, B);
    if (op == 1) return set_intersect(A, B);
    if (op == 2) return set_difference(A, B);
    throw std::invalid_argument("Operación inválida");
}