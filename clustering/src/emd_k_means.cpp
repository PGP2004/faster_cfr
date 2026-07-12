    #include "utils.h"
    #include "emd_k_means.h"
    #include <string>
    #include <algorithm>
    #include <random>
    #include <iostream>
    #include <limits>
    #include <vector>
    #include <span>
    #include <chrono>

    using namespace std;

namespace emd{

    //TODO: Finish this matching paper convention
    void fill_emd_cache(const Params& params, const Center& ctr, EMDCache& emd_cache){

        vector<size_t> idx_scratch(params.atoms_per_center);
        vector<float> dist_scratch(params.atoms_per_center);

        emd_cache.ordered_clusters.resize(params.num_atoms * params.atoms_per_center);
        emd_cache.sorted_distances.resize(params.num_atoms * params.atoms_per_center);

        for (size_t i = 0; i < params.num_atoms; ++i){
            for (size_t j = 0; j < params.atoms_per_center; ++j){

                size_t atom = static_cast<size_t>(ctr.atoms[j]);
                dist_scratch[j] = params.dist_matrix[i*params.num_atoms+atom];
                idx_scratch[j] = j;
            }

            sort(idx_scratch.begin(), idx_scratch.end(), 
                [&](size_t a, size_t b){return dist_scratch[a] < dist_scratch[b];});
            
            for (size_t j = 0; j < params.atoms_per_center; ++j){

                emd_cache.ordered_clusters[i*params.atoms_per_center + j] = idx_scratch[j];
                emd_cache.sorted_distances[i*params.atoms_per_center + j] = dist_scratch[idx_scratch[j]];               
            }
        }
    }


    float approx_EMD(const Params& params, span<const uint16_t> pdf, const Center& ctr, const EMDCache& emd_cache, EMDScratch& emd_scratch) {

        if (pdf.size() != params.pdf_size) throw runtime_error("Something got cooked");

        float temp = 1.0f / static_cast<float>(pdf.size());
        emd_scratch.targets.assign(pdf.size(), temp);
        emd_scratch.done.assign(pdf.size(), false);
        emd_scratch.mean_remaining.assign(ctr.atom_wts.begin(), ctr.atom_wts.end());

        float total_cost = 0;

        for (size_t i = 0; i < params.atoms_per_center; ++i) {

            for (size_t j = 0; j < params.pdf_size; ++j) {

                if (emd_scratch.done[j]) continue;

                uint16_t ground_cluster = pdf[j];
                size_t oc_idx = ground_cluster * params.atoms_per_center + i;
                int mean_cluster = emd_cache.ordered_clusters[oc_idx];

                float amt_rem = emd_scratch.mean_remaining[mean_cluster];
                if (amt_rem == 0) continue;

                float d = emd_cache.sorted_distances[oc_idx];

                if (amt_rem < emd_scratch.targets[j]) {
                    total_cost += d * amt_rem;
                    emd_scratch.targets[j] -= amt_rem;
                    emd_scratch.mean_remaining[mean_cluster] = 0;
                } 

                else {
                    total_cost += emd_scratch.targets[j] * d;
                    emd_scratch.mean_remaining[mean_cluster] -= emd_scratch.targets[j];
                    emd_scratch.targets[j] = 0;
                    emd_scratch.done[j] = true;
                }
            }
        }
        return total_cost;
    }

    void update_ass_and_counts(const Params& params, const vector<uint16_t>& pdfs,
        const vector<Center>& centers, ClusterBuffer& c_buff, EMDCache& emd_cache) {

        c_buff.assignments.assign(params.num_pdfs, 0);
        c_buff.best_dists.assign(params.num_pdfs, numeric_limits<float>::max());

        for (size_t ctr = 0; ctr < centers.size(); ++ctr) {
            fill_emd_cache(params, centers[ctr], emd_cache);   

            #pragma omp parallel
            {
                EMDScratch local_scratch;              
                #pragma omp for schedule(static)
                for (size_t pdf = 0; pdf < params.num_pdfs; ++pdf) {
                    span<const uint16_t> pdf_span(&pdfs[pdf * params.pdf_size], params.pdf_size);
                    float dist = approx_EMD(params, pdf_span, centers[ctr], emd_cache, local_scratch);
                    if (dist < c_buff.best_dists[pdf]) {
                        c_buff.best_dists[pdf] = dist;
                        c_buff.assignments[pdf] = static_cast<uint16_t>(ctr);
                    }
                }
            }
        }

        c_buff.counts.assign(params.num_centers, 0);
        for (size_t pdf = 0; pdf < params.num_pdfs; ++pdf)
            c_buff.counts[c_buff.assignments[pdf]] += 1;
    }

