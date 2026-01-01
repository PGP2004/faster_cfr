#pragma once
#include <cstdint>
#include <utility>

using InfoKey = std::pair<uint64_t, uint64_t>;

struct InfoKeyHash {
    size_t operator()(const InfoKey& k) const noexcept {
        uint64_t x = k.first ^ (k.second + 0x9e3779b97f4a7c15ULL);

        x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ULL;
        x ^= x >> 27; x *= 0x94d049bb133111ebULL;
        x ^= x >> 31;

        return static_cast<size_t>(x);
    }
};