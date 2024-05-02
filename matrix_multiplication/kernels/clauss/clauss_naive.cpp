#include <chrono>
#include <iostream>
#include <vector>

#define N 10000

void computationKernelUntiled(std::vector<std::vector<int>> &a,
                              std::vector<std::vector<int>> &b) {
  int rows = 3 * N - 1;
  int cols = N;

  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      if ((i + j >= N - 1) && (i + j <= 3 * N - 2)) {
        a[i][j] = 3; // modified this to see some values, else all are 0
      }
      if ((i + j >= 2 * N - 1) && (i + j <= 4 * N - 2)) {
        if (i >= N) {
          b[i][j] = a[i - N][j];
        }
      }
    }
  }
}

int main() {
  std::vector<std::vector<int>> a(3 * N - 1, std::vector<int>(N, 0));
  std::vector<std::vector<int>> b(3 * N - 1, std::vector<int>(N, 0));

  auto start = std::chrono::high_resolution_clock::now();
  computationKernelUntiled(a, b);
  auto stop = std::chrono::high_resolution_clock::now();

  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

  std::cout << "Kernel execution time: " << duration.count() << " milliseconds."
            << std::endl;

  // for (int i = 0; i < b.size(); ++i) {
  //     for (int j = 0; j < b[i].size(); ++j) {
  //           std::cout << b[i][j] << " ";
  //     }
  //     std::cout << "\n";
  // }

  return 0;
}
