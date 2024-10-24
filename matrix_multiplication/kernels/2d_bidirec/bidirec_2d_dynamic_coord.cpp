#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <tuple>
#include <cfloat>
#include <string>
#include <cmath>

using namespace std;

const int N = 1000;

using Matrix = vector<vector<int>>;

// Naive bidirectional 2D function
void bidirec_2d_naive(Matrix& a) {
    for (int i = 1; i < N; i++) {
        for (int j = 0; j < N; j++) {
            a[i][j] += a[i - 1][j] + a[i - 1][N - 1 - j];
        }
    }
}

// Pluto-optimized bidirectional 2D function with tiling (only tile_size_t2)
void bidirec_2d_pluto(Matrix& a, int tile_size_t2) {
    int t1, t2, t3;
    int lbv, ubv;

    for (t1 = 1; t1 <= N - 1; t1++) {
        for (t2 = 0; t2 <= (N - 1) / tile_size_t2; t2++) {
            lbv = tile_size_t2 * t2;
            ubv = min(N - 1, tile_size_t2 * t2 + tile_size_t2 - 1);

            for (t3 = lbv; t3 <= ubv; t3++) {
                a[t1][t3] += a[t1 - 1][t3] + a[t1 - 1][N - 1 - t3];
            }
        }
    }
}

// Experimental bidirectional 2D function with parametric tiling over t2 (columns)
void bidirec_2d_expt(Matrix& a, int tile_size_t2) {
    int t1, t2, t3;
    int lbv, ubv;

    for (t1 = 1; t1 <= N - 1; t1++) {
        for (t2 = 0; t2 <= (N - 1) / tile_size_t2; t2++) {
            lbv = tile_size_t2 * t2;
            ubv = min(N - 1, tile_size_t2 * t2 + tile_size_t2 - 1);

            for (t3 = lbv; t3 <= ubv; t3++) {
                a[t1][t3] += a[t1 - 1][t3] + a[t1 - 1][N - 1 - t3];
            }
        }
    }
}

// Function to measure performance of bidirec_2d_expt
double measure_performance_expt(const Matrix& initial_a, int tile_size_t2, int runs) {
    double total_duration = 0.0;

    for (int run = 0; run < runs; ++run) {
        Matrix a = initial_a; // Make a copy

        auto start = chrono::high_resolution_clock::now();
        bidirec_2d_expt(a, tile_size_t2);
        auto end = chrono::high_resolution_clock::now();

        total_duration += chrono::duration<double, milli>(end - start).count();
    }

    return total_duration / runs;
}

// Function to perform exhaustive search over tile_size_t2
tuple<int, double> exhaustive_search(const Matrix& initial_a, const vector<int>& allowed_values, int runs) {
    int best_ts2 = 0;
    double best_performance = DBL_MAX;

    cout << "Starting Exhaustive Search...\n";
    for (int ts2 : allowed_values) {
        double performance = measure_performance_expt(initial_a, ts2, runs);
        cout << "Performance at tile_size_t2 = " << ts2 << ": " << performance << " ms\n";
        if (performance < best_performance) {
            best_performance = performance;
            best_ts2 = ts2;
        }
    }

    return make_tuple(best_ts2, best_performance);
}

// Function to find neighboring tile sizes within a given distance
vector<int> find_neighbors(int x, const vector<int>& allowed_values, int max_distance) {
    vector<int> neighbors;

    auto it = find(allowed_values.begin(), allowed_values.end(), x);
    if (it != allowed_values.end()) {
        int index = distance(allowed_values.begin(), it);
        int size = allowed_values.size();

        // Expand to left and right within max_distance
        for (int dist = 1; dist <= max_distance; ++dist) {
            if (index - dist >= 0) {
                neighbors.push_back(allowed_values[index - dist]);
            }
            if (index + dist < size) {
                neighbors.push_back(allowed_values[index + dist]);
            }
        }
    }

    return neighbors;
}

// Coordinate Descent to find optimal tile_size_t2
tuple<int, double> coordinate_descent(const Matrix& initial_a, int initial_ts2, const vector<int>& allowed_values, int runs, int max_distance) {
    int current_ts2 = initial_ts2;
    double current_performance = measure_performance_expt(initial_a, current_ts2, runs);
    bool improved = true;

    cout << "Starting Coordinate Descent...\n";

    while (improved) {
        improved = false;
        vector<int> neighbors = find_neighbors(current_ts2, allowed_values, max_distance);
        for (int neighbor_ts2 : neighbors) {
            double performance = measure_performance_expt(initial_a, neighbor_ts2, runs);
            cout << "Performance at tile_size_t2 = " << neighbor_ts2 << ": " << performance << " ms\n";
            if (performance < current_performance) {
                cout << "Improving tile_size_t2 from " << current_ts2 << " to " << neighbor_ts2 << " with performance " << performance << " ms\n";
                current_ts2 = neighbor_ts2;
                current_performance = performance;
                improved = true;
            }
        }
    }

    return make_tuple(current_ts2, current_performance);
}

