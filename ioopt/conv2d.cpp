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
std::vector<std::vector<int>> C(M - F + 1, std::vector<int>(N - F + 1, 0));
std::vector<std::vector<int>> C_tiled(M - F + 1, std::vector<int>(N - F + 1, 0));

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

void conv2d_ioopt() {
    for (int j3 = 0; j3 < N - F + 1; j3 += 114) {
        for (int i3 = 0; i3 < M - F + 1; i3 += 114) {
            int Sj3 = min(114, N - F + 1 - j3);  // Values in {88, 114}
            for (int j2 = j3; j2 < j3 + Sj3; j2 += 109) {
                int Si3 = min(114, M - F + 1 - i3);  // Values in {88, 114}
                for (int i2 = i3; i2 < i3 + Si3; i2 += 109) {
                    int Sj2 = min(109, j3 + Sj3 - j2);  // Values in {88, 5, 109}
                    for (int j1 = j2; j1 < j2 + Sj2; j1 += 12) {
                        int Si2 = min(109, i3 + Si3 - i2);  // Values in {88, 5, 109}
                        for (int i1 = i2; i1 < i2 + Si2; i1 += 12) {
                            for (int kj = 0; kj < F; kj += 1) {
                                for (int ki = 0; ki < F; ki += 1) {
                                    int Sj1 = min(12, j2 + Sj2 - j1);  // Values in {1, 5, 12, 4}
                                    for (int j = j1; j < j1 + Sj1; j += 1) {
                                        int Si1 = min(12, i2 + Si2 - i1);  // Values in {1, 5, 12, 4}
                                        for (int i = i1; i < i1 + Si1; i += 1) {
                                            C_tiled[i][j] += A[i + ki][j + kj] * B[ki][kj];
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
}

int main() {
    initialize_matrices();

    auto start_standard = std::chrono::high_resolution_clock::now();
    conv2d_ioopt();
    auto end_standard = std::chrono::high_resolution_clock::now();
    auto duration_standard = duration_cast<std::chrono::microseconds>(end_standard - start_standard);
    std::cout << "conv2d time taken: " << duration_standard.count() << " microseconds" << std::endl;

    return 0;
}