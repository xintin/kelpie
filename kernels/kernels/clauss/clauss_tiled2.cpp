#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>

#define N 10000

inline int ceild(int n, int d) {
  return static_cast<int>(
      std::ceil(static_cast<double>(n) / static_cast<double>(d)));
}
inline int floord(int n, int d) {
  return static_cast<int>(
      std::floor(static_cast<double>(n) / static_cast<double>(d)));
}
inline int max(int x, int y) { return std::max(x, y); }
inline int min(int x, int y) { return std::min(x, y); }

void computationKernel(std::vector<std::vector<int>> &a,
                       std::vector<std::vector<int>> &b) {
  int t1, t2, t3, t4;
  int lbv, ubv;

  if (N >= 1) {
    for (t1 = 0; t1 <= floord(N - 1, 32); t1++) {
      for (t2 = max(0, ceild(-32 * t1 + N - 63, 32));
           t2 <= floord(3 * N - 2, 32); t2++) {
        for (t3 = max(32 * t2, -32 * t1 + N - 32);
             t3 <= min(32 * t2 + 31, -32 * t1 + 2 * N - 33); t3++) {
          lbv = max(32 * t1, -t3 + N - 1);
          ubv = min(N - 1, 32 * t1 + 31);
          for (t4 = lbv; t4 <= ubv; t4++) {
            a[t3][t4] = 0;
          }
        }
        for (t3 = max(32 * t2, -32 * t1 + 2 * N - 32);
             t3 <= min(N - 1, 32 * t2 + 31); t3++) {
          lbv = max(32 * t1, -t3 + N - 1);
          ubv = N - 1;
          for (t4 = lbv; t4 <= ubv; t4++) {
            a[t3][t4] = 0;
          }
        }
        for (t3 = max(max(N, 32 * t2), -32 * t1 + 2 * N - 32);
             t3 <= min(32 * t2 + 31, -32 * t1 + 2 * N - 2); t3++) {
          lbv = 32 * t1;
          ubv = -t3 + 2 * N - 2;
          for (t4 = lbv; t4 <= ubv; t4++) {
            a[t3][t4] = 0;
          }
          lbv = -t3 + 2 * N - 1;
          ubv = min(N - 1, 32 * t1 + 31);
          for (t4 = lbv; t4 <= ubv; t4++) {
            a[t3][t4] = 0;
            b[t3][t4] = a[t3 - N][t4];
          }
        }
        for (t3 = max(32 * t2, -32 * t1 + 2 * N - 1);
             t3 <= min(32 * t2 + 31, -32 * t1 + 3 * N - 33); t3++) {
          lbv = 32 * t1;
          ubv = min(N - 1, 32 * t1 + 31);
          for (t4 = lbv; t4 <= ubv; t4++) {
            a[t3][t4] = 0;
            b[t3][t4] = a[t3 - N][t4];
          }
        }
        for (t3 =
                 max(max(32 * t2, -32 * t1 + 2 * N - 1), -32 * t1 + 3 * N - 32);
             t3 <= min(2 * N - 1, 32 * t2 + 31); t3++) {
          lbv = 32 * t1;
          ubv = N - 1;
          for (t4 = lbv; t4 <= ubv; t4++) {
            a[t3][t4] = 0;
            b[t3][t4] = a[t3 - N][t4];
          }
        }
        for (t3 = max(max(2 * N, 32 * t2), -32 * t1 + 3 * N - 32);
             t3 <= min(32 * t2 + 31, -32 * t1 + 3 * N - 2); t3++) {
          lbv = 32 * t1;
          ubv = -t3 + 3 * N - 2;
          for (t4 = lbv; t4 <= ubv; t4++) {
            a[t3][t4] = 0;
            b[t3][t4] = a[t3 - N][t4];
          }
          lbv = -t3 + 3 * N - 1;
          ubv = min(N - 1, 32 * t1 + 31);
          for (t4 = lbv; t4 <= ubv; t4++) {
            b[t3][t4] = a[t3 - N][t4];
          }
        }
        for (t3 = max(32 * t2, -32 * t1 + 3 * N - 1);
             t3 <= min(3 * N - 2, 32 * t2 + 31); t3++) {
          lbv = 32 * t1;
          ubv = min(N - 1, 32 * t1 + 31);
          for (t4 = lbv; t4 <= ubv; t4++) {
            b[t3][t4] = a[t3 - N][t4];
          }
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