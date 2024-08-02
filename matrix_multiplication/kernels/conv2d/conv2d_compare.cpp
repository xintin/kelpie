#include <algorithm>
#include <chrono>   
#include <iostream>
#include <vector>

#define M 1000
#define N 1000
#define F 3

std::vector<std::vector<int>> A(M, std::vector<int>(N, -1)); 
std::vector<std::vector<int>> B(F, std::vector<int>(F, -1));
std::vector<std::vector<int>>
    C(M - F + 1,
      std::vector<int>(N - F + 1, 0));
std::vector<std::vector<int>> C_tiled(
    M - F + 1,
    std::vector<int>(N - F + 1, 0)); 

void initialize_matrices() {
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
      A[i][j] = static_cast<int>(i + j); 
    }
  }

  for (int i = 0; i < F; ++i) {
    for (int j = 0; j < F; ++j) {
      B[i][j] = static_cast<int>(i - j); 
    }
  }
}

void conv2d() {
  for (int i = 0; i <= M - F; ++i) {
    for (int j = 0; j <= N - F; ++j) {
      C[i][j] = 0; 
      for (int ki = 0; ki < F; ++ki) {
        for (int kj = 0; kj < F; ++kj) {
          C[i][j] += A[i + ki][j + kj] * B[ki][kj];
        }
      }
    }
  }
}

void conv2d_tiled(int tile_size_t2, int tile_size_t3) {
  int t2, t3, t4, t5, t6, t7, t8;
  int lbv, ubv;

  if ((F <= N) && (M >= F)) {
    for (t2 = 0; t2 <= (M - F) / tile_size_t2; t2++) {
      for (t3 = 0; t3 <= (N - F) / tile_size_t3; t3++) {
        for (t4 = tile_size_t2 * t2;
             t4 <= std::min(M - F, tile_size_t2 * t2 + tile_size_t2 - 1);
             t4++) {
          lbv = tile_size_t3 * t3;
          ubv = std::min(N - F, tile_size_t3 * t3 + tile_size_t3 - 1);
          for (t5 = lbv; t5 <= ubv; t5++) {
            C_tiled[t4][t5] = 0;
          }
        }
      }
    }
    if (F >= 1) {
      for (t2 = 0; t2 <= (M - F) / tile_size_t2; t2++) {
        for (t3 = 0; t3 <= (N - F) / tile_size_t3; t3++) {
          for (t4 = 0; t4 <= (F - 1) / tile_size_t2; t4++) {
            for (t5 = tile_size_t2 * t2;
                 t5 <= std::min(M - F, tile_size_t2 * t2 + tile_size_t2 - 1);
                 t5++) {
              for (t6 = tile_size_t3 * t3;
                   t6 <= std::min(N - F, tile_size_t3 * t3 + tile_size_t3 - 1);
                   t6++) {
                for (t7 = tile_size_t2 * t4;
                     t7 <=
                     std::min(F - 1, tile_size_t2 * t4 + tile_size_t2 - 1);
                     t7++) {
                  for (t8 = 0; t8 <= F - 1; t8++) {
                    C_tiled[t5][t6] += A[t5 + t7][t6 + t8] * B[t7][t8];
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

bool compare_results(const std::vector<std::vector<int>> &C1,
                     const std::vector<std::vector<int>> &C2) {
  if (C1.size() != C2.size())
    return false;
  for (size_t i = 0; i < C1.size(); ++i) {
    if (C1[i].size() != C2[i].size())
      return false;
    for (size_t j = 0; j < C1[i].size(); ++j) {
      if (C1[i][j] != C2[i][j]) {
        std::cout << "Mismatch at (" << i << ", " << j << "): " << C1[i][j]
                  << " != " << C2[i][j] << std::endl;
        return false;
      }
    }
  }
  return true;
}

int main() {
  initialize_matrices();

  // Measure time for standard conv2d
  auto start = std::chrono::high_resolution_clock::now();
  conv2d();
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> duration = end - start;
  std::cout << "Standard conv2d time taken: " << duration.count() << " seconds"
            << std::endl;

  // Measure time for tiled conv2d with different tile sizes
  std::vector<int> tile_sizes = {32}; //{2, 4, 16, 32};
  for (int tile_size_t2 : tile_sizes) {
    for (int tile_size_t3 : tile_sizes) {
      // Reset C_tiled matrix
      for (auto &row : C_tiled) {
        std::fill(row.begin(), row.end(), 0);
      }

      start = std::chrono::high_resolution_clock::now();
      conv2d_tiled(tile_size_t2, tile_size_t3);
      end = std::chrono::high_resolution_clock::now();
      duration = end - start;

      std::cout << "Tiled conv2d time taken (tile sizes " << tile_size_t2
                << ", " << tile_size_t3 << "): " << duration.count()
                << " seconds" << std::endl;

      // Compare results
      if (compare_results(C, C_tiled)) {
        std::cout << "Results match for tile sizes (" << tile_size_t2 << ", "
                  << tile_size_t3 << ")\n";
      } else {
        std::cout << "Results do NOT match for tile sizes (" << tile_size_t2
                  << ", " << tile_size_t3 << ")\n";
      }
    }
  }

  return 0;
}
