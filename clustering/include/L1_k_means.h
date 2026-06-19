#pragma once
#include<vector>
#include<random>

std::vector<int> get_assignments(uint8_t vector_dim, const std::vector<uint8_t>& pts, const std::vector<uint8_t>& centers,
    std::vector<uint16_t>& assignments);

void make_grouped(const int vector_dim, const std::vector<int>& counts,
                  const std::vector<uint8_t>& pts, const std::vector<uint16_t>& assignments,
                  std::vector<uint8_t>& grouped);

std::vector<bool> update_centers(const int vector_dim, const std::vector<int>& counts,
    std::vector<uint8_t>& grouped, std::vector<uint8_t>& centers);


//TODO: replace uniform reseeding with same heuristic from the init
void reseed_centers(uint8_t vector_dim, const std::vector<uint8_t>& pts,
                    const std::vector<bool>& reseeded, std::vector<uint8_t>& centers,
                    std::mt19937& rng); 
                    
bool update_step(uint8_t vector_dim, const std::vector<uint8_t>& pts,
    std::vector<uint8_t>& centers, std::vector<uint16_t>& assignments,     
    std::vector<uint16_t>& prev_assignments,
    std::vector<uint8_t>& grouped, std::mt19937& rng);


std::vector<uint8_t> init_centers(uint8_t vector_dim, const std::vector<uint8_t>&pts,
    size_t num_centers, std::mt19937& rng);


std::pair<std::vector<uint8_t>,std::vector<uint16_t>> run_L1_kmeans(uint8_t vector_dim, const std::vector<uint8_t>& pts,
    size_t num_centers, int max_iters, std::mt19937& rng);

    //assumes that pts is a flattened list of vectors ie: 
    //[v_0[0], v_0[1],  ... v_0[k], v_1[0], v_1[1], v_1[2] ....]