int main(int argc, char* argv[]) {
    // Define allowed tile sizes
    vector<int> allowed_values = {1, 2, 4, 8, 16, 32, 64, 128, 256};

    // Check command-line arguments
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <runs> <max_distance>\n";
        return 1;
    }

    int runs = atoi(argv[1]);
    int max_distance = atoi(argv[2]);
    if (runs <= 0 || max_distance <= 0) {
        cerr << "Number of runs and max_distance must be positive integers.\n";
        return 1;
    }

    // Initialize matrices
    Matrix a_naive(N, vector<int>(N, 0));
    Matrix a_pluto(N, vector<int>(N, 0));
    Matrix a_exhaustive(N, vector<int>(N, 0));
    Matrix a_coordinate(N, vector<int>(N, 0));

    // Populate the matrices with initial values (e.g., i + j)
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            a_naive[i][j] = i + j;
            a_pluto[i][j] = i + j;
            a_exhaustive[i][j] = i + j;
            a_coordinate[i][j] = i + j;
        }
    }

    // Time bidirec_2d_naive
    cout << "Running bidirec_2d_naive...\n";
    auto start_naive = chrono::high_resolution_clock::now();
    bidirec_2d_naive(a_naive);
    auto end_naive = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duration_naive = end_naive - start_naive;
    cout << "Execution time of bidirec_naive_2d: " << duration_naive.count() << " ms\n\n";

    // Time bidirec_2d_pluto with default tile_size_t2
    int default_tile_size_pluto = 32;
    cout << "Running bidirec_2d_pluto with tile_size_t2 = " << default_tile_size_pluto << "...\n";
    auto start_pluto = chrono::high_resolution_clock::now();
    bidirec_2d_pluto(a_pluto, default_tile_size_pluto);
    auto end_pluto = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duration_pluto = end_pluto - start_pluto;
    cout << "Execution time of bidirec_2d_pluto: " << duration_pluto.count() << " ms\n\n";

    
    // Perform Coordinate Descent to find optimal tile_size_t2
    cout << "Performing Coordinate Descent...\n";
    auto start_coordinate_descent = chrono::high_resolution_clock::now();
    // Initialize Coordinate Descent with a starting tile_size_t2, e.g., 32
    int initial_tile_size_coord = 32;
    tuple<int, double> coordinate_result = coordinate_descent(a_coordinate, initial_tile_size_coord, allowed_values, runs, max_distance);
    auto end_coordinate_descent = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duration_coordinate_descent = end_coordinate_descent - start_coordinate_descent;
    cout << "Coordinate Descent converged to tile_size_t2 = " << get<0>(coordinate_result)
         << " with performance = " << get<1>(coordinate_result) << " ms\n";
    cout << "Coordinate Descent took " << duration_coordinate_descent.count() << " ms to converge.\n\n";


    cout << "Performing Exhaustive Search...\n";
    auto start_exhaustive_search = chrono::high_resolution_clock::now();
    tuple<int, double> exhaustive_result = exhaustive_search(a_exhaustive, allowed_values, runs);
    auto end_exhaustive_search = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duration_exhaustive_search = end_exhaustive_search - start_exhaustive_search;
    cout << "Exhaustive Search found tile_size_t2 = " << get<0>(exhaustive_result)
         << " with performance = " << get<1>(exhaustive_result) << " ms\n";
    cout << "Exhaustive Search took " << duration_exhaustive_search.count() << " ms to complete.\n\n";

    // Verification: Compare a_naive with a_pluto
    //cout << "Verifying bidirec_naive_2d and bidirec_2d_pluto...\n";
    bool same_naive_pluto = true;
    for (int i = 0; i < N && same_naive_pluto; i++) {
        if (!equal(a_naive[i].begin(), a_naive[i].end(), a_pluto[i].begin())) {
            same_naive_pluto = false;
            cout << "Difference found between a_naive and a_pluto at row " << i << "\n";
            break;
        }
    }
    // if (same_naive_pluto) {
    //     cout << "Outputs of bidirec_naive_2d and bidirec_2d_pluto are the same.\n\n";
    // } else {
    //     cout << "Outputs of bidirec_naive_2d and bidirec_2d_pluto differ.\n\n";
    // }

    // Verification: Compare a_naive with a_exhaustive
    // cout << "Verifying bidirec_naive_2d and bidirec_2d_expt (Exhaustive Search)...\n";
    bool same_naive_exhaustive = true;
    for (int i = 0; i < N && same_naive_exhaustive; i++) {
        if (!equal(a_naive[i].begin(), a_naive[i].end(), a_exhaustive[i].begin())) {
            same_naive_exhaustive = false;
            cout << "Difference found between a_naive and a_exhaustive at row " << i << "\n";
            break;
        }
    }
    // if (same_naive_exhaustive) {
    //     cout << "Outputs of bidirec_naive_2d and bidirec_2d_expt (Exhaustive Search) are the same.\n\n";
    // } else {
    //     cout << "Outputs of bidirec_naive_2d and bidirec_2d_expt (Exhaustive Search) differ.\n\n";
    // }

    // Verification: Compare a_naive with a_coordinate
    // cout << "Verifying bidirec_naive_2d and bidirec_2d_expt (Coordinate Descent)...\n";
    bool same_naive_coordinate = true;
    for (int i = 0; i < N && same_naive_coordinate; i++) {
        if (!equal(a_naive[i].begin(), a_naive[i].end(), a_coordinate[i].begin())) {
            same_naive_coordinate = false;
            cout << "Difference found between a_naive and a_coordinate at row " << i << "\n";
            break;
        }
    }
    // if (same_naive_coordinate) {
    //     cout << "Outputs of bidirec_naive_2d and bidirec_2d_expt (Coordinate Descent) are the same.\n";
    // } else {
    //     cout << "Outputs of bidirec_naive_2d and bidirec_2d_expt (Coordinate Descent) differ.\n";
    // }

    return 0;
}