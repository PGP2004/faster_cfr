#include "L1_k_means.h"
#include "utils.h"
#include "utils.h"
#include <string>
#include <algorithm>
#include <random>
#include <iostream>

using namespace std;

vector<int> get_assignments(uint8_t vector_dim, const vector<uint8_t>& pts, const vector<uint8_t>& centers,
                    vector<uint16_t>& assignments) {
    
    //fills assignments with the assignment data per point and returns a count of the number of points assigned
    //to each center
    
    if (centers.size() % vector_dim != 0 || pts.size() % vector_dim != 0)
        throw runtime_error("sizes don't match the dimension");

    size_t num_centers = centers.size() / vector_dim;
    size_t num_pts = pts.size() / vector_dim;

    if (assignments.size() != num_pts) throw runtime_error("assignments vector is not the right size");
    assignments.assign(num_pts, 0);

    vector<int> cluster_counts(num_centers);

    for (size_t pt_idx = 0; pt_idx < num_pts; ++pt_idx) {

        int best_dist = INT_MAX;
        uint16_t best_center = 0;

        for (size_t center_idx = 0; center_idx < num_centers; ++center_idx) {
            int d = L1_dist(pts, pt_idx*vector_dim, centers, center_idx*vector_dim, vector_dim);
            if (d < best_dist) { 
                best_dist = d; 
                best_center = static_cast<uint16_t>(center_idx); 
            }
        }

        cluster_counts[best_center] += 1;
        assignments[pt_idx] = best_center;
    }
    return cluster_counts;
}

void make_grouped(const int vector_dim, const vector<int>& counts,
                  const vector<uint8_t>& pts, const vector<uint16_t>& assignments,
                  vector<uint8_t>& grouped) {
    //Note: when passed in pts is grouped by (pt_idx, dim_idx)
    //this is a reshaping of the pts based off assignments to make L1 distance calculation easier
    //grouped in the pts data ordered lexicographically by  (center_idx, dim_idx, pt_idx) 
    
    //Makes it nice to use the L1 dist functions ince the data it wants to use is all contiguos in the array

    size_t num_centers = counts.size();

    vector<size_t> offsets(num_centers * vector_dim, 0);
    size_t running = 0;
    for (size_t center_idx = 0; center_idx < num_centers; ++center_idx) {
        for (int dim_idx = 0; dim_idx < vector_dim; ++dim_idx) {
            offsets[vector_dim * center_idx + dim_idx] = running;
            running += counts[center_idx];
        }
    }
     
    size_t num_pts = assignments.size();
    grouped.resize(num_pts * vector_dim);   

    for (size_t pt_idx = 0; pt_idx < num_pts; ++pt_idx) {
        uint16_t assigned_center = assignments[pt_idx];
        for (int dim_idx = 0; dim_idx < vector_dim; ++dim_idx) {
            size_t gpd_idx = offsets[vector_dim * assigned_center + dim_idx];
            grouped[gpd_idx] = pts[pt_idx * vector_dim + dim_idx];
            ++offsets[vector_dim * assigned_center + dim_idx];
        }
    }
}

vector<bool> update_centers(const int vector_dim, const vector<int>& counts,
                    vector<uint8_t>& grouped, vector<uint8_t>& centers) {
    
    //updates centers using L1 centroid (this is the coordinate wise median)
    //Returns an arraty of bools where the i^th index is True 
    // if and only if the i^th center needs to be reseeded since it was not assigned any pts
    //in this case it does not do the reseeding itself it leaves that center untouched

    size_t num_centers = counts.size();
    centers.assign(num_centers * vector_dim, 0);

    size_t running = 0;
    vector<bool> center_reseeded(counts.size());
    for (size_t center_idx = 0; center_idx < num_centers; ++center_idx) {
        for (int dim_idx = 0; dim_idx < vector_dim; ++dim_idx) {
            size_t block_len = counts[center_idx];
            auto it = grouped.begin() + running;
            running += block_len;      

            if (counts[center_idx] == 0){
                center_reseeded[center_idx] = true;
                continue;
            } 

            nth_element(it, it + block_len / 2, it + block_len); 
            centers[vector_dim * center_idx + dim_idx] = *(it + block_len / 2);
        }
    }

    return center_reseeded;
}

//should probably add same heuristic as used in init here.
//am lazy and for my use case does not get called much

void reseed_centers(uint8_t vector_dim, const vector<uint8_t>& pts,
                    const vector<bool>& reseeded, vector<uint8_t>& centers,
                    mt19937& rng) {

    size_t num_pts = pts.size() / vector_dim;  
    uniform_int_distribution<size_t> pick(0, num_pts - 1);

    for (size_t c = 0; c < reseeded.size(); ++c) {
        if (!reseeded[c]) continue;
        size_t r = pick(rng);                      
        for (int j = 0; j < vector_dim; ++j)
            centers[c * vector_dim + j] = pts[r * vector_dim + j];
    }
}  

bool update_step(uint8_t vector_dim, const vector<uint8_t>& pts,
    vector<uint8_t>& centers, vector<uint16_t>& assignments,     
    vector<uint16_t>& prev_assignments,
    vector<uint8_t>& grouped, mt19937& rng) {

    prev_assignments.swap(assignments);              

    vector<int> counts = get_assignments(vector_dim, pts, centers, assignments); 
    make_grouped(vector_dim, counts, pts, assignments, grouped);
    vector<bool> reseeded = update_centers(vector_dim, counts, grouped, centers);

    if (find(reseeded.begin(), reseeded.end(), true) != reseeded.end()){
        reseed_centers(vector_dim, pts, reseeded, centers, rng);
    }
    return assignments != prev_assignments;
}

vector<uint8_t> init_centers(uint8_t vector_dim, const vector<uint8_t>&pts,
    size_t num_centers, mt19937& rng){
    //heuristic initialization of centers with distance caching

    size_t num_pts = pts.size()/vector_dim;
    vector<uint8_t> centers(num_centers*vector_dim);

    uniform_int_distribution<size_t> upto(0, num_pts -1 );
    size_t first_center = upto(rng);

    for (int i = 0; i < vector_dim; ++i){
        centers[i] = pts[first_center*vector_dim + i];
    }

    vector<int> min_d_cache(num_pts, INT_MAX); 

    uint64_t total = 0;
    for (size_t c = 1; c < num_centers; ++c){
        //c is the center we are trying to sample
        total = 0;
        for(size_t p = 0; p < num_pts; ++p){
            //p is the pt idx

            int dist =  L1_dist(pts, p * vector_dim, centers, (c-1) * vector_dim, vector_dim);
            if (dist < min_d_cache[p]){
                min_d_cache[p] = dist;
            }

            total += min_d_cache[p];
        }

        uniform_int_distribution<uint64_t> upto(0, total);
        uint64_t target = upto(rng);
        size_t chosen = 0;

        uint64_t cum_sum = 0;
        for (size_t p = 0; p < num_pts; ++p){
            cum_sum += min_d_cache[p];
            if(cum_sum >= target){
                chosen = p;
                break;
            }
        }

        for (int i = 0; i < vector_dim; ++i){
            centers[c*vector_dim+i] = pts[chosen*vector_dim+i];
        }
    }

    return centers;
}


pair<vector<uint8_t>,vector<uint16_t>> run_L1_kmeans(uint8_t vector_dim, const vector<uint8_t>& pts,
    size_t num_centers, int max_iters, mt19937& rng) {
    cout << "The k means started" << endl;

    if (pts.size() % vector_dim != 0) throw runtime_error("pts size not a multiple of dim");
    size_t num_pts = pts.size() / vector_dim;
    vector<uint16_t> assignments(num_pts);

    vector<uint8_t> centers = init_centers(vector_dim, pts, num_centers,rng);
    cout << "finished intializing centers" << endl;

    vector<uint16_t> prev_assignments(num_pts, -1); // set values = to -1 so that doest coincide w centers
    vector<uint8_t> grouped;

    for (int iter = 0; iter < max_iters; ++iter) {

        bool changed = update_step(vector_dim, pts, centers, assignments, prev_assignments, grouped, rng);
        cout << "completed iter" << endl;
        if (!changed) break;    
    }

    return {std::move(centers), std::move(assignments)};
}