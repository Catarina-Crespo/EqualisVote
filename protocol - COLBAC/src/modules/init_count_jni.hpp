//
// Created by Catarina on 21/01/2026.
//

#pragma once

#include <vector>
#include "ShuffleResult.hpp"

ShuffleResult init_count_jni(
    size_t group_size,
    const unsigned char *s_G,
    unsigned char **T,
    const std::vector<unsigned char *> &vec_of_ballots,
    size_t n_ballots
);
