#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>

constexpr int N = 100;

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

bool matricesEqual(const std::vector<std::vector<int>> &a,
                   const std::vector<std::vector<int>> &b) {
  if (a.size() != b.size() || a[0].size() != b[0].size())
    return false;
  for (size_t i = 0; i < a.size(); ++i) {
    for (size_t j = 0; j < a[i].size(); ++j) {
      if (a[i][j] != b[i][j])
        return false;
    }
  }
  return true;
}

void computationKernelUntiled(std::vector<std::vector<int>> &a,
                              std::vector<std::vector<int>> &b) {
  int rows = 3 * N - 1;
  int cols = N;

  a.resize(rows, std::vector<int>(cols, 1));
  b.resize(rows, std::vector<int>(cols, 2));

  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      if ((i + j >= N - 1) && (i + j <= 3 * N - 2)) {
        a[i][j] = 3;
      }
      if ((i + j >= 2 * N - 1) && (i + j <= 4 * N - 2)) {
        if (i >= N) {
          b[i][j] = a[i - N][j];
        }
      }
    }
  }
}

void computationKernelTiled(std::vector<std::vector<int>> &a,
                            std::vector<std::vector<int>> &b) {
  int rows = 3 * N - 1;
  int cols = N;

  a.resize(rows, std::vector<int>(cols, 1));
  b.resize(rows, std::vector<int>(cols, 2));

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
            a[t3][t4] = 3;
          }
        }
        for (t3 = max(32 * t2, -32 * t1 + 2 * N - 32);
             t3 <= min(N - 1, 32 * t2 + 31); t3++) {
          lbv = max(32 * t1, -t3 + N - 1);
          ubv = N - 1;
          for (t4 = lbv; t4 <= ubv; t4++) {
            a[t3][t4] = 3;
          }
        }
        for (t3 = max(max(N, 32 * t2), -32 * t1 + 2 * N - 32);
             t3 <= min(32 * t2 + 31, -32 * t1 + 2 * N - 2); t3++) {
          lbv = 32 * t1;
          ubv = -t3 + 2 * N - 2;
          for (t4 = lbv; t4 <= ubv; t4++) {
            a[t3][t4] = 3;
          }
          lbv = -t3 + 2 * N - 1;
          ubv = min(N - 1, 32 * t1 + 31);
          for (t4 = lbv; t4 <= ubv; t4++) {
            a[t3][t4] = 3;
            b[t3][t4] = a[t3 - N][t4];
          }
        }
        for (t3 = max(32 * t2, -32 * t1 + 2 * N - 1);
             t3 <= min(32 * t2 + 31, -32 * t1 + 3 * N - 33); t3++) {
          lbv = 32 * t1;
          ubv = min(N - 1, 32 * t1 + 31);
          for (t4 = lbv; t4 <= ubv; t4++) {
            a[t3][t4] = 3;
            b[t3][t4] = a[t3 - N][t4];
          }
        }
        for (t3 =
                 max(max(32 * t2, -32 * t1 + 2 * N - 1), -32 * t1 + 3 * N - 32);
             t3 <= min(2 * N - 1, 32 * t2 + 31); t3++) {
          lbv = 32 * t1;
          ubv = N - 1;
          for (t4 = lbv; t4 <= ubv; t4++) {
            a[t3][t4] = 3;
            b[t3][t4] = a[t3 - N][t4];
          }
        }
        for (t3 = max(max(2 * N, 32 * t2), -32 * t1 + 3 * N - 32);
             t3 <= min(32 * t2 + 31, -32 * t1 + 3 * N - 2); t3++) {
          lbv = 32 * t1;
          ubv = -t3 + 3 * N - 2;
          for (t4 = lbv; t4 <= ubv; t4++) {
            a[t3][t4] = 3;
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

void computationKernelTiled2(std::vector<std::vector<int>> &a,
                             std::vector<std::vector<int>> &b, int T1, int T2) {
  int rows = 3 * N - 1;
  int cols = N;

  a.resize(rows, std::vector<int>(cols, 1));
  b.resize(rows, std::vector<int>(cols, 2));

  if (N >= 1) {
    // for (int t1 = 0; t1 <= floord(N - 1, T1); t1++) {
    //       for (int t2 = max(0, ceild(-T1 * t1 + N - 63, T2)); t2 <= floord(3
    //       * N - 2, T2); t2++) {
    //           for (int t3 = max(T2 * t2, -T1 * t1 + N - T2); t3 <= min(T2 *
    //           t2 + T2 - 1, -T1 * t1 + 2 * N - T2 - 1); t3++) {

    for (int t1 = 0; t1 <= floord(N - 1, T1); t1++) {
      for (int t2 = max(0, ceild(-T1 * t1 + N - (2 * T1 - 1), T2));
           t2 <= floord(3 * N - 2, T2); t2++) {
        for (int t3 = max(T2 * t2, -T1 * t1 + N - T1);
             t3 <= min(T2 * t2 + T2 - 1, -T1 * t1 + 2 * N - T1 - 1); t3++) {
          int lbv = max(T1 * t1, -t3 + N - 1);
          int ubv = min(N - 1, T1 * t1 + T1 - 1);
          for (int t4 = lbv; t4 <= ubv; t4++) {
            a[t3][t4] = 3;
          }
        }
        for (int t3 = max(T2 * t2, -T1 * t1 + 2 * N - T2);
             t3 <= min(N - 1, T2 * t2 + T2 - 1); t3++) {
          int lbv = max(T1 * t1, -t3 + N - 1);
          int ubv = N - 1;
          for (int t4 = lbv; t4 <= ubv; t4++) {
            a[t3][t4] = 3;
          }
        }
        for (int t3 = max(max(N, T2 * t2), -T1 * t1 + 2 * N - T2);
             t3 <= min(T2 * t2 + T2 - 1, -T1 * t1 + 2 * N - 2); t3++) {
          int lbv = T1 * t1;
          int ubv = -t3 + 2 * N - 2;
          for (int t4 = lbv; t4 <= ubv; t4++) {
            a[t3][t4] = 3;
          }
          lbv = -t3 + 2 * N - 1;
          ubv = min(N - 1, T1 * t1 + T1 - 1);
          for (int t4 = lbv; t4 <= ubv; t4++) {
            a[t3][t4] = 3;
            b[t3][t4] = a[t3 - N][t4];
          }
        }
        // for (int t3 = max(32 * t2, -32 * t1 + 2 * N - 1); t3 <= min(32 * t2 +
        // 31, -32 * t1 + 3 * N - 33); t3++) {
        for (int t3 = max(T2 * t2, -T1 * t1 + 2 * N - 1);
             t3 <= min(T2 * t2 + T2 - 1, -T1 * t1 + 3 * N - T2 - 1); t3++) {
          int lbv = T1 * t1;
          int ubv = min(N - 1, T1 * t1 + T1 - 1);
          for (int t4 = lbv; t4 <= ubv; t4++) {
            a[t3][t4] = 3;
            b[t3][t4] = a[t3 - N][t4];
          }
        }
        // for (int t3 = max(max(32 * t2, -32 * t1 + 2 * N - 1), -32 * t1 + 3 *
        // N - 32); t3 <= min(2 * N - 1, 32 * t2 + 31); t3++) {
        for (int t3 =
                 max(max(T2 * t2, -T1 * t1 + 2 * N - 1), -T1 * t1 + 3 * N - T2);
             t3 <= min(2 * N - 1, T2 * t2 + T2 - 1); t3++) {
          int lbv = T1 * t1;
          int ubv = N - 1;
          for (int t4 = lbv; t4 <= ubv; t4++) {
            a[t3][t4] = 3;
            b[t3][t4] = a[t3 - N][t4];
          }
        }
        // for (int t3 = max(max(2 * N, 32 * t2), -32 * t1 + 3 * N - 32); t3 <=
        // min(32 * t2 + 31, -32 * t1 + 3 * N - 2); t3++) {
        for (int t3 = max(max(2 * N, T2 * t2), -T1 * t1 + 3 * N - T2);
             t3 <= min(T2 * t2 + T2 - 1, -T1 * t1 + 3 * N - 2); t3++) {
          int lbv = T1 * t1;
          int ubv = -t3 + 3 * N - 2;
          for (int t4 = lbv; t4 <= ubv; t4++) {
            a[t3][t4] = 3;
            b[t3][t4] = a[t3 - N][t4];
          }
          lbv = -t3 + 3 * N - 1;
          ubv = min(N - 1, T1 * t1 + T1 - 1);
          for (int t4 = lbv; t4 <= ubv; t4++) {
            b[t3][t4] = a[t3 - N][t4];
          }
        }
        for (int t3 = max(T2 * t2, -T1 * t1 + 3 * N - 1);
             t3 <= min(3 * N - 2, T2 * t2 + T2 - 1); t3++) {
          int lbv = T1 * t1;
          int ubv = min(N - 1, T1 * t1 + T1 - 1);
          for (int t4 = lbv; t4 <= ubv; t4++) {
            b[t3][t4] = a[t3 - N][t4];
          }
        }
      }
    }
  }
}

void computationKernelTiled3(std::vector<std::vector<int>> &a,
                             std::vector<std::vector<int>> &b, int T1, int T2) {
  int rows = 3 * N - 1;
  int cols = N;

  a.resize(rows, std::vector<int>(cols, 1));
  b.resize(rows, std::vector<int>(cols, 2));

  if (N >= 1) {
    for (int t1 = 0; t1 <= floord(N - 1, T1); t1++) {
      for (int t2 = max(0, ceild(-T1 * t1 + N - (T1 + T2 - 1), T2));
           t2 <= floord(3 * N - 2, T2); t2++) {
        for (int t3 = max(T2 * t2, -T1 * t1 + N - T1);
             t3 <= min(T2 * t2 + T2 - 1, -T1 * t1 + 2 * N - T1 - 1); t3++) {
          int lbv = max(T1 * t1, -t3 + N - 1);
          int ubv = min(N - 1, T1 * t1 + T1 - 1);
          for (int t4 = lbv; t4 <= ubv; t4++) {
            a[t3][t4] = 3;
          }
        }
        for (int t3 = std::max(T2 * t2, -T1 * t1 + 2 * N - T1);
             t3 <= std::min(N - 1, T2 * t2 + T2 - 1); t3++) {
          int lbv = std::max(T1 * t1, -t3 + N - 1);
          int ubv = N - 1;
          for (int t4 = lbv; t4 <= ubv; t4++) {
            a[t3][t4] = 3;
          }
        }
        for (int t3 = std::max(std::max(N, T2 * t2), -T1 * t1 + 2 * N - T1);
             t3 <= std::min(T2 * t2 + T2 - 1, -T1 * t1 + 2 * N - 2); t3++) {
          int lbv = T1 * t1;
          int ubv = -t3 + 2 * N - 2;
          for (int t4 = lbv; t4 <= ubv; t4++) {
            a[t3][t4] = 3;
          }
          lbv = -t3 + 2 * N - 1;
          ubv = std::min(N - 1, T1 * t1 + T1 - 1);
          for (int t4 = lbv; t4 <= ubv; t4++) {
            a[t3][t4] = 3;
            b[t3][t4] = a[t3 - N][t4];
          }
        }
        for (int t3 = std::max(T2 * t2, -T1 * t1 + 2 * N - 1);
             t3 <= std::min(T2 * t2 + T2 - 1, -T1 * t1 + 3 * N - T1 - 1);
             t3++) {
          int lbv = T1 * t1;
          int ubv = std::min(N - 1, T1 * t1 + T1 - 1);
          for (int t4 = lbv; t4 <= ubv; t4++) {
            a[t3][t4] = 3;
            b[t3][t4] = a[t3 - N][t4];
          }
        }
        for (int t3 = std::max(std::max(T2 * t2, -T1 * t1 + 2 * N - 1),
                               -T1 * t1 + 3 * N - T1);
             t3 <= std::min(2 * N - 1, T2 * t2 + T2 - 1); t3++) {
          int lbv = T1 * t1;
          int ubv = N - 1;
          for (int t4 = lbv; t4 <= ubv; t4++) {
            a[t3][t4] = 3;
            b[t3][t4] = a[t3 - N][t4];
          }
        }
        for (int t3 = std::max(std::max(2 * N, T2 * t2), -T1 * t1 + 3 * N - T1);
             t3 <= std::min(T2 * t2 + T2 - 1, -T1 * t1 + 3 * N - 2); t3++) {
          int lbv = T1 * t1;
          int ubv = -t3 + 3 * N - 2;
          for (int t4 = lbv; t4 <= ubv; t4++) {
            a[t3][t4] = 3;
            b[t3][t4] = a[t3 - N][t4];
          }
          lbv = -t3 + 3 * N - 1;
          ubv = std::min(N - 1, T1 * t1 + T1 - 1);
          for (int t4 = lbv; t4 <= ubv; t4++) {
            b[t3][t4] = a[t3 - N][t4];
          }
        }
        for (int t3 = std::max(T2 * t2, -T1 * t1 + 3 * N - 1);
             t3 <= std::min(3 * N - 2, T2 * t2 + T2 - 1); t3++) {
          int lbv = T1 * t1;
          int ubv = std::min(N - 1, T1 * t1 + T1 - 1);
          for (int t4 = lbv; t4 <= ubv; t4++) {
            b[t3][t4] = a[t3 - N][t4];
          }
        }
      }
    }
  }
}

int main() {
  std::vector<std::vector<int>> a_tiled(3 * N - 1, std::vector<int>(N, 1));
  std::vector<std::vector<int>> b_tiled(3 * N - 1, std::vector<int>(N, 2));
  std::vector<std::vector<int>> a_untiled(3 * N - 1, std::vector<int>(N, 1));
  std::vector<std::vector<int>> b_untiled(3 * N - 1, std::vector<int>(N, 2));
  std::vector<std::vector<int>> a_tiled2(3 * N - 1, std::vector<int>(N, 1));
  std::vector<std::vector<int>> b_tiled2(3 * N - 1, std::vector<int>(N, 2));

  a_untiled = a_tiled;
  b_untiled = b_tiled;

  computationKernelTiled(a_tiled, b_tiled);

  // computationKernelUntiled(a_untiled, b_untiled);

  // computationKernelTiled2(a_tiled2, b_tiled2, 64, 32);

  computationKernelTiled3(a_tiled2, b_tiled2, 32, 64);

  // bool result =
  //     matricesEqual(a_tiled, a_untiled) && matricesEqual(b_tiled, b_untiled);
  // std::cout << "The outputs (for untiled and tiled default) are " << (result
  // ? "identical." : "different.")
  //           << std::endl;

  auto result =
      matricesEqual(a_tiled, a_tiled2) && matricesEqual(b_tiled, b_tiled2);
  std::cout << "The outputs (for tiled 1 and 2) are "
            << (result ? "identical." : "different.") << std::endl;

  return 0;
}