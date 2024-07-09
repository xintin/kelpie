#include <algorithm>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <tuple>
#include <vector>
#include "utilities.h"

#define N 10000
#define input_height 10000
#define input_width 10000
#define pool_size 2

void computationKernel2(const std::vector<std::vector<float>>& input, std::vector<std::vector<float>>& output, int tile_size_height, int tile_size_width);
vector<vector<float>> input(input_height, vector<float>(input_width));

const std::vector<int> allowed_values = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512};

inline int ceild(int n, int d) {
  return static_cast<int>(std::ceil(static_cast<double>(n) / static_cast<double>(d)));
}

inline int floord(int n, int d) {
  return static_cast<int>(std::floor(static_cast<double>(n) / static_cast<double>(d)));
}

inline int max(int x, int y) { return std::max(x, y); }
inline int min(int x, int y) { return std::min(x, y); }

int measure_performance(const std::vector<std::vector<float>>& input, int ts1, int ts2, int runs) {
  long long total_duration = 0;
  int output_height = (input.size() - 2) + 1;
  int output_width = (input[0].size() - 2) + 1;

  std::vector<std::vector<float>> output(output_height, std::vector<float>(output_width, 0));

  for (int i = 0; i < runs; ++i) {
    auto start = std::chrono::high_resolution_clock::now();
    computationKernel2(input, output, ts1, ts2);
    auto stop = std::chrono::high_resolution_clock::now();

    total_duration += std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
  }

  return total_duration / runs;
}

std::vector<std::tuple<int, int>> find_neighbors(int x, int y) {
  std::vector<std::tuple<int, int>> neighbors;

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

std::tuple<int, int, int> move_to_minimum(int x, int y, int runs) {
  auto neighbors = find_neighbors(x, y);
  int best_performance = measure_performance(input, x, y, runs);
  std::tuple<int, int> best_config = std::make_tuple(x, y);

  for (auto &neighbor : neighbors) {
    int nx = std::get<0>(neighbor), ny = std::get<1>(neighbor);
    int perf = measure_performance(input), nx, ny, runs);
    if (perf < best_performance) {
      best_performance = perf;
      best_config = neighbor;
    }
  }

  return std::make_tuple(std::get<0>(best_config), std::get<1>(best_config), best_performance);
}

std::tuple<int, int, int> coordinate_descent(int x, int y, int runs) {
  std::cout << "Starting position: (" << x << ", " << y << ") with performance: " << measure_performance(input, x, y, runs) << " ms\n";
  auto current_position = move_to_minimum(x, y, runs);
  int current_performance = std::get<2>(current_position);

  while (true) {
    auto next_position = move_to_minimum(std::get<0>(current_position), std::get<1>(current_position), runs);
    int next_performance = std::get<2>(next_position);
    if (next_performance >= current_performance) {
      return current_position;
    }
    current_position = next_position;
    current_performance = next_performance;

    std::cout << "tile size (x, y): (" << std::get<0>(current_position) << ", " << std::get<1>(current_position)
              << ") with performance: " << current_performance << " ms\n";
  }
}

std::tuple<int, int, int> exhaustive_search(int runs) {
  int best_ts1 = 0, best_ts2 = 0, best_performance = INT_MAX;

  for (int ts1 : allowed_values) {
    for (int ts2 : allowed_values) {
      int performance = measure_performance(input, ts1, ts2, runs);

      std::cout << ts1 << " " << ts2 << " " << performance << "\n";
      if (performance < best_performance) {
        best_performance = performance;
        best_ts1 = ts1;
        best_ts2 = ts2;
      }
    }
  }

  return std::make_tuple(best_ts1, best_ts2, best_performance);
}

std::tuple<int, int, int> combined_exhaustive_search(int runs) {
  int best_ts1 = 0, best_ts2 = 0, best_performance = INT_MAX;

  for (int ts1 : allowed_values) {
    for (int ts2 : allowed_values) {
      int performance = measure_performance(input, ts1, ts2, runs);
      if (performance < best_performance) {
        best_performance = performance;
        best_ts1 = ts1;
        best_ts2 = ts2;
        std::cout << "tile size (x, y): " << best_ts1 << " " << best_ts2 << " " << best_performance << " ms \n";
      }
    }
  }

  return coordinate_descent(best_ts1, best_ts2, runs);
}

void computationKernel2(const std::vector<std::vector<float>>& input, std::vector<std::vector<float>>& output, int tile_size_height, int tile_size_width) {
    int output_height = input.size() - 1;
    int output_width = input[0].size() - 1;

    for (int ii = 0; ii < output_height; ii += tile_size_height) {
        for (int jj = 0; jj < output_width; jj += tile_size_width) {
            for (int i = ii; i < std::min(ii + tile_size_height, output_height); ++i) {
                for (int j = jj; j < std::min(jj + tile_size_width, output_width); ++j) {
                    float max_val = -1e10;
                    for (int ki = 0; ki < 2; ++ki) {
                        for (int kj = 0; kj < 2; ++kj) {
                            float current_val = input[i + ki][j + kj];
                            max_val = std::max(current_val, max_val);
                        }
                    }
                    output[i][j] = max_val;
                }
            }
        }
    }
}
