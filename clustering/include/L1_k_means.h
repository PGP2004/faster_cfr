#pragma once
#include<vector>
#include<random>
#include<span>
#include <cstdint>

struct ClusterBuffer{
    std::vector<std::uint8_t> grouped;
    std::vector<std::uint16_t> assignments;
    std::vector<std::uint16_t> prev_assignments;
    std::vector<size_t> counts;
};

struct Params{
    size_t num_centers;
    size_t num_pts;
    size_t dim;
    size_t max_iters;
    mutable std::mt19937 rng; 
};

void update_ass_and_counts(const Params& params, const std::vector<uint8_t>& pts, 
    const std::vector<uint8_t>& centers, ClusterBuffer& c_buff);

void update_grouped(const Params& params, const std::vector<uint8_t>& pts, ClusterBuffer& c_buff);

std::vector<bool> update_centers(const Params& params, std::vector<uint8_t>& centers, ClusterBuffer& c_buff);

void reseed_centers(const Params& params, const std::vector<uint8_t>& pts, std::vector<uint8_t>& centers,
    const std::vector<bool>& reseeded); 
                    
bool step(const Params& params, const std::vector<uint8_t>& pts, std::vector<uint8_t>& centers, ClusterBuffer& c_buff);

std::vector<uint8_t> l1_center_init(const Params& params, const std::vector<uint8_t>&pts);

std::pair<std::vector<uint8_t>,std::vector<uint16_t>> l1_k_means(const Params& params, 
    const std::vector<uint8_t>& pts);

    //assumes that pts is a flattened list of vectors ie: 
    //[v_0[0], v_0[1],  ... v_0[k], v_1[0], v_1[1], v_1[2] ....]