    void update_grouped(const Params& params, const vector<uint16_t>& pdfs, ClusterBuffer& c_buff) {
        // groups pdfs by cluster assignment into contiguous blocks in c_buff.grouped

        vector<size_t> offsets(params.num_centers, 0);

        for (size_t ctr = 1; ctr < params.num_centers; ++ctr) {
            offsets[ctr] = offsets[ctr-1] + c_buff.counts[ctr-1] * params.pdf_size;
        }

        c_buff.grouped.resize(params.num_pdfs * params.pdf_size);

        for (size_t pdf = 0; pdf < params.num_pdfs; ++pdf) {
            uint16_t ass_ctr = c_buff.assignments[pdf];
            size_t gpd_idx = offsets[ass_ctr];
            size_t pdfs_idx = pdf * params.pdf_size;
            for (size_t i = 0; i < params.pdf_size; ++i) {
                c_buff.grouped[gpd_idx + i] = pdfs[pdfs_idx + i];
            }
            offsets[ass_ctr] += params.pdf_size;
        }   
    }

    void clipped_dense_center(const Params& params, const vector<int>& sparse_rep, Center& ctr){

        ctr.atom_wts.resize(params.atoms_per_center);
        ctr.atoms.resize(params.atoms_per_center);

        vector<size_t> idx(sparse_rep.size());
        for (size_t i = 0; i < sparse_rep.size(); ++i) idx[i] = i;
        nth_element(idx.begin(), idx.begin() + params.atoms_per_center, idx.end(),
                    [&](size_t a, size_t b){ return sparse_rep[a] > sparse_rep[b]; });

        float cum_sum = 0;
        for (size_t i = 0; i < params.atoms_per_center; ++i){
            ctr.atoms[i] = idx[i];
            cum_sum += static_cast<float>(sparse_rep[idx[i]]);
        }

        for (size_t i = 0; i < params.atoms_per_center; ++i){
            size_t sr_idx = idx[i];
            ctr.atom_wts[i] = static_cast<float>(sparse_rep[sr_idx])/cum_sum;
        }

    }

    vector<bool> update_centers(const Params& params, vector<Center>& centers, ClusterBuffer& c_buff) {
        

        vector<bool> center_reseeded(c_buff.counts.size());
        size_t running = 0;

        vector<int> sparse_rep(params.num_atoms);

        for (size_t ctr = 0; ctr < centers.size(); ++ctr){

            sparse_rep.assign(params.num_atoms, 0);

            if (c_buff.counts[ctr] == 0){
                center_reseeded[ctr] = true;
                continue;
            } 

            size_t end = running + c_buff.counts[ctr]*params.pdf_size;
            for (size_t i = running; i < end; ++i){
                sparse_rep[c_buff.grouped[i]] += 1;
            }

            running = end;
            clipped_dense_center(params, sparse_rep, centers[ctr]);
        }
        return center_reseeded;
    }


    void reseed_centers(const Params& params, const vector<uint16_t>& pdfs,
        vector<Center>& centers, const vector<bool>& reseeded) {

        //fully randomized reinitialize. Should prolly do better at some point

        size_t num_pdfs = pdfs.size() / params.pdf_size;  
        uniform_int_distribution<size_t> pick(0, num_pdfs - 1);

        for (size_t ctr = 0; ctr < params.num_centers; ++ctr) { 

            if (!reseeded[ctr]) continue;

            centers[ctr].atoms.assign(params.atoms_per_center, 0);
            centers[ctr].atom_wts.assign(params.atoms_per_center, 0.0);

            size_t pdf = pick(params.rng);                      
            for (size_t j = 0; j < params.pdf_size; ++j){
                centers[ctr].atoms[j] = pdfs[pdf * params.pdf_size + j];
                centers[ctr].atom_wts[j] = 1.0/static_cast<float>(params.pdf_size);
            }
        }
    }  

    vector<Center> init_centers(const Params& params, const vector<uint16_t>&pdfs, 
        ClusterBuffer& c_buff, EMDCache& emd_cache){
        //heuristic initialization of centers 
        
        cout << "began init" << endl;
        vector<Center> centers(params.num_centers);

        uniform_int_distribution<size_t> upto(0, params.num_pdfs -1 );
        size_t first_center = upto(params.rng);


        for (size_t i = 0; i < params.num_centers; ++i){
            centers[i].atoms.assign(params.atoms_per_center, 0);
            centers[i].atom_wts.assign(params.atoms_per_center, 0.0);
        }

        for (size_t j = 0; j < params.pdf_size; ++j){
            centers[0].atoms[j] = pdfs[first_center * params.pdf_size + j];
            centers[0].atom_wts[j] = 1.0/static_cast<float>(params.pdf_size);
        }

        c_buff.best_dists.assign(params.num_pdfs, numeric_limits<float>::max());

        //best_dist here actually holds distance squared. kinda funky but nicer this way

        for (size_t ctr = 1; ctr < params.num_centers; ++ctr){
            float total = 0.0;

            fill_emd_cache(params, centers[ctr-1], emd_cache);
            #pragma omp parallel
            {
                EMDScratch local_scratch;                     
                #pragma omp for reduction(+:total) schedule(static)
                for (size_t pdf = 0; pdf < params.num_pdfs; ++pdf) {
                    span<const uint16_t> pdf_span(&pdfs[pdf * params.pdf_size], params.pdf_size);
                    float dist = approx_EMD(params, pdf_span, centers[ctr-1], emd_cache, local_scratch);
                    dist = dist * dist;
                    if (dist < c_buff.best_dists[pdf]) c_buff.best_dists[pdf] = dist;
                    total += c_buff.best_dists[pdf];
                }
            }

            uniform_real_distribution<float> pick_dist(0.0f, total);
            float target = pick_dist(params.rng);

            size_t chosen = 0;

            float cum_sum = 0;
            for (size_t pdf = 0; pdf < params.num_pdfs; ++pdf){
                cum_sum += c_buff.best_dists[pdf];
                if(cum_sum >= target){
                    chosen = pdf;
                    break;
                }
            }

            for (size_t i = 0; i < params.pdf_size; ++i){
                centers[ctr].atoms[i] = pdfs[chosen * params.pdf_size + i];
                centers[ctr].atom_wts[i] = 1.0/static_cast<float>(params.pdf_size);
            }

            cout << "Initialized center: " << ctr << endl;
        }

        return centers;
    }


    bool step(const Params& params, const vector<uint16_t>& pdfs,
        vector<Center>& centers, ClusterBuffer& c_buff, EMDCache& emd_cache) {

        c_buff.prev_assignments.swap(c_buff.assignments);   
        
        update_ass_and_counts(params, pdfs, centers, c_buff, emd_cache); 
        cout << "Updated assignment and counts" << endl;
        update_grouped(params, pdfs, c_buff);
        cout << "Updated grouped" << endl;
        vector<bool> reseeded = update_centers(params, centers, c_buff);
        cout << "Updated centers" << endl;
        if (find(reseeded.begin(), reseeded.end(), true) != reseeded.end()){
            cout << "Did a center reseeding " << endl;
            reseed_centers(params, pdfs, centers, reseeded);
        }

        return c_buff.assignments != c_buff.prev_assignments;
    }

    pair<vector<Center>, vector<uint16_t>> emd_k_means(const Params& params, const vector<uint16_t>& pdfs) {
        cout << "The k means started" << endl;

        if (pdfs.size() != params.pdf_size*params.num_pdfs){
            throw runtime_error("pdf size does not match params");
        }

        ClusterBuffer c_buff;
        c_buff.assignments.assign(params.num_pdfs, 0);
        c_buff.prev_assignments.assign(params.num_pdfs, 0);
        c_buff.best_dists.assign(params.num_pdfs, 0.0);
        c_buff.grouped.assign(params.num_pdfs*params.pdf_size, 0);
        c_buff.counts.assign(params.num_centers, 0);

        EMDCache emd_cache;
        EMDScratch emd_scratch;

        vector<Center> centers = init_centers(params, pdfs, c_buff, emd_cache);
        cout << "finished intializing centers" << endl;

        for (size_t iter = 0; iter < params.max_iters; ++iter) {

            auto t_iter_start = chrono::steady_clock::now();
            cout << "starting iter : " << iter << endl;
            bool changed = step(params, pdfs, centers, c_buff, emd_cache);   
            auto t_iter_end = chrono::steady_clock::now();    
            cout << "completed iter: " << iter << endl;
            cout << "iter " << iter << " took: " << chrono::duration_cast<chrono::duration<double>>(t_iter_end - t_iter_start).count() << " s" << endl;
            if (!changed) break;    
        }

        return {std::move(centers), std::move(c_buff.assignments)};
    }
}
