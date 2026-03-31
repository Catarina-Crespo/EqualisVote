#include <random>
#include <algorithm>
#include <iostream>
#include "random_permutation.hpp"

std::vector<size_t> random_permutation(size_t n)
{
    std::vector<size_t> perm(n);            // Create a vector to store the permutation
    std::iota(perm.begin(), perm.end(), 0);  // Fill the vector with the values 0, 1, ..., n-1
    std::random_device rd;               // Create a random device (source of randomness)
    std::mt19937 g(rd());                // Use the random device to seed a Mersenne Twister random generator
    std::shuffle(perm.begin(), perm.end(), g);  // Shuffle the vector perm using the random generator
    return perm;                         // Return the shuffled vector (random permutation)
}

void random_permutation(size_t n, size_t *permutation)
{
    std::vector<size_t> perm = random_permutation(n);
    std::copy(perm.begin(), perm.end(), permutation);
}

void print_permutation(std::vector<size_t> perm)
{
    for (auto el : perm)
    {
        std::cout << el << " ";
    }
    std::cout << std::endl;
}

void print_permutation(size_t n, size_t *perm)
{
    for (auto i = 0; i < n; i++)
    {
        std::cout << perm[i] << " ";
    }
    std::cout << std::endl;
}

void inverse_permutation(size_t n, size_t *perm, size_t *inv_perm)
{
  for (auto i = 0; i < n; i++)
  {
    inv_perm[perm[i]] = i;
  }
}

// given permutations perm1 and perm2, it returns out_perm = perm2 o perm1
void compose_permutations(size_t n, size_t *perm1, size_t *perm2, size_t *out_perm)
{
    for (auto i = 0; i < n; i++)
    {
        out_perm[i] = perm1[perm2[i]];
    }
}