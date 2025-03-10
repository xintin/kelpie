#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <cmath>
#include <climits>

#define alpha 1.0
#define beta 0.4
#define input_height 1000
#define input_width 1000

using namespace std;
using namespace std::chrono;

inline int ceild(int n, int d) {
    return static_cast<int>(ceil(static_cast<double>(n) / static_cast<double>(d)));
}

inline int floord(int n, int d) {
    return static_cast<int>(floor(static_cast<double>(n) / static_cast<double>(d)));
}

inline int max(int x, int y) { return std::max(x, y); }
inline int min(int x, int y) { return std::min(x, y); }

vector<double> bias(input_width, 1.0);
const std::vector<int> allowed_values = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512};


vector<vector<float>> output_naive(input_height, vector<float>(input_width, 0));
vector<vector<float>> output_tiled(input_height, vector<float>(input_width, 0));
vector<vector<float>> output_pluto(input_height, vector<float>(input_width, 0));

void gemm_bias_relu_tiled(const vector<vector<float>>& A, const vector<vector<float>>& B, int tile_size_m, int tile_size_n, int tile_size_k);

int measure_performance(int ts1, int ts2, int ts3, int runs) {
  long long total_duration = 0;

  vector<vector<float>> A(1000, vector<float>(1000, 1.0));
  vector<vector<float>> B(1000, vector<float>(1000, 1.0));

  for (int i = 0; i < runs; ++i) {
    auto start = std::chrono::high_resolution_clock::now();
    gemm_bias_relu_tiled(A, B, ts1, ts2, ts3);
    auto stop = std::chrono::high_resolution_clock::now();

    total_duration += std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
  }

  return total_duration / runs;
}

std::vector<std::tuple<int, int, int>> find_neighbors(int ts1, int ts2, int ts3) {
  std::vector<std::tuple<int, int, int>> neighbors;

  for (int i = 0; i < 3; ++i) {
    int current = (i == 0) ? ts1 : (i == 1) ? ts2 : ts3;
    auto it = std::find(allowed_values.begin(), allowed_values.end(), current);
    if (it != allowed_values.end()) {
      if (it != allowed_values.begin()) {
        int prev1 = *(it - 1);
        neighbors.emplace_back((i == 0) ? prev1 : ts1, (i == 1) ? prev1 : ts2, (i == 2) ? prev1 : ts3);
        if (it - 1 != allowed_values.begin()) {
          int prev2 = *(it - 2);
          neighbors.emplace_back((i == 0) ? prev2 : ts1, (i == 1) ? prev2 : ts2, (i == 2) ? prev2 : ts3);
        }
      }
      if (it + 1 != allowed_values.end()) {
        int next1 = *(it + 1);
        neighbors.emplace_back((i == 0) ? next1 : ts1, (i == 1) ? next1 : ts2, (i == 2) ? next1 : ts3);
        if (it + 2 != allowed_values.end()) {
          int next2 = *(it + 2);
          neighbors.emplace_back((i == 0) ? next2 : ts1, (i == 1) ? next2 : ts2, (i == 2) ? next2 : ts3);
        }
      }
    }
  }

  std::cout << "Neighbor size: " << neighbors.size() << "\n";
  return neighbors;
}

std::tuple<int, int, int, int> move_to_minimum(int ts1, int ts2, int ts3, int runs) {
  auto neighbors = find_neighbors(ts1, ts2, ts3);
  int best_performance = measure_performance(ts1, ts2, ts3, runs);
  std::tuple<int, int, int> best_config = std::make_tuple(ts1, ts2, ts3);

  for (auto &neighbor : neighbors) {
    int n1 = std::get<0>(neighbor), n2 = std::get<1>(neighbor), n3 = std::get<2>(neighbor);
    int perf = measure_performance(n1, n2, n3, runs);
    if (perf < best_performance) {
      best_performance = perf;
      best_config = neighbor;
    }
  }

  return std::make_tuple(std::get<0>(best_config), std::get<1>(best_config), std::get<2>(best_config), best_performance);
}

std::tuple<int, int, int, int> coordinate_descent(int ts1, int ts2, int ts3, int runs) {
  std::cout << "Starting position: (" << ts1 << ", " << ts2 << ", " << ts3 << ") with performance: "
            << measure_performance(ts1, ts2, ts3, runs) << " ms\n";
  auto current_position = move_to_minimum(ts1, ts2, ts3, runs);
  int current_performance = std::get<3>(current_position);

  while (true) {
    auto next_position = move_to_minimum(std::get<0>(current_position), std::get<1>(current_position), std::get<2>(current_position), runs);
    int next_performance = std::get<3>(next_position);
    if (next_performance >= current_performance) {
      return current_position;
    }
    current_position = next_position;
    current_performance = next_performance;

    std::cout << "tile size (t1, t2, t3): (" << std::get<0>(current_position) << ", " << std::get<1>(current_position) << ", " << std::get<2>(current_position)
              << ") with performance: " << current_performance << " ms\n";
  }
}

