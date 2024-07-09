#include <algorithm>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <tuple>
#include <vector>
#include <ctime>

#define inp_size 1000000
#define ceild(n,d)  ceil(((double)(n))/((double)(d)))
#define floord(n,d) floor(((double)(n))/((double)(d)))
#define max(x,y)    ((x) > (y)? (x) : (y))
#define min(x,y)    ((x) < (y)? (x) : (y))

using namespace std;
using namespace std::chrono;

vector<float> input(inp_size);
vector<float> exp_values(inp_size);
vector<float> output_naive(inp_size);
vector<float> output_tiled(inp_size);
vector<float> output_pluto(inp_size);
float max_val;

const std::vector<int> allowed_values = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512};

void initialize(vector<float>& input, vector<float>& exp_values, float& max_val) {
    srand(static_cast<unsigned>(time(0)));

    for (int i = 0; i < inp_size; ++i) {
        input[i] = static_cast<float>(rand()) / RAND_MAX * 100;
    }

    max_val = *max_element(input.begin(), input.end());
}

void softmax_naive(const vector<float>& input, vector<float>& exp_values, float max_val) {
    float sum_exp = 0.0;

    // Compute the exponential values and their sum
    for (int i = 0; i < inp_size; ++i) {
        exp_values[i] = exp(input[i] - max_val);
        sum_exp += exp_values[i];
    }

    // Compute the softmax values
    for (int i = 0; i < inp_size; ++i) {
        output_naive[i] = exp_values[i] / sum_exp;
    }
}

void softmax_tiled(const vector<float>& input, vector<float>& exp_values, float max_val, int tile_size) {
    int t2, t3;
    int lbv, ubv;
    float sum_exp = 0.0;

    // Compute the exponential values and their sum
    for (t2 = 0; t2 < inp_size; ++t2) {
        exp_values[t2] = exp(input[t2] - max_val);
        sum_exp += exp_values[t2];
    }

    // Compute the softmax values using tiling
    for (t2 = 0; t2 <= floord(inp_size - 1, tile_size); ++t2) {
        lbv = tile_size * t2;
        ubv = min(inp_size - 1, tile_size * t2 + tile_size - 1);
        #pragma ivdep
        #pragma vector always
        for (t3 = lbv; t3 <= ubv; ++t3) {
            output_tiled[t3] = exp_values[t3] / sum_exp;
        }
    }
}

int measure_performance(int tile_size, int runs) {
    long long total_duration = 0;

    for (int i = 0; i < runs; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        softmax_tiled(input, exp_values, max_val, tile_size);
        auto stop = std::chrono::high_resolution_clock::now();

        total_duration += std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
    }

    return total_duration / runs;
}

std::vector<int> find_neighbors(int tile_size) {
    std::vector<int> neighbors;

    auto it = std::find(allowed_values.begin(), allowed_values.end(), tile_size);
    if (it != allowed_values.end()) {
        if (it != allowed_values.begin()) {
            int prev = *(it - 1);
            neighbors.push_back(prev);
        }
        if (it + 1 != allowed_values.end()) {
            int next = *(it + 1);
            neighbors.push_back(next);
        }
    }

    return neighbors;
}

std::tuple<int, int> move_to_minimum(int tile_size, int runs) {
    auto neighbors = find_neighbors(tile_size);
    int best_performance = measure_performance(tile_size, runs);
    int best_tile_size = tile_size;

    for (auto &neighbor : neighbors) {
        int perf = measure_performance(neighbor, runs);
        if (perf < best_performance) {
            best_performance = perf;
            best_tile_size = neighbor;
        }
    }

    return std::make_tuple(best_tile_size, best_performance);
}

std::tuple<int, int> coordinate_descent(int tile_size, int runs) {
    std::cout << "Starting position: " << tile_size
              << " with performance: " << measure_performance(tile_size, runs)
              << " ms\n";
    auto current_position = move_to_minimum(tile_size, runs);
    int current_performance = std::get<1>(current_position);

    while (true) {
        auto next_position = move_to_minimum(std::get<0>(current_position), runs);
        int next_performance = std::get<1>(next_position);
        if (next_performance >= current_performance) {
            return current_position;
        }
        current_position = next_position;
        current_performance = next_performance;

        std::cout << "Tile size: " << std::get<0>(current_position)
                  << " with performance: " << current_performance << " ms\n";
    }
}

std::tuple<int, int> exhaustive_search(int runs) {
    int best_tile_size = 0, best_performance = INT_MAX;

    for (int ts : allowed_values) {
        int performance = measure_performance(ts, runs);

        std::cout << ts << " " << performance << "\n";
        if (performance < best_performance) {
            best_performance = performance;
            best_tile_size = ts;
        }
    }

    return std::make_tuple(best_tile_size, best_performance);
}

std::tuple<int, int> combined_exhaustive_search(int runs) {
    int best_tile_size = 0, best_performance = INT_MAX;

    for (int ts : allowed_values) {
        int performance = measure_performance(ts, runs);
        if (performance < best_performance) {
            best_performance = performance;
            best_tile_size = ts;
            std::cout << "Tile size: " << best_tile_size << " "
                      << best_performance << " ms\n";
        }
    }

    return coordinate_descent(best_tile_size, runs);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <number_of_runs>\n";
        return 1;
    }

    int runs = std::atoi(argv[1]);
    if (runs <= 0) {
        std::cerr << "Number of runs must be a positive integer.\n";
        return 2;
    }

    initialize(input, exp_values, max_val);

    int tile_size = 1;

    std::cout << "\nCoordinate Descent:\n";
    auto result = coordinate_descent(tile_size, runs);
    std::cout << "Converged to: " << std::get<0>(result)
              << " with performance: " << std::get<1>(result) << " ms\n";

    std::cout << "\nExhaustive search:\n";
    result = exhaustive_search(runs);
    std::cout << "Converged to: " << std::get<0>(result)
              << " with performance: " << std::get<1>(result) << " ms\n";

    return 0;
}
