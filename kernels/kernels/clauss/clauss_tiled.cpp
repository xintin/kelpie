#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>

// constexpr int N = 10000;
#define N 10000

void computationKernel(std::vector<std::vector<int>> &a,
                       std::vector<std::vector<int>> &b) {

  for (int t1 = 0; t1 <= static_cast<int>(std::floor((N - 1) / 32.0)); t1++) {
    for (int t2 = std::max(
             0, static_cast<int>(std::ceil((-32 * t1 + N - 63) / 32.0)));
         t2 <= static_cast<int>(std::floor((3 * N - 2) / 32.0)); t2++) {
      for (int t3 = std::max(32 * t2, -32 * t1 + N - 32);
           t3 <= std::min(32 * t2 + 31, -32 * t1 + 2 * N - 33); t3++) {
        for (int t4 = std::max(32 * t1, -t3 + N - 1);
             t4 <= std::min(N - 1, 32 * t1 + 31); t4++) {
          a[t3][t4] = 3;
        }
      }
      for (int t3 = std::max(32 * t2, -32 * t1 + 2 * N - 32);
           t3 <= std::min(N - 1, 32 * t2 + 31); t3++) {
        for (int t4 = std::max(32 * t1, -t3 + N - 1); t4 <= N - 1; t4++) {
          a[t3][t4] = 3;
        }
      }
      for (int t3 = std::max(std::max(N, 32 * t2), -32 * t1 + 2 * N - 32);
           t3 <= std::min(32 * t2 + 31, -32 * t1 + 2 * N - 2); t3++) {
        for (int t4 = 32 * t1; t4 <= -t3 + 2 * N - 2; t4++) {
          a[t3][t4] = 3;
        }
        for (int t4 = -t3 + 2 * N - 1; t4 <= std::min(N - 1, 32 * t1 + 31);
             t4++) {
          a[t3][t4] = 3;
          b[t3][t4] = a[t3 - N][t4];
        }
      }
      for (int t3 = std::max(32 * t2, -32 * t1 + 2 * N - 1);
           t3 <= std::min(32 * t2 + 31, -32 * t1 + 3 * N - 33); t3++) {
        for (int t4 = 32 * t1; t4 <= std::min(N - 1, 32 * t1 + 31); t4++) {
          a[t3][t4] = 3;
          b[t3][t4] = a[t3 - N][t4];
        }
      }
      for (int t3 = std::max(std::max(32 * t2, -32 * t1 + 2 * N - 1),
                             -32 * t1 + 3 * N - 32);
           t3 <= std::min(2 * N - 1, 32 * t2 + 31); t3++) {
        for (int t4 = 32 * t1; t4 <= N - 1; t4++) {
          a[t3][t4] = 3;
          b[t3][t4] = a[t3 - N][t4];
        }
      }
      for (int t3 = std::max(std::max(2 * N, 32 * t2), -32 * t1 + 3 * N - 32);
           t3 <= std::min(32 * t2 + 31, -32 * t1 + 3 * N - 2); t3++) {
        for (int t4 = 32 * t1; t4 <= -t3 + 3 * N - 2; t4++) {
          a[t3][t4] = 3;
          b[t3][t4] = a[t3 - N][t4];
        }
        for (int t4 = -t3 + 3 * N - 1; t4 <= std::min(N - 1, 32 * t1 + 31);
             t4++) {
          b[t3][t4] = a[t3 - N][t4];
        }
      }
      for (int t3 = std::max(32 * t2, -32 * t1 + 3 * N - 1);
           t3 <= std::min(3 * N - 2, 32 * t2 + 31); t3++) {
        for (int t4 = 32 * t1; t4 <= std::min(N - 1, 32 * t1 + 31); t4++) {
          b[t3][t4] = a[t3 - N][t4];
        }
      }
    }
  }
}

int main() {
  std::vector<std::vector<int>> a(3 * N - 1, std::vector<int>(N, 0));
  std::vector<std::vector<int>> b(3 * N - 1, std::vector<int>(N, 0));
  auto start = std::chrono::high_resolution_clock::now();

  computationKernel(a, b);

  auto stop = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

  std::cout << "Kernel execution time: " << duration.count() << " milliseconds."
            << std::endl;

  // for (int i = 0; i < a.size(); ++i) {
  //     for (int j = 0; j < a[i].size(); ++j) {
  //           std::cout << a[i][j] << " ";
  //     }
  //     std::cout << "\n";
  // }

  return 0;
}