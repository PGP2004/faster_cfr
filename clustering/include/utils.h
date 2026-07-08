#pragma once
#include <cstdint>
#include <array>
#include <vector>
#include <fstream>
#include <span>
#include <iostream>

extern "C" {
#define _Bool bool
#include "hand_index.h"
#undef _Bool
}

inline uint8_t card_rank(uint8_t c) noexcept { return (uint8_t)(c / 4); }
inline uint8_t card_suit(uint8_t c) noexcept { return (uint8_t)(c % 4); }
uint32_t evaluate_raw(uint8_t* ranks, uint8_t* suits, uint8_t n);
uint32_t evaluate(std::array<uint8_t, 7>& cards);
uint64_t pack_cards(const std::vector<std::uint8_t>& cards);

struct Indexer {
    hand_indexer_t h;
    Indexer(uint32_t rounds, const uint8_t* cpr) { hand_indexer_init(rounds, cpr, &h); }
    ~Indexer() { hand_indexer_free(&h); }
    Indexer(const Indexer&) = delete; 
    Indexer& operator=(const Indexer&) = delete;
};


inline int L1_dist(std::span<const uint8_t> a, std::span<const uint8_t> b){
    int sum = 0;

    if (a.size() != b.size()) throw std::runtime_error("Dimensions dont match in L1 dist");

    for (size_t i = 0; i < a.size(); ++i)
        sum += std::abs(static_cast<int>(a[i]) - static_cast<int>(b[i]));
    return sum;
}

struct DataHeader{
    uint64_t round; //0 =preflop , 1 = flop, 2 = turn, 3 = river
    uint64_t num_rows;
    uint64_t num_cols;
    uint64_t bytes_per_elt;
    bool operator==(const DataHeader&) const = default;

    std::string to_string() const {
        return "DataHeader{round=" + std::to_string(round) + 
        + ", num_rows=" + std::to_string(num_rows)
        + ", num_cols=" + std::to_string(num_cols)
        + ", bytes_per_elt=" + std::to_string(bytes_per_elt) + "}";
    }
};

//TODO: Clean up the data loading across my scripts
template <typename T>
std::pair<std::vector<T>, DataHeader> load_matrix_and_header(const std::string& result_path) {
    std::ifstream in(result_path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open " + result_path);

    DataHeader header;
    in.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (in.gcount() != static_cast<std::streamsize>(sizeof(header))) throw std::runtime_error("missing header");
    std::cout << "The header says bytes per elt of: " << header.bytes_per_elt << std::endl;
    std::cout << "the template says bytes per elt of : " << sizeof(T) << std::endl;


    if (header.bytes_per_elt != sizeof(T)) throw std::runtime_error("Wrong size type compared to header");

    uint64_t body_bytes = header.num_rows * header.num_cols * header.bytes_per_elt;

    std::streampos here = in.tellg(); in.seekg(0, std::ios::end);
    std::streampos end = in.tellg(); in.seekg(here);
    if (static_cast<uint64_t>(end - here) != body_bytes) throw std::runtime_error("body size does not match header");

    std::vector<T> results(body_bytes / sizeof(T));
    in.read(reinterpret_cast<char*>(results.data()), static_cast<std::streamsize>(body_bytes));
    if (static_cast<uint64_t>(in.gcount()) != body_bytes) throw std::runtime_error("truncated body");

    return {std::move(results), header};
}

template <typename T>
inline void write_matrix_and_header(const std::string& write_path, DataHeader header, const std::vector<T>& results) {

    if (header.bytes_per_elt != sizeof(T)) throw std::runtime_error("The header bytes per elt does not match the actual size");
    if (header.num_rows * header.num_cols != results.size())
        throw std::runtime_error("results size does not match num_rows * num_cols");
    if (header.bytes_per_elt != sizeof(results[0])) throw std::runtime_error("The results element size does not match the header");

    std::ofstream out(write_path, std::ios::binary);
    if (!out) throw std::runtime_error("cant open the path: " + write_path);

    out.write(reinterpret_cast<const char*>(&header), sizeof(header));
    out.write(reinterpret_cast<const char*>(results.data()),
              static_cast<std::streamsize>(results.size() * sizeof(T)));
    if (!out) throw std::runtime_error("write failed: " + write_path);
}