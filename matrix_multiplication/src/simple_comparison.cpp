#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>

const int M = 1000;
const int N = 1000;
const int K = 1000;

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

void matrix_multiply_tiled(const std::vector<std::vector<double>> &A,
                           const std::vector<std::vector<double>> &B,
                           std::vector<std::vector<double>> &C) {
  int lbv, ubv;
  int t1, t2, t3, t4, t5, t6;

  for (t1 = 0; t1 <= (M - 1) / 32; t1++) {
    for (t2 = 0; t2 <= (N - 1) / 32; t2++) {
      for (t3 = 0; t3 <= (K - 1) / 32; t3++) {
        for (t4 = 32 * t1; t4 <= std::min(M - 1, 32 * t1 + 31); t4++) {
          for (t5 = 32 * t3; t5 <= std::min(K - 1, 32 * t3 + 31); t5++) {
            lbv = 32 * t2;
            ubv = std::min(N - 1, 32 * t2 + 31);
            for (t6 = lbv; t6 <= ubv; t6++) {
              C[t4][t6] += A[t4][t5] * B[t5][t6];
            }
          }
        }
      }
    }
  }
}

int main() {
  std::vector<std::vector<double>> A(M, std::vector<double>(K, 1.0));
  std::vector<std::vector<double>> B(K, std::vector<double>(N, 2.0));
  std::vector<std::vector<double>> C(M, std::vector<double>(N, 0.0));
  std::vector<std::vector<double>> C_tiled(M, std::vector<double>(N, 0.0));

  auto start = std::chrono::high_resolution_clock::now();
  matrix_multiply_naive(A, B, C);
  auto stop = std::chrono::high_resolution_clock::now();

  auto duration =
      std::chrono::duration_cast<std::chrono::duration<double>>(stop - start)
          .count();
  std::cout << "Time taken by naive matrix multiplication: " << duration
            << " seconds.\n";

  start = std::chrono::high_resolution_clock::now();
  matrix_multiply_tiled(A, B, C_tiled);
  stop = std::chrono::high_resolution_clock::now();

  duration =
      std::chrono::duration_cast<std::chrono::duration<double>>(stop - start)
          .count();
  std::cout << "Time taken by tiled matrix multiplication: " << duration
            << " seconds.\n";

  if (C == C_tiled) {
    std::cout << "The results of both multiplication methods are identical.\n";
  } else {
    std::cout << "The results differ.\n";
  }

  return 0;
}
