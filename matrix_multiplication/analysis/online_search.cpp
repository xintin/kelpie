#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>

const int M = 1000;
const int N = 800;
const int K = 700;

bool isValid(std::vector<std::vector<double>> C,
             std::vector<std::vector<double>> C_tiled) {
  if (C != C_tiled)
    return false;
  return true;
}

void matrix_multiply_naive(const std::vector<std::vector<double>> &A,
                           const std::vector<std::vector<double>> &B,
                           std::vector<std::vector<double>> &C) {
  for (size_t i = 0; i < M; ++i) {
    for (size_t j = 0; j < N; ++j) {
      for (size_t k = 0; k < K; ++k) {
        C[i][j] += A[i][k] * B[k][j];
      }
    }
  }
}

void matrix_multiply_tiled_pluto_expt(const std::vector<std::vector<double>> &A,
                                      const std::vector<std::vector<double>> &B,
                                      std::vector<std::vector<double>> &C,
                                      const int T4, const int T5,
                                      const int T6) {

  int lbv, ubv;
  int t1, t2, t3, t4, t5, t6;

  for (t1 = 0; t1 <= (M - 1) / T4; t1++) {
    for (t2 = 0; t2 <= (N - 1) / T5; t2++) {
      for (t3 = 0; t3 <= (K - 1) / T6; t3++) {
        for (t4 = T4 * t1; t4 <= std::min(M - 1, T4 * t1 + T4 - 1); t4++) {
          for (t5 = T6 * t3; t5 <= std::min(K - 1, T6 * t3 + T6 - 1); t5++) {
            lbv = T5 * t2;
            ubv = std::min(N - 1, T5 * t2 + T5 - 1);
            for (t6 = lbv; t6 <= ubv; t6++) {
              C[t4][t6] += A[t4][t5] * B[t5][t6];
            }
          }
        }
      }
    }
  }
}

std::vector<std::tuple<int, int, int>>
get_neighbors(int i, int j, int k, const std::vector<int> &candidates) {
  std::vector<std::tuple<int, int, int>> neighbors;

  int csize = static_cast<int>(candidates.size());

  if (i > 0)
    neighbors.emplace_back(i - 1, j, k);
  if (i < csize - 1)
    neighbors.emplace_back(i + 1, j, k);

  if (j > 0)
    neighbors.emplace_back(i, j - 1, k);
  if (j < csize - 1)
    neighbors.emplace_back(i, j + 1, k);

  if (k > 0)
    neighbors.emplace_back(i, j, k - 1);
  if (k < csize - 1)
    neighbors.emplace_back(i, j, k + 1);

  return neighbors;
}


double measure_performance(const std::vector<std::vector<double>>& A,
                           const std::vector<std::vector<double>>& B,
                           std::vector<std::vector<double>>& C,
                           int ts4, int ts5, int ts6) {
    auto start = std::chrono::high_resolution_clock::now();
    matrix_multiply_tiled_pluto_expt(A, B, C, ts4, ts5, ts6);
    auto stop = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::duration<double>>(stop - start).count();
}

void run_experiment_greedy(const std::vector<std::vector<double>> &A,
                           const std::vector<std::vector<double>> &B,
                           std::vector<std::vector<double>> &C) {
  std::vector<int> search_candidates = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512};
  double best_duration = std::numeric_limits<double>::max();
  std::tuple<int, int, int> best_config;

  std::vector<std::vector<double>> C_reference(M, std::vector<double>(N, 0.0));
  matrix_multiply_naive(A, B, C_reference);

  for (int ts4 : search_candidates) {
    for (int ts5 : search_candidates) {
      for (int ts6 : search_candidates) {
        C.assign(M, std::vector<double>(N, 0.0));
        auto duration = measure_performance(A, B, C, ts4, ts5, ts6);

        if (C_reference==C && duration < best_duration) {
          best_duration = duration;
          best_config = std::make_tuple(ts4, ts5, ts6);
          std::cout << std::get<0>(best_config) << " " << std::get<1>(best_config) << " " << std::get<2>(best_config) << " " << best_duration << " " << "\n";
        }
      }
    }
  }

  std::cout << "Best configuration: ts4=" << std::get<0>(best_config)
            << ", ts5=" << std::get<1>(best_config)
            << ", ts6=" << std::get<2>(best_config)
            << " with duration: " << best_duration << " ms\n";
}


int binary_search_optimization(const std::vector<std::vector<double>>& A,
                               const std::vector<std::vector<double>>& B,
                               std::vector<std::vector<double>>& C,
                               std::vector<int>& candidates,
                               int fixed_ts5, int fixed_ts6,
                               bool isTs4) {
    int low = 0;
    int high = candidates.size() - 1;
    double best_duration = std::numeric_limits<double>::max();
    int best_ts = candidates[0];

    while (low <= high) {
        int mid = low + (high - low) / 2;
        int ts = candidates[mid];

        C.assign(M, std::vector<double>(N, 0.0));
        auto duration = measure_performance(A, B, C, isTs4 ? ts : fixed_ts5, isTs4 ? fixed_ts5 : ts, fixed_ts6);

        if (duration < best_duration) {
            best_duration = duration;
            best_ts = ts;
            std::cout << best_duration << " " << ts << "\n";
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }
    return best_ts;
}

int optimize_dimension(const std::vector<std::vector<double>>& A,
                       const std::vector<std::vector<double>>& B,
                       std::vector<std::vector<double>>& C,
                       const std::vector<int>& candidates,
                       int& ts4, int& ts5, int& ts6, char dimension) {
    int low = 0, high = candidates.size() - 1;
    double best_duration = std::numeric_limits<double>::max();
    int best_value = candidates[low];

    while (low <= high) {
        int mid = low + (high - low) / 2;
        // C.assign(M, std::vector<double>(N, 0.0));
        auto duration = measure_performance(A, B, C, 
                     dimension == '4' ? candidates[mid] : ts4,
                     dimension == '5' ? candidates[mid] : ts5,
                     dimension == '6' ? candidates[mid] : ts6);

        if (duration < best_duration) {
            best_duration = duration;
            best_value = candidates[mid];
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }

    return best_value;
}

void run_experiment(const std::vector<std::vector<double>>& A,
                    const std::vector<std::vector<double>>& B,
                    std::vector<std::vector<double>>& C) {
    std::vector<int> search_candidates = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512};
   
    int ts4 = 32, ts5 = 32, ts6 = 32; 

    for (int iteration = 0; iteration < 3; ++iteration) {
        ts4 = optimize_dimension(A, B, C, search_candidates, ts4, ts5, ts6, '4');
        ts5 = optimize_dimension(A, B, C, search_candidates, ts4, ts5, ts6, '5');
        ts6 = optimize_dimension(A, B, C, search_candidates, ts4, ts5, ts6, '6');
    }

    std::cout << "Best configuration: ts4=" << ts4
              << ", ts5=" << ts5
              << ", ts6=" << ts6 << "\n";
}



int main() {
  std::vector<std::vector<double>> A(M, std::vector<double>(K, 1.0));
  std::vector<std::vector<double>> B(K, std::vector<double>(N, 2.0));
  std::vector<std::vector<double>> C(M, std::vector<double>(N, 0.0));
  std::vector<std::vector<double>> C_tiled(M, std::vector<double>(N, 0.0));
  std::vector<std::vector<double>> C_tiled_ioopt(M,
                                                 std::vector<double>(N, 0.0));

  auto start = std::chrono::high_resolution_clock::now();
  matrix_multiply_naive(A, B, C);
  auto stop = std::chrono::high_resolution_clock::now();

  auto duration =
      std::chrono::duration_cast<std::chrono::duration<double>>(stop - start)
          .count();
  std::cout << "Time taken by naive matrix multiplication: " << duration
            << " seconds.\n";

  C_tiled.assign(M, std::vector<double>(N, 0.0));
  start = std::chrono::high_resolution_clock::now();
  matrix_multiply_tiled_pluto_expt(A, B, C_tiled, 32, 32, 32);
  stop = std::chrono::high_resolution_clock::now();
  duration =
      std::chrono::duration_cast<std::chrono::duration<double>>(stop - start)
          .count();
  std::cout << "time taken by pluto tiled mm (32, 32, 32): " << duration
            << " seconds.\n";

  if (C != C_tiled) {
    std::cout << "The results differ.\n";
  }

  C_tiled.assign(M, std::vector<double>(N, 0.0));
  start = std::chrono::high_resolution_clock::now();
  matrix_multiply_tiled_pluto_expt(A, B, C_tiled, 128, 128, 16);
  stop = std::chrono::high_resolution_clock::now();
  duration =
      std::chrono::duration_cast<std::chrono::duration<double>>(stop - start)
          .count();
  std::cout << "time taken by pluto tiled mm (128, 128, 16): " << duration
            << " seconds.\n";

  if (C != C_tiled) {
    std::cout << "The results differ.\n";
  }

  run_experiment_greedy(A, B, C_tiled);
  // run_experiment(A, B, C_tiled);

  return 0;
}
