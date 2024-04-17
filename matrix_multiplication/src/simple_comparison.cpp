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

void matrix_multiply_tiled_pluto(const std::vector<std::vector<double>> &A,
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

void matrix_multiply_tiled_ioopt(const std::vector<std::vector<double>> &A,
                                 const std::vector<std::vector<double>> &B,
                                 std::vector<std::vector<double>> &C) {

  size_t tileK = 619;
  size_t tileJ = 230;
  size_t tileI = 140;

  for (size_t k2 = 0; k2 < K; k2 += tileK) {
    size_t Sk2 =
        std::min(tileK, K - k2); // Compute the actual size of the tile in K
    for (size_t j2 = 0; j2 < N; j2 += tileJ) {
      size_t Sj2 =
          std::min(tileJ, N - j2); // Compute the actual size of the tile in J
      for (size_t i2 = 0; i2 < M; i2 += tileI) {
        size_t Si2 =
            std::min(tileI, M - i2); // Compute the actual size of the tile in I
        for (size_t k1 = k2; k1 < k2 + Sk2; k1++) {
          for (size_t j = j2; j < j2 + Sj2; j++) {
            for (size_t i = i2; i < i2 + Si2; i++) {
              // (*this)(i, j) += A(i, k1) * B(k1, j);
              C[i][j] += A[i][k1] * B[k1][j];
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

  start = std::chrono::high_resolution_clock::now();
  matrix_multiply_tiled_pluto(A, B, C_tiled);
  stop = std::chrono::high_resolution_clock::now();

  duration =
      std::chrono::duration_cast<std::chrono::duration<double>>(stop - start)
          .count();
  std::cout << "Time taken by pluto tiled matrix multiplication: " << duration
            << " seconds.\n";

  start = std::chrono::high_resolution_clock::now();
  matrix_multiply_tiled_ioopt(A, B, C_tiled_ioopt);
  stop = std::chrono::high_resolution_clock::now();

  duration =
      std::chrono::duration_cast<std::chrono::duration<double>>(stop - start)
          .count();
  std::cout << "Time taken by ioopt tiled matrix multiplication: " << duration
            << " seconds.\n";

  if (C == C_tiled) {
    std::cout << "The results of both multiplication methods are identical.\n";
  } else {
    std::cout << "The results differ.\n";
  }

  return 0;
}
