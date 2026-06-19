#include "utils.h"
#include "utils.h"
#include <string>
#include <algorithm>
#include <random>
#include <iostream>
#include <vector>

using namespace std;

struct EMDContext{
    uint64_t num_center_entries;
    uint64_t num_ground_clusters;
    vector<float> sorted_distances; // both arrays should be vector_dim**2 size
    vector<uint16_t> ordered_clusters;
};

struct VecBuffers{
    vector<float> targets;
    vector<float> mean_remaining;
    vector<bool> done;
};

struct Center{
    vector<int> cluster_ids;
    vector<float> cluster_weights;
}

float approx_EMD(const vector<uint16_t>& pts_arr, size_t pt_idx, int num_pt_masses, 
    const vector<uint16_t>& centers_arr, size_t center_idx, const EMDContext& context, 
    VecBuffers& buff){

    float temp = 1.0/static_cast<float>(num_pt_masses);
    buff.targets.assign(temp, num_pt_masses);
    buff.done.assign(false, num_pt_masses);

    buff.mean_remaining.resize(context.num_center_entries);
    for (int i = 0; i < context.num_center_entries; ++i){
        buff.mean_remaining[i] = centers_arr[center_idx+ i];
    }

    float total_cost = 0;

    for (int i = 0; i < context.num_center_entries; ++i){
        for (int j = 0; j < num_pt_masses; ++j){

            if (buff.done[j]) continue;
            uint16_t pt_cluster = pts_arr[pt_idx+j];

            //index = i * num_cols + j
            size_t oc_idx = pt_cluster*context.num_ground_clusters + i;
            int mean_cluster = context.ordered_clusters[oc_idx];
            float amt_rem = buff.mean_remaining[mean_cluster];

            if (amt_rem == 0) continue;

            float d = context.sorted_distances[oc_idx];

            if (amt_rem < buff.targets[j]){
                total_cost += d*amt_rem;
                buff.targets[j] -= amt_rem;
                buff.mean_remaining[mean_cluster] = 0;
            }
            else{
                total_cost += buff.targets[j]*d;
                buff.mean_remaining[mean_cluster] += -buff.targets[j];
                buff.targets[j] = 0;
                buff.done[j] = true;
            }
        }
    }

    return total_cost;
}


//TODO: FIll this function out,it should compute sorted_distances and ordered_clusters
EMDContext precompute_context(Center center, uint16_t num_ground_clusters,
     const vector<int>& dist_matrix){

    vector<int> scratch(num_ground_clusters);
    for (int i = 0; i < center.cluster_ids.size(); ++i){

        for (int j = 0; j < num_ground_clusters;j++){
            //TODO: is this the right way to idx idk. Check
            size_t ij_idx = num_ground_clusters*i+j;

            scratch[j] = dist_matrix[ij_idx]
        }
    }

};


// vector<int> get_assignments( const vector<uint16_t>& pts, const vector<uint16_t>& centers,    
//     const EMDContext& EMDContext, VecBuffers& buff, vector<uint16_t>& assignments) {
    
//     //fills assignments with the assignment data per point and returns a count of the number of points assigned
//     //to each center

//     if (pts.size()%EMDContext.num_pt_masses!= 0) throw runtime_error("The data per pt has to divide the pts size");
//     if (centers.size()!= EMDContext.num_center_entries*EMDContext.desired_clusters) throw runtime_error("The center size is not right");

//     size_t num_flop_centers = centers.size() / EMDContext.desired_clusters;
//     size_t num_pts = pts.size() / EMDContext.num_pt_masses;

//     if (assignments.size() != num_pts) throw runtime_error("assignments vector is not the right size");

//     assignments.assign(num_pts, 0);

//     vector<int> cluster_counts(context.desired_clusters);
//     vector<EMDContext>context_vec(context.desired_clusters)

//     for (size_t pt_idx = 0; pt_idx < num_pts; ++pt_idx) {

//         float best_dist = std::numeric_limits<float>::max();
//         uint16_t best_center = 0;

    
//         for (size_t center_idx = 0; center_idx < EMDContext.desired_clusters; ++center_idx) {
//             float d = approx_EMD(pts, pt_idx*EMDContext.num_pt_masses,
//                 centers, center_idx*EMDContext.num_center_entries,
//                 EMDContext, buff);
        
//             if (d < best_dist) { 
//                 best_dist = d; 
//                 best_center = static_cast<uint16_t>(center_idx); 
//             }
//         }

//         cluster_counts[best_center] += 1;
//         assignments[pt_idx] = best_center;
//     }
//     return cluster_counts;
// }
