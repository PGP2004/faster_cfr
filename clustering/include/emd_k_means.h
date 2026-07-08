
#pragma once
#include "utils.h"
#include <cstdint>
#include <string>
#include <algorithm>
#include <random>
#include <iostream>
#include <limits>
#include <vector>
#include <span>


namespace emd{

//TODO: Add support for variable atoms per center
struct Center{
    std::vector<float> atom_wts;
    std::vector<std::uint16_t> atoms;
};


struct EMDCache{
    std::vector<float> sorted_distances; 
    std::vector<std::uint16_t> ordered_clusters;
};

struct EMDScratch{
    std::vector<float> targets;
    std::vector<float> mean_remaining;
    std::vector<bool> done;
};

struct ClusterBuffer{
    std::vector<float> best_dists;
    std::vector<std::uint16_t> grouped;
    std::vector<std::uint16_t> assignments;
    std::vector<std::uint16_t> prev_assignments;
    std::vector<size_t> counts;
};

struct Params{
    size_t num_centers;
    size_t num_atoms;
    size_t atoms_per_center;
    size_t pdf_size;
    size_t num_pdfs;
    std::vector<float> dist_matrix;
    size_t max_iters;
    mutable std::mt19937 rng;
};

void fill_emd_cache(const Params& params, const Center& ctr, EMDCache& emd_cache);

float approx_EMD(const Params& params, std::span<const std::uint16_t> pdf, 
    const Center& ctr, const EMDCache& emd_cache, EMDScratch& emd_scratch);

void clipped_dense_center(const Params& params, const std::vector<int>& sparse_rep, Center& ctr);

void update_ass_and_counts(const Params& params, const std::vector<std::uint16_t>& pdfs,
    const std::vector<Center>& centers, ClusterBuffer& c_buff, EMDCache& emd_cache);

void update_grouped(const Params& params,const std::vector<std::uint16_t>& pdfs, ClusterBuffer& c_buff);

std::vector<bool> update_centers(const Params& params, std::vector<Center>& centers, ClusterBuffer& c_buff);

void reseed_centers(const Params& params, const std::vector<std::uint16_t>& pdfs,
    std::vector<Center>& centers, const std::vector<bool>& reseeded);

std::vector<Center> init_centers(const Params& params, const std::vector<std::uint16_t>&pdfs, 
    ClusterBuffer& c_buff, EMDCache& emd_cache);

bool step(const Params& params, const std::vector<std::uint16_t>& pdfs,
    std::vector<Center>& centers, ClusterBuffer& c_buff, EMDCache& emd_cache);

std::pair<std::vector<Center>, std::vector<std::uint16_t>> emd_k_means(const Params& params, const std::vector<std::uint16_t>& pdfs);
   
}