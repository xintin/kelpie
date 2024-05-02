#include <iostream>
#include <vector>
#include <tuple>
#include <chrono>
#include <algorithm>

const std::vector<int> allowed_values = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512};
const int M = 1000, N = 800, K = 700;


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

std::vector<std::vector<double>> A(M, std::vector<double>(K, 1.0)), B(K, std::vector<double>(N, 2.0)), C(M, std::vector<double>(N, 0.0));

int measure_performance(int ts4, int ts5, int ts6) {
    auto start = std::chrono::high_resolution_clock::now();
    matrix_multiply_tiled_pluto_expt(A, B, C, ts4, ts5, ts6);
    auto stop = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
}

std::vector<std::tuple<int, int, int>> find_neighbors(int x, int y, int z) {
    std::vector<std::tuple<int, int, int>> neighbors;

    for (auto nx : {x}) {
        for (auto ny : {y}) {
            for (auto nz : {z}) {
                if (nx != x || ny != y || nz != z) {
                    neighbors.emplace_back(nx, ny, nz);
                }
            }
        }
    }

    for (int i = 0; i < 3; ++i) {
        int current = (i == 0) ? x : (i == 1) ? y : z;
        std::vector<int>::const_iterator it = std::find(allowed_values.begin(), allowed_values.end(), current);
        if (it != allowed_values.end()) {
            if (it != allowed_values.begin()) {
                int prev = *(it - 1);
                (i == 0) ? neighbors.emplace_back(prev, y, z) : (i == 1) ? neighbors.emplace_back(x, prev, z) : neighbors.emplace_back(x, y, prev);
            }
            if (it + 1 != allowed_values.end()) {
                int next = *(it + 1);
                (i == 0) ? neighbors.emplace_back(next, y, z) : (i == 1) ? neighbors.emplace_back(x, next, z) : neighbors.emplace_back(x, y, next);
            }
        }
    }

    return neighbors;
}

std::tuple<int, int, int, int> move_to_minimum(int x, int y, int z) {
    auto neighbors = find_neighbors(x, y, z);
    int best_performance = measure_performance(x, y, z);
    std::tuple<int, int, int> best_config = std::make_tuple(x, y, z);

    for (auto& neighbor : neighbors) {
        int nx = std::get<0>(neighbor), ny = std::get<1>(neighbor), nz = std::get<2>(neighbor);
        int perf = measure_performance(nx, ny, nz);
        if (perf < best_performance) {
            best_performance = perf;
            best_config = neighbor;
        }
    }

    return std::make_tuple(std::get<0>(best_config), std::get<1>(best_config), std::get<2>(best_config), best_performance);
}

std::tuple<int, int, int, int> coordinate_descent(int x, int y, int z) {
    std::cout << "Starting position: (" << x << ", " << y << ", " << z << ") with performance: " << measure_performance(x, y, z) << " ms\n";
    auto current_position = move_to_minimum(x, y, z);
    int current_performance = std::get<3>(current_position);
    
    while (true) {
        auto next_position = move_to_minimum(std::get<0>(current_position), std::get<1>(current_position), std::get<2>(current_position));
        int next_performance = std::get<3>(next_position);
        if (next_performance >= current_performance) {
            return current_position;
        }
        current_position = next_position;
        current_performance = next_performance;

        std::cout << "Converged to: (" << std::get<0>(current_position) << ", " << std::get<1>(current_position) << ", " << std::get<2>(current_position) << ") with performance: " << current_performance << " ms\n";
    }
}

int main() {
    int x = 32, y = 32, z = 32;  // seed

    auto result = coordinate_descent(x, y, z);
    std::cout << "Converged to: (" << std::get<0>(result) << ", " << std::get<1>(result) << ", " << std::get<2>(result) << ") with performance: " << std::get<3>(result) << " ms\n";
    return 0;
}
