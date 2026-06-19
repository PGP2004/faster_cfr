#include <array>
#include "utils.h"
#include "L1_k_means.h"
#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <algorithm>
#include <random>

namespace fs = std::filesystem;
using namespace std;

// pair<vector<uint8_t>, uint8_t> load_turn_cdfs(const string& read_cdf_path) {
//     ifstream in(read_cdf_path, ios::binary);
//     if (!in) throw runtime_error("cannot open " + read_cdf_path);

//     DataHeader header;

//     cout << "Recieved turn_cdf_ header" << header.to_string() << endl;
//     in.read(reinterpret_cast<char*>(&header), sizeof(header));
//     if (in.gcount() != static_cast<streamsize>(sizeof(header))) throw runtime_error("missing header");
//     if (header.round != 2) throw runtime_error("not reading data from turn");
//     if (header.bytes_per_elt != sizeof(uint8_t)) throw runtime_error("not reading uint8_t");

//     uint8_t  num_buckets = static_cast<uint8_t>(header.num_cols);
//     uint64_t total_turns = header.num_rows;
//     uint64_t total_bytes = total_turns * header.num_cols * header.bytes_per_elt;

//     streampos here = in.tellg(); in.seekg(0, ios::end);
//     streampos end = in.tellg(); in.seekg(here);
//     if (static_cast<uint64_t>(end - here) != total_bytes) throw runtime_error("body size does not match header");

//     vector<uint8_t> cdf_pts(total_bytes);
//     in.read(reinterpret_cast<char*>(cdf_pts.data()), static_cast<streamsize>(total_bytes));
//     if (static_cast<uint64_t>(in.gcount()) != total_bytes) throw runtime_error("truncated body");

//     return {std::move(cdf_pts), num_buckets};
// }


void write_assignment_and_centers(const string& center_write_path, const string& assignment_write_path,
    const string& cdf_read_path, uint16_t num_clusters, mt19937& rng) {

    if (fs::exists(center_write_path)) throw runtime_error("write path already exists");
    if (fs::exists(assignment_write_path)) throw runtime_error("write path already exists");

    auto [cdfs, cdf_header] = load_matrix_and_header<uint8_t>(cdf_read_path);
    // cout << "The cdf header is: " << cdf_header.to_string() << endl;
    // cout << "Finished loading cdfs" << endl;

    uint64_t num_buckets = cdf_header.num_rows;
    int max_iters = 1000;
    auto [centers, assignments] = run_L1_kmeans(num_buckets, cdfs, num_clusters, max_iters, rng);

    cout << "Finished L1 k means" << endl;

    DataHeader center_header{2, num_clusters, num_buckets, 1};
    DataHeader assignment_header{2, assignments.size(), 1, 2};

    write_matrix_and_header<uint8_t>(center_write_path, center_header, centers);
    write_matrix_and_header<uint16_t>(assignment_write_path, assignment_header, assignments);
    //writes centers as [c0[0], c0[1] .. c0[k],
    //                   c1[0] ,c1[1],.. c1[k], 
    //                  ... ] where k = CDF dimension = num_clusters
}

int main(int argc, char** argv) {
    cout << "Started" << endl;
    fs::path exe = fs::weakly_canonical(fs::path(argv[0]));
    fs::path root = exe.parent_path();                              
    fs::path storage = root / "storage";

    uint8_t num_clusters = 50;
    std::mt19937 rng{42};

    write_assignment_and_centers((storage / "turn_cdf_centers").string(),
        (storage / "turn_assignments").string(), (storage / "turn_cdfs").string(),
        num_clusters, rng);

    cout << "Finished" << endl;
}