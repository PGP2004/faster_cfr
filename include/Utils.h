
#pragma once
#include <array>
#include <random>
#include <utility>   
#include <string>

using std::swap;
using std::string;
using std::array;
using std::mt19937;
using std::vector;

#pragma once
#include <array>
#include <random>
#include <utility> // std::swap

uint32_t evaluate_raw(uint8_t* ranks, uint8_t* suits, uint8_t n);

struct Action{
    int type;
    int amt;
};
