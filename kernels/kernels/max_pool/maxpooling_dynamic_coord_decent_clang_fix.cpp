#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <climits>
#include <cmath>
#include <tuple>
#include <algorithm>

using namespace std;
using namespace std::chrono;

#define input_height 10000
#define input_width 10000
#define pool_size 2
#define output_height (input_height - pool_size + 1)
#define output_width (input_width - pool_size + 1)

const vector<int> allowed_values = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512};

inline int ceild(int n, int d) {
    return static_cast<int>(ceil(static_cast<double>(n) / static_cast<double>(d)));
}

inline int floord(int n, int d) {
    return static_cast<int>(floor(static_cast<double>(n) / static_cast<double>(d)));
}

void computationKernel2(const vector<vector<int>>& input, vector<vector<int>>& output, int tile_size_height, int tile_size_width);

int measure_performance(vector<vector<int>>& A, vector<vector<int>>& B, int ts1, int ts2, int runs) {
    long long total_duration = 0;

    for (int i = 0; i < runs; ++i) {
        auto start = chrono::high_resolution_clock::now();
        computationKernel2(A, B, ts1, ts2);
        auto stop = chrono::high_resolution_clock::now();

        total_duration += chrono::duration_cast<chrono::milliseconds>(stop - start).count();
    }

    return total_duration / runs;
}

vector<tuple<int, int>> find_neighbors(int x, int y) {
    vector<tuple<int, int>> neighbors;

    for (int i = 0; i < 2; ++i) {
        int current = (i == 0) ? x : y;
        auto it = std::find(allowed_values.begin(), allowed_values.end(), current);
        if (it != allowed_values.end()) {
            if (it != allowed_values.begin()) {
                int prev = *(it - 1);
                neighbors.emplace_back((i == 0) ? prev : x, (i == 0) ? y : prev);
            }
            if (it + 1 != allowed_values.end()) {
                int next = *(it + 1);
                neighbors.emplace_back((i == 0) ? next : x, (i == 0) ? y : next);
            }
        }
    }

    return neighbors;
}

tuple<int, int, int> move_to_minimum(vector<vector<int>>& A, vector<vector<int>>& B, int x, int y, int runs) {
    auto neighbors = find_neighbors(x, y);
    int best_performance = measure_performance(A, B, x, y, runs);
    tuple<int, int> best_config = make_tuple(x, y);

    for (auto& neighbor : neighbors) {
        int nx = get<0>(neighbor), ny = get<1>(neighbor);
        int perf = measure_performance(A, B, nx, ny, runs);
        if (perf < best_performance) {
            best_performance = perf;
            best_config = neighbor;
        }
    }

    return make_tuple(get<0>(best_config), get<1>(best_config), best_performance);
}

tuple<int, int, int> coordinate_descent(vector<vector<int>>& A, vector<vector<int>>& B, int x, int y, int runs) {
    cout << "Starting position: (" << x << ", " << y << ") with performance: " << measure_performance(A, B, x, y, runs) << " ms\n";
    auto current_position = move_to_minimum(A, B, x, y, runs);
    int current_performance = get<2>(current_position);

    while (true) {
        auto next_position = move_to_minimum(A, B, get<0>(current_position), get<1>(current_position), runs);
        int next_performance = get<2>(next_position);
        if (next_performance >= current_performance) {
            return current_position;
        }
        current_position = next_position;
        current_performance = next_performance;

        cout << "tile size (x, y): (" << get<0>(current_position) << ", " << get<1>(current_position) << ") with performance: " << current_performance << " ms\n";
    }
}

tuple<int, int, int> exhaustive_search(vector<vector<int>>& A, vector<vector<int>>& B, int runs) {
    int best_ts1 = 0, best_ts2 = 0, best_performance = INT_MAX;

    for (int ts1 : allowed_values) {
        for (int ts2 : allowed_values) {
            int performance = measure_performance(A, B, ts1, ts2, runs);

            cout << ts1 << " " << ts2 << " " << performance << "\n";
            if (performance < best_performance) {
                best_performance = performance;
                best_ts1 = ts1;
                best_ts2 = ts2;
            }
        }
    }

    return make_tuple(best_ts1, best_ts2, best_performance);
}

