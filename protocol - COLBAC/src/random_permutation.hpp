#pragma once
#include <vector>

std::vector<size_t> random_permutation(size_t n);
void random_permutation(size_t n, size_t *permutation);

void print_permutation(std::vector<size_t> perm);
void print_permutation(size_t n, size_t *perm);
void inverse_permutation(size_t n, size_t *perm, size_t *inv_perm);
void compose_permutations(size_t n, size_t *perm1, size_t *perm2, size_t *out_perm);
