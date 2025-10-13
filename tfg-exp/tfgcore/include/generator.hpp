#pragma once
#include <bitset>
#include <vector>
#include <random>
#include <iostream>
#include "domain.hpp"

using Bitset = std::bitset<U_size>;

// -----------------------------------------------------------------------------
// Parámetros de generación (puedes cargarlos de YAML o definirlos constantes)
// -----------------------------------------------------------------------------
struct GenConfig {
    int G_size_min = 10;

    int F_n_min = 10;
    int F_n_max = 100;

    int Fi_size_min = 1;
    int Fi_size_max = 64;
};

// -----------------------------------------------------------------------------
// Generar G
// -----------------------------------------------------------------------------
Bitset generar_G(int n, int tam_min, int seed = 456) {
    Bitset G;
    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> dist_tam(tam_min, n);
    std::uniform_int_distribution<int> dist_idx(0, n - 1);

    int count = 0;
    int tam = dist_tam(gen);

    while (count < tam) {
        int idx;
        do{
            idx = dist_idx(gen);
        } while (G[idx]);

        G[idx] = 1;
        count++;
    }
    return G;
}

// -----------------------------------------------------------------------------
// Generar F
// -----------------------------------------------------------------------------
std::vector<Bitset> generar_F(int n, int n_min, int n_max,
                              int tam_min, int tam_max, int seed = 123) {
    std::vector<Bitset> F;

    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> dist_tam_sub(tam_min, tam_max);
    std::uniform_int_distribution<int> dist_tam_f(n_min, n_max);
    std::uniform_int_distribution<int> dist_idx(0, n - 1);

    int tam_f = dist_tam_f(gen);
    F.reserve(tam_f);

    for (int i=0; i<tam_f; i++) {
        Bitset sub; 
        int tam_sub = dist_tam_sub(gen);
        int count = 0;

        while (count < tam_sub) {
            int idx;

            do{
                idx = dist_idx(gen);
            } while (sub[idx]);

            sub[idx] = 1;
            count++;
        }
        F.push_back(sub);
    }
    return F;
}
