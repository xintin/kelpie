#include "matrix.h"
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>

int main(int argc, char *argv[]) {
  if (argc < 4) {
    std::cerr << "Usage: " << argv[0] << " rowsA colsA colsB\n";
    return 1;
  }

  size_t rowsA = std::atoi(argv[1]);
  size_t colsA = std::atoi(argv[2]);
  size_t colsB = std::atoi(argv[3]);

  auto rowsB = colsA;

  Matrix A(rowsA, colsA, 1.0);
  Matrix B(rowsB, colsB, 3.0);
  Matrix C(rowsA, colsB);
  Matrix C_tiled(rowsA, colsB);

  // Start timing
  auto start = std::chrono::high_resolution_clock::now();
  C.mm(A, B);
  auto stop = std::chrono::high_resolution_clock::now();

  auto duration =
      std::chrono::duration_cast<std::chrono::duration<double>>(stop - start)
          .count();

  std::cout << std::fixed << std::setprecision(6)
            << "Time taken by matrix multiplication: " << duration
            << " seconds.\n";

  // Timing tiled matrix multiplication
  start = std::chrono::high_resolution_clock::now();
  C_tiled.tiledMM_kernel1(A, B);
  stop = std::chrono::high_resolution_clock::now();
  duration =
      std::chrono::duration_cast<std::chrono::duration<double>>(stop - start)
          .count();

  std::cout << std::fixed << std::setprecision(6)
            << "Time taken by tiled matrix multiplication: " << duration
            << " seconds.\n";

  if (Matrix::matricesAreEqual(C, C_tiled)) {
    std::cout << "The results of both multiplication methods are identical.\n";
  } else {
    std::cout << "The results differ.\n";
  }

  std::cout << "Testing tile sizes:\n";

  int maxTileSize = 256;
  MatrixTester::testTiledMM(A, B, maxTileSize);

  // C.print();

  return 0;
}