std::tuple<int, int, int, int> exhaustive_search(int runs) {
  int best_ts1 = 0, best_ts2 = 0, best_ts3 = 0, best_performance = INT_MAX;

  for (int ts1 : allowed_values) {
    for (int ts2 : allowed_values) {
      for (int ts3 : allowed_values) {
        int performance = measure_performance(ts1, ts2, ts3, runs);

        cout << ts1 << " " << ts2 << " " << ts3 << " " << performance << "\n";
        if (performance < best_performance) {
          best_performance = performance;
          best_ts1 = ts1;
          best_ts2 = ts2;
          best_ts3 = ts3;
        }
      }
    }
  }

  return std::make_tuple(best_ts1, best_ts2, best_ts3, best_performance);
}

std::tuple<int, int, int, int> combined_exhaustive_search(int runs) {
  int best_ts1 = 0, best_ts2 = 0, best_ts3 = 0, best_performance = INT_MAX;

  for (int ts1 : allowed_values) {
    for (int ts2 : allowed_values) {
      for (int ts3 : allowed_values) {
        int performance = measure_performance(ts1, ts2, ts3, runs);
        if (performance < best_performance) {
          best_performance = performance;
          best_ts1 = ts1;
          best_ts2 = ts2;
          best_ts3 = ts3;
          std::cout << "tile size (t1, t2, t3): " << best_ts1 << " " << best_ts2 << " " << best_ts3 << " " << best_performance << "ms \n";
        }
      }
    }
  }

  return coordinate_descent(best_ts1, best_ts2, best_ts3, runs);
}

int main() {
  int runs = 1; // Number of runs for performance measurement

  // auto result = combined_exhaustive_search(runs);
  cout << "Start coordinate search:\n";
  auto start_timer = std::chrono::high_resolution_clock::now();
  auto result = coordinate_descent(1, 1, 1, runs);
  auto end_timer = std::chrono::high_resolution_clock::now();
  auto duration_timer = std::chrono::duration_cast<std::chrono::milliseconds>(end_timer - start_timer);
  std::cout << "coordinate search took " << duration_timer.count() << " milliseconds to converge." << "\n";

  cout << "Best tile sizes: (" << get<0>(result) << ", " << get<1>(result) << ", " << std::get<2>(result) << ") with performance: "
            << get<3>(result) << " ms\n";

  cout << "Start exhaustive search:\n";
  start_timer = std::chrono::high_resolution_clock::now();
  result = exhaustive_search(runs);
  end_timer = std::chrono::high_resolution_clock::now();
  duration_timer = std::chrono::duration_cast<std::chrono::milliseconds>(end_timer - start_timer);
  std::cout << "coordinate search took " << duration_timer.count() << " milliseconds to converge." << "\n";

  cout << "Best tile sizes: (" << get<0>(result) << ", " << get<1>(result) << ", " << std::get<2>(result) << ") with performance: "
            << get<3>(result) << " ms\n";

  return 0;
}

// Implementation of gemm_bias_relu_tiled function
void gemm_bias_relu_tiled(const vector<vector<float>>& A, const vector<vector<float>>& B, int tile_size_m, int tile_size_n, int tile_size_k) {
  int t2, t3, t4, t5, t6, t7;
  int lbv, ubv;

  vector<vector<float>> output_tiled(input_height, vector<float>(input_width, 0));

  // Beta scaling
  for (t2 = 0; t2 <= floord(input_height - 1, tile_size_m); t2++) {
    for (t3 = 0; t3 <= floord(input_width - 1, tile_size_n); t3++) {
      for (t4 = tile_size_m * t2; t4 <= min(input_height - 1, tile_size_m * t2 + tile_size_m - 1); t4++) {
        lbv = tile_size_n * t3;
        ubv = min(input_width - 1, tile_size_n * t3 + tile_size_n - 1);
        for (t5 = lbv; t5 <= ubv; t5++) {
          output_tiled[t4][t5] *= beta;
        }
      }
    }
  }

  // Matrix multiplication
  for (t2 = 0; t2 <= floord(input_height - 1, tile_size_m); t2++) {
    for (t3 = 0; t3 <= floord(input_width - 1, tile_size_n); t3++) {
      for (t4 = 0; t4 <= floord(input_width - 1, tile_size_k); t4++) {
        for (t5 = tile_size_m * t2; t5 <= min(input_height - 1, tile_size_m * t2 + tile_size_m - 1); t5++) {
          for (t6 = tile_size_k * t4; t6 <= min(input_width - 1, tile_size_k * t4 + tile_size_k - 1); t6++) {
            lbv = tile_size_n * t3;
            ubv = min(input_width - 1, tile_size_n * t3 + tile_size_n - 1);
            for (t7 = lbv; t7 <= ubv; t7++) {
              output_tiled[t5][t7] += alpha * A[t5][t6] * B[t6][t7];
            }
          }
        }
      }
    }
  }

  // Add bias and apply ReLU
  for (t2 = 0; t2 <= floord(input_height - 1, tile_size_m); t2++) {
    for (t3 = 0; t3 <= floord(input_width - 1, tile_size_n); t3++) {
      for (t4 = tile_size_m * t2; t4 <= min(input_height - 1, tile_size_m * t2 + tile_size_m - 1); t4++) {
        lbv = tile_size_n * t3;
        ubv = min(input_width - 1, tile_size_n * t3 + tile_size_n - 1);
        for (t5 = lbv; t5 <= ubv; t5++) {
          output_tiled[t4][t5] += 1.0; // Example bias value, replace with actual bias if needed
          output_tiled[t4][t5] = max(output_tiled[t4][t5], 0.0f);
        }
      }
    }
  }
}
