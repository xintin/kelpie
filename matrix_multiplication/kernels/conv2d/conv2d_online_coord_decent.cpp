#include <algorithm>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <tuple>
#include <vector>

#define M 1000
#define N 1000
#define F 3 

std::vector<std::vector<int>> A(M, std::vector<int>(N, -1)); 
std::vector<std::vector<int>> B(F, std::vector<int>(F, -1)); 
std::vector<std::vector<int>>
    C(M - F + 1,
      std::vector<int>(N - F + 1, 0)); 

const std::vector<int> allowed_values = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512};

inline int ceild(int n, int d) {
  return static_cast<int>(
      std::ceil(static_cast<double>(n) / static_cast<double>(d)));
}
inline int floord(int n, int d) {
  return static_cast<int>(
      std::floor(static_cast<double>(n) / static_cast<double>(d)));
}
inline int max(int x, int y) { return std::max(x, y); }
inline int min(int x, int y) { return std::min(x, y); }

void initialize_matrices() {
  // Initialize input matrix A with some values
  for (int i = 0; i < M; ++i) {
    for (int j = 0; j < N; ++j) {
      A[i][j] = static_cast<int>(i + j); 
    }
  }

  // Initialize filter matrix B with some values
  for (int i = 0; i < F; ++i) {
    for (int j = 0; j < F; ++j) {
      B[i][j] = static_cast<int>(i - j);
    }
  }
}

void computationKernel2(std::vector<std::vector<int>> &A,
                        std::vector<std::vector<int>> &B, int T1, int T2);

int measure_performance(int ts1, int ts2, int runs) {
  long long total_duration = 0;

  for (int i = 0; i < runs; ++i) {
    auto start = std::chrono::high_resolution_clock::now();
    computationKernel2(A, B, ts1, ts2);
    auto stop = std::chrono::high_resolution_clock::now();

    total_duration +=
        std::chrono::duration_cast<std::chrono::milliseconds>(stop - start)
            .count();
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
  int best_performance = measure_performance(x, y, runs);
  std::tuple<int, int> best_config = std::make_tuple(x, y);

  for (auto &neighbor : neighbors) {
    int nx = std::get<0>(neighbor), ny = std::get<1>(neighbor);
    int perf = measure_performance(nx, ny, runs);
    if (perf < best_performance) {
      best_performance = perf;
      best_config = neighbor;
    }
  }

  return std::make_tuple(std::get<0>(best_config), std::get<1>(best_config),
                         best_performance);
}

std::tuple<int, int, int> coordinate_descent(int x, int y, int runs) {
  std::cout << "Starting position: (" << x << ", " << y
            << ") with performance: " << measure_performance(x, y, runs)
            << " ms\n";
  auto current_position = move_to_minimum(x, y, runs);
  int current_performance = std::get<2>(current_position);

  while (true) {
    auto next_position = move_to_minimum(std::get<0>(current_position),
                                         std::get<1>(current_position), runs);
    int next_performance = std::get<2>(next_position);
    if (next_performance >= current_performance) {
      return current_position;
    }
    current_position = next_position;
    current_performance = next_performance;

    std::cout << "tile size (x, y): (" << std::get<0>(current_position) << ", "
              << std::get<1>(current_position)
              << ") with performance: " << current_performance << " ms\n";
  }
}

std::tuple<int, int, int> exhaustive_search(int runs) {
  int best_ts1 = 0, best_ts2 = 0, best_performance = INT_MAX;

  for (int ts1 : allowed_values) {
    for (int ts2 : allowed_values) {
      int performance = measure_performance(ts1, ts2, runs);

      std::cout << ts1 << " " << ts2 << " " << performance << "\n";
      if (performance < best_performance) {
        best_performance = performance;
        best_ts1 = ts1;
        best_ts2 = ts2;
        // std::cout << "tile size (x, y): " << best_ts1 << " " << best_ts2 << "
        // "
        //          << best_performance << "ms \n";
      }
    }
  }

  return std::make_tuple(best_ts1, best_ts2, best_performance);
}

std::tuple<int, int, int> combined_exhaustive_search(int runs) {
  int best_ts1 = 0, best_ts2 = 0, best_performance = INT_MAX;

  for (int ts1 : allowed_values) {
    for (int ts2 : allowed_values) {
      int performance = measure_performance(ts1, ts2, runs);
      if (performance < best_performance) {
        best_performance = performance;
        best_ts1 = ts1;
        best_ts2 = ts2;
        std::cout << "tile size (x, y): " << best_ts1 << " " << best_ts2 << " "
                  << best_performance << "ms \n";
      }
    }
  }

  return coordinate_descent(best_ts1, best_ts2, runs);
}

int main(int argc, char *argv[]) {
  int runs = std::atoi(argv[1]);
  if (runs <= 0) {
    std::cerr << "Number of runs must be a positive integer.\n";
    return 2;
  }

  initialize_matrices();
  int x = 1, y = 1;

  std::cout << "\nCoordinate Descent:"
            << "\n";
  auto result = coordinate_descent(x, y, runs);
  std::cout << "Converged to: (" << std::get<0>(result) << ", "
            << std::get<1>(result)
            << ") with performance: " << std::get<2>(result) << " ms\n";

  std::cout << "\nExhaustive search:"
            << "\n";
  result = exhaustive_search(runs);
  std::cout << "Converged to: (" << std::get<0>(result) << ", "
            << std::get<1>(result)
            << ") with performance: " << std::get<2>(result) << "ms\n";

  // std::cout << "\nCombined search:"
  //          << "\n";
  // result = combined_exhaustive_search(runs);
  // std::cout << "Converged to: (" << std::get<0>(result) << ", "
  //           << std::get<1>(result)
  //          << ") with performance: " << std::get<2>(result) << " ms\n";

  return 0;
}

void computationKernel2(std::vector<std::vector<int>> &A,
                        std::vector<std::vector<int>> &B, int tile_size_t2,
                        int tile_size_t3) {
  int t2, t3, t4, t5, t6, t7, t8;
  int lbv, ubv;

  if ((F <= N) && (M >= F)) {
    for (t2 = 0; t2 <= (M - F) / tile_size_t2; t2++) {
      for (t3 = 0; t3 <= (N - F) / tile_size_t3; t3++) {
        for (t4 = tile_size_t2 * t2;
             t4 <= min(M - F, tile_size_t2 * t2 + tile_size_t2 - 1); t4++) {
          lbv = tile_size_t3 * t3;
          ubv = min(N - F, tile_size_t3 * t3 + tile_size_t3 - 1);

          for (t5 = lbv; t5 <= ubv; t5++) {
            C[t4][t5] = 0;
          }
        }
      }
    }
    if (F >= 1) {
      for (t2 = 0; t2 <= (M - F) / tile_size_t2; t2++) {
        for (t3 = 0; t3 <= (N - F) / tile_size_t3; t3++) {
          for (t4 = 0; t4 <= (F - 1) / tile_size_t2; t4++) {
            for (t5 = tile_size_t2 * t2;
                 t5 <= min(M - F, tile_size_t2 * t2 + tile_size_t2 - 1); t5++) {
              for (t6 = tile_size_t3 * t3;
                   t6 <= min(N - F, tile_size_t3 * t3 + tile_size_t3 - 1);
                   t6++) {
                for (t7 = tile_size_t2 * t4;
                     t7 <= min(F - 1, tile_size_t2 * t4 + tile_size_t2 - 1);
                     t7++) {
                  for (t8 = 0; t8 <= F - 1; t8++) {
                    C[t5][t6] += A[t5 + t7][t6 + t8] * B[t7][t8];
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}
