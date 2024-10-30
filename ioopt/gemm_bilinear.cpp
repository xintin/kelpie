#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <ctime>
#include <chrono>

#define alpha 1.0
#define beta 0.4
#define input_height 1000
#define input_width 1000

using namespace std;
using namespace std::chrono;

inline int ceild(int n, int d) {
    return static_cast<int>(ceil(static_cast<double>(n) / static_cast<double>(d)));
}

inline int floord(int n, int d) {
    return static_cast<int>(floor(static_cast<double>(n) / static_cast<double>(d)));
}

inline int max(int x, int y) { return std::max(x, y); }
inline int min(int x, int y) { return std::min(x, y); }

vector<vector<float>> output_naive(input_height, vector<float>(input_width, 0));
vector<vector<float>> output_tiled(input_height, vector<float>(input_width, 0));

void gemm_bilinear_ioopt(const vector<vector<float>>& A, const vector<vector<float>>& B) {
    // Temporary matrix to store the result of alpha * A * B
    vector<vector<float>> temp(input_height, vector<float>(input_width, 0.0));

    for (int p3 = 0; p3 < input_width; p3 += 251)
        for (int j3 = 0; j3 < input_width; j3 += 54)
            for (int i3 = 0; i3 < input_height; i3 += 36) {
                int Si3 = min(36, input_height - i3);
                for (int i2 = i3; i2 < i3 + Si3; i2 += 23) {
                    int Sj3 = min(54, input_width - j3);
                    for (int j1 = j3; j1 < j3 + Sj3; j1 += 16) {
                        int Si2 = min(23, i3 + Si3 - i2);
                        for (int i1 = i2; i1 < i2 + Si2; i1 += 16) {
                            int Sp3 = min(251, input_width - p3);
                            for (int p1 = p3; p1 < p3 + Sp3; p1 += 1) {
                                int Sj1 = min(16, j3 + Sj3 - j1);
                                for (int j = j1; j < j1 + Sj1; j += 1) {
                                    int Si1 = min(16, i2 + Si2 - i1);
                                    for (int i = i1; i < i1 + Si1; i += 1) {
                                        temp[i][j] += alpha * A[i][p1] * B[p1][j];
                                    }
                                }
                            }
                            // Update matrix C with alpha * A * B + beta * C after tiling
                            for (int i = i1; i < i1 + Si2 && i < input_height; i++) {
                                for (int j = j1; j < j1 + Sj3 && j < input_width; j++) {
                                    output_tiled[i][j] = temp[i][j] + beta * output_tiled[i][j];
                                }
                            }
                        }
                    }
                }
            }
}

// Naive version of gemm_bilinear
void gemm_bilinear_naive(const vector<vector<float>>& A, const vector<vector<float>>& B) {
    vector<vector<float>> temp(input_height, vector<float>(input_width, 0.0));

    for (int i = 0; i < input_height; ++i) {
        for (int j = 0; j < input_width; ++j) {
            for (int p = 0; p < input_height; ++p) {
                temp[i][j] += alpha * A[i][p] * B[p][j];
            }
        }
    }

    for (int i = 0; i < input_height; ++i) {
        for (int j = 0; j < input_width; ++j) {
            output_naive[i][j] = temp[i][j] + beta * output_naive[i][j];
        }
    }
}

int main() {
    srand(time(0));

    vector<vector<float>> A(input_height, vector<float>(input_width));
    vector<vector<float>> B(input_height, vector<float>(input_width));
    for (int i = 0; i < input_height; ++i) {
        for (int j = 0; j < input_width; ++j) {
            A[i][j] = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 100;
            B[i][j] = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 100;
        }
    }

    auto start_naive = high_resolution_clock::now();
    gemm_bilinear_naive(A, B);
    auto end_naive = high_resolution_clock::now();
    auto duration_naive = duration_cast<milliseconds>(end_naive - start_naive);
    cout << "Naive version took " << duration_naive.count() << " milliseconds." << endl;

    auto start_tiled = high_resolution_clock::now();
    gemm_bilinear_ioopt(A, B);
    auto end_tiled = high_resolution_clock::now();
    auto duration_tiled = duration_cast<milliseconds>(end_tiled - start_tiled);
    cout << "IOOPT version took " << duration_tiled.count() << " milliseconds." << endl;

    return 0;
}