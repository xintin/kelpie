#include "matrix.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <vector>

void MatrixTester::testTiledMM(const Matrix &A, const Matrix &B,
                               int maxTileSize) {
  std::vector<size_t> tileSizes;

  for (size_t size = 8; size <= maxTileSize; size += 8) {
    tileSizes.push_back(size);
  }

  Matrix C_gold(A.getRows(), B.getCols());
  C_gold.mm(A, B);

  // Test each combination of tile sizes
  for (size_t tileK : tileSizes) {
    for (size_t tileJ : tileSizes) {
      for (size_t tileI : tileSizes) {
        Matrix C_tiled(A.getRows(), B.getCols());

        std::cout << "Testing with Tile Sizes - K: " << tileK
                  << ", J: " << tileJ << ", I: " << tileI << "\n";

        // time taken for each configuration
        auto start = std::chrono::high_resolution_clock::now();
        MatrixTester::tiledMM_kernel1(C_tiled, A, B, tileK, tileJ, tileI);
        auto stop = std::chrono::high_resolution_clock::now();

        double duration =
            std::chrono::duration_cast<std::chrono::duration<double>>(stop -
                                                                      start)
                .count();
        std::cout << "Duration: " << duration << " seconds." << std::endl;

        if (Matrix::matricesAreEqual(C_gold, C_tiled)) {
          continue;
        } else {
          std::cout << "The results differ.\n";
        }
      }
    }
  }
}

void MatrixTester::tiledMM_kernel1(Matrix &C, const Matrix &A, const Matrix &B,
                                   size_t tileK, size_t tileJ, size_t tileI) {
  if (A.getCols() != B.getRows()) {
    throw std::invalid_argument(
        "Matrices A and B dimensions do not allow multiplication.");
  }

  size_t M = A.getRows();
  size_t K = A.getCols();
  size_t N = B.getCols();

  for (size_t k2 = 0; k2 < K; k2 += tileK) {
    size_t Sk2 = std::min(tileK, K - k2);
    for (size_t j2 = 0; j2 < N; j2 += tileJ) {
      size_t Sj2 = std::min(tileJ, N - j2);
      for (size_t i2 = 0; i2 < M; i2 += tileI) {
        size_t Si2 = std::min(tileI, M - i2);
        for (size_t k1 = k2; k1 < k2 + Sk2; k1++) {
          for (size_t j = j2; j < j2 + Sj2; j++) {
            for (size_t i = i2; i < i2 + Si2; i++) {
              C(i, j) += A(i, k1) * B(k1, j);
            }
          }
        }
      }
    }
  }
}
