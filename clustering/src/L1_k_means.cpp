#include "l1_k_means.h"
#include "utils.h"
#include <string>
#include <cstdint>
#include <algorithm>
#include <random>
#include <iostream>
#include <climits>
#include <vector>
#include <span>




using namespace std;

//using namesapce here cus its awfully similar to the EMD stuff since theres a lot of overlap.

namespace l1{

void update_ass_and_counts(const Params& params, const vector<uint8_t>& pts, 
    const vector<uint8_t>& centers, ClusterBuffer& c_buff) {
    
    c_buff.assignments.assign(params.num_pts, 0);
    c_buff.counts.assign(params.num_centers, 0);

    for (size_t pt_idx = 0; pt_idx < params.num_pts; ++pt_idx) {

        int best_dist = INT_MAX;
        uint16_t best_center = 0;

        for (size_t center_idx = 0; center_idx < params.num_centers; ++center_idx) {

            span<const uint8_t>pt_span(&pts[pt_idx * params.dim], params.dim);
            span<const uint8_t>ctr_span(&centers[center_idx * params.dim], params.dim);
            int d = L1_dist(pt_span, ctr_span);

            if (d < best_dist) { 
                best_dist = d; 
                best_center = static_cast<uint16_t>(center_idx); 
            }
        }
        c_buff.counts[best_center] += 1;
        c_buff.assignments[pt_idx] = best_center;
    }
}

void update_grouped(const Params& params, const vector<uint8_t>& pts, ClusterBuffer& c_buff) {
    //Note: when passed in pts is grouped by (pt_idx, dim_idx)
    //this is a reshaping of the pts based off assignments to make L1 distance calculation easier
    //grouped in the pts data ordered lexicographically by  (center_idx, dim_idx, pt_idx) 
    
    //Makes it nice to use the L1 dist functions ince the data it wants to use is all contiguos in the array

    vector<size_t> offsets(params.num_centers * params.dim, 0);
    size_t running = 0;

    for (size_t center_idx = 0; center_idx < params.num_centers; ++center_idx) {
        for (size_t dim = 0; dim <params.dim; ++dim) {
            offsets[params.dim * center_idx + dim] = running;
            running += c_buff.counts[center_idx];
        }
    }

    c_buff.grouped.resize(params.num_pts*params.dim);   

    for (size_t pt_idx = 0; pt_idx < params.num_pts; ++pt_idx) {
        uint16_t assigned_center = c_buff.assignments[pt_idx];
        for (size_t dim_idx = 0; dim_idx < params.dim; ++dim_idx) {
            size_t gpd_idx = offsets[params.dim * assigned_center + dim_idx];
            c_buff.grouped[gpd_idx] = pts[pt_idx * params.dim + dim_idx];
            ++offsets[params.dim * assigned_center + dim_idx];
        }
    }
}

vector<bool> update_centers(const Params& params, vector<uint8_t>& centers, ClusterBuffer& c_buff) {
    
    //updates centers using L1 centroid (this is the coordinate wise median)
    //Returns an arraty of bools where the i^th index is True 
    // if and only if the i^th center needs to be reseeded since it was not assigned any pts
    //in this case it does not do the reseeding itself it leaves that center untouched
    centers.resize(params.num_centers * params.dim);
    size_t running = 0;

    vector<bool> center_reseeded(params.num_centers);
    for (size_t ctr = 0; ctr < params.num_centers; ++ctr) {
        for (size_t dim = 0; dim < params.dim; ++dim) {
            size_t block_len = c_buff.counts[ctr];
            auto it = c_buff.grouped.begin() + running;
            running += block_len;      

            if (c_buff.counts[ctr] == 0){
                center_reseeded[ctr] = true;
                continue;
            } 

            nth_element(it, it + block_len / 2, it + block_len); 
            centers[params.dim * ctr + dim] = *(it + block_len / 2);
        }
    }

    return center_reseeded;
}


void reseed_centers(const Params& params, const vector<uint8_t>& pts, vector<uint8_t>& centers,
    const vector<bool>& reseeded) {

    uniform_int_distribution<size_t> pick(0, params.num_pts - 1);

    for (size_t c = 0; c < reseeded.size(); ++c) {
        if (!reseeded[c]) continue;
        size_t r = pick(params.rng);                      
        for (size_t j = 0; j < params.dim; ++j)
            centers[c * params.dim + j] = pts[r * params.dim + j];
    }
}  

bool step(const Params& params, const vector<uint8_t>& pts,
    vector<uint8_t>& centers, ClusterBuffer& c_buff){

    c_buff.prev_assignments.swap(c_buff.assignments);              

    update_ass_and_counts(params, pts, centers, c_buff); 
    update_grouped(params, pts, c_buff);
    vector<bool> reseeded = update_centers(params, centers, c_buff);

    if (find(reseeded.begin(), reseeded.end(), true) != reseeded.end()){
        reseed_centers(params, pts, centers, reseeded);
    }
    return c_buff.assignments != c_buff.prev_assignments;
}

vector<uint8_t> l1_center_init(const Params& params, const vector<uint8_t>&pts){
    //heuristic initialization of centers with distance caching
    vector<uint8_t> centers(params.num_centers*params.dim);

    uniform_int_distribution<size_t> upto(0, params.num_pts -1 );
    size_t first_center = upto(params.rng);

    for (size_t i = 0; i < params.dim; ++i){
        centers[i] = pts[first_center*params.dim + i];
    }
    vector<int> min_d_cache(params.num_pts, INT_MAX); 
    uint64_t total = 0;

    for (size_t c = 1; c < params.num_centers; ++c){
        //c is the center we are trying to sample
        total = 0;
        for(size_t p = 0; p < params.num_pts; ++p){
            //p is the pt idx

            //TODO:Switch to span L1 implementation

            span<const uint8_t>pt_span(&pts[p * params.dim], params.dim);
            span<const uint8_t>ctr_span(&centers[(c-1) * params.dim], params.dim);
            int dist = L1_dist(pt_span, ctr_span);
           
            if (dist < min_d_cache[p]){
                min_d_cache[p] = dist;
            }

            total += min_d_cache[p];
        }

        uniform_int_distribution<uint64_t> upto(0, total);
        uint64_t target = upto(params.rng);
        size_t chosen = 0;

        uint64_t cum_sum = 0;
        for (size_t p = 0; p < params.num_pts; ++p){
            cum_sum += min_d_cache[p];
            if(cum_sum >= target){
                chosen = p;
                break;
            }
        }

        for (size_t i = 0; i < params.dim; ++i){
            centers[c*params.dim+i] = pts[chosen*params.dim+i];
        }
    }

    return centers;
}


pair<vector<uint8_t>,vector<uint16_t>> l1_k_means(const Params& params, const vector<uint8_t>& pts){
    cout << "The k means started" << endl;
    if (pts.size() != params.dim* params.num_pts) throw runtime_error("pt size doesnt match param specs");

    ClusterBuffer c_buff;
    c_buff.assignments.resize(params.num_pts);
    c_buff.prev_assignments.assign(params.num_pts, -1);
    c_buff.grouped.resize(params.num_pts*params.dim);
    c_buff.counts.resize(params.num_centers);

    vector<uint8_t> centers = l1_center_init(params, pts);
    cout << "finished intializing centers" << endl;

    for (int iter = 0; iter < params.max_iters; ++iter) {
        bool changed = step(params, pts,centers, c_buff);
        cout << "completed iter" << endl;
        if (!changed) break;    
    }

    return {std::move(centers), std::move(c_buff.assignments)};
}
}