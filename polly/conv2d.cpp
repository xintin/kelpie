#include <algorithm>
#include <chrono>   
#include <iostream>
#include <vector>

#define M 1000
#define N 1000
#define F 3

using namespace std;
using namespace std::chrono;

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


int main() {
  initialize_matrices();

  auto start = std::chrono::high_resolution_clock::now();
  conv2d();
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = duration_cast<std::chrono::milliseconds>(end - start);
  std::cout << "Standard conv2d time taken: " << duration.count() << " milliseconds"
            << std::endl;

  return 0;
}