tuple<int, int, int> combined_exhaustive_search(vector<vector<int>>& A, vector<vector<int>>& B, int runs) {
    int best_ts1 = 0, best_ts2 = 0, best_performance = INT_MAX;

    for (int ts1 : allowed_values) {
        for (int ts2 : allowed_values) {
            int performance = measure_performance(A, B, ts1, ts2, runs);
            if (performance < best_performance) {
                best_performance = performance;
                best_ts1 = ts1;
                best_ts2 = ts2;
                cout << "tile size (x, y): " << best_ts1 << " " << best_ts2 << " " << best_performance << " ms \n";
            }
        }
    }

    return coordinate_descent(A, B, best_ts1, best_ts2, runs);
}

void initialize(vector<vector<int>>& A, vector<vector<int>>& B) {
    // Initialize the input matrix with random values
    for (int i = 0; i < input_height; ++i) {
        for (int j = 0; j < input_width; ++j) {
            A[i][j] = rand() % 100; // Corrected random initialization
        }
    }

    // Initialize filter matrix B with 0
    for (int i = 0; i < output_height; ++i) {
        for (int j = 0; j < output_width; ++j) {
            B[i][j] = 0;
        }
    }
}

int main() {
    // Seed for random number generation
    srand(time(0));

    // Initialize matrices
    vector<vector<int>> A(input_height, vector<int>(input_width));
    vector<vector<int>> B(output_height, vector<int>(output_width, 0));
    initialize(A, B);

    // Perform coordinate descent and measure time
    int runs = 1; // Example number of runs
    
    auto start_timer = high_resolution_clock::now();
    auto result = coordinate_descent(A, B, 1, 1, runs);
    auto end_timer = high_resolution_clock::now();
    auto duration_timer = duration_cast<microseconds>(end_timer - start_timer);
    cout << "coordinate search took " << duration_timer.count() << " microseconds to converge." << endl;    
    cout << "Coordinate Descent result: (" << get<0>(result) << ", " << get<1>(result) << ") with performance: " << get<2>(result) << " ms\n";

    initialize(A, B);
    start_timer = high_resolution_clock::now();
    result = exhaustive_search(A, B, runs);
    end_timer = high_resolution_clock::now();
    duration_timer = duration_cast<microseconds>(end_timer - start_timer);
    cout << "Exhaustive search took " << duration_timer.count() << " microseconds to converge." << endl;
    cout << "Exhaustive Search result: (" << get<0>(result) << ", " << get<1>(result) << ") with performance: " << get<2>(result) << " ms\n";

    // initialize(A, B);
    // result = combined_exhaustive_search(A, B, runs);
    // cout << "Combined Exhaustive Search result: (" << get<0>(result) << ", " << get<1>(result) << ") with performance: " << get<2>(result) << " ms\n";

    return 0;
}

// Tiled version of max pooling
void computationKernel2(const vector<vector<int>>& input, vector<vector<int>>& output, int tile_size_height, int tile_size_width) {
    for (int ii = 0; ii < output_height; ii += tile_size_height) {
        for (int jj = 0; jj < output_width; jj += tile_size_width) {
            for (int i = ii; i < min(ii + tile_size_height, output_height); ++i) {
                for (int j = jj; j < min(jj + tile_size_width, output_width); ++j) {
                    int max_val = -1e10;
                    for (int ki = 0; ki < pool_size; ++ki) {
                        for (int kj = 0; kj < pool_size; ++kj) {
                            int current_val = input[i + ki][j + kj];
                            max_val = max(current_val, max_val);
                        }
                    }
                    output[i][j] = max_val;
                }
            }
        }
    }
}

