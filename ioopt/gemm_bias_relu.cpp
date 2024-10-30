#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <cmath>

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

vector<double> bias(input_width, 1.0);
vector<vector<float>> output_naive(input_height, vector<float>(input_width, 0));
vector<vector<float>> output_tiled(input_height, vector<float>(input_width, 0));

// ioopt version of gemm_bias_relu
void gemm_bias_relu_ioopt(const vector<vector<float>>& A, const vector<vector<float>>& B) {
    for (int p3 = 0; p3 < input_width; p3 += 178)
        for (int i3 = 0; i3 < input_height; i3 += 77)
            for (int j3 = 0; j3 < input_width; j3 += 51) {
                int Sj3 = min(51, input_width - j3);
                for (int j2 = j3; j2 < j3 + Sj3; j2 += 31) {
                    int Sj2 = min(31, j3 + Sj3 - j2);
                    for (int j1 = j2; j1 < j2 + Sj2; j1 += 22) {
                        int Si3 = min(77, input_height - i3);
                        for (int i1 = i3; i1 < i3 + Si3; i1 += 23) {
                            int Sp3 = min(178, input_width - p3);
                            for (int p1 = p3; p1 < p3 + Sp3; p1 += 1) {
                                int Sj1 = min(22, j2 + Sj2 - j1);
                                for (int j = j1; j < j1 + Sj1; j += 1) {
                                    int Si1 = min(23, i3 + Si3 - i1);
                                    for (int i = i1; i < i1 + Si1; i += 1) {
                                        if (i < input_height && j < input_width && p1 < input_width) {
                                            output_tiled[i][j] += alpha * A[i][p1] * B[p1][j];  // Matrix multiplication
                                        }
                                    }
                                }
                            }
                            // Ensure valid indices for bias addition and ReLU
                            for (int i = i1; i < min(i1 + Si3, input_height); i++) {
                                for (int j = j1; j < min(j1 + Sj2, input_width); j++) {
                                    output_tiled[i][j] *= beta;               // Apply beta scaling
                                    output_tiled[i][j] += bias[j];            // Add bias
                                    output_tiled[i][j] = max(output_tiled[i][j], 0.0f);  // ReLU activation
                                }
                            }
                        }
                    }
                }
            }
}

// Naive version of gemm_bias_relu
void gemm_bias_relu_naive(const vector<vector<float>>& A, const vector<vector<float>>& B) {
    for (int i = 0; i < input_height; ++i) {
        for (int j = 0; j < input_width; ++j) {
            output_naive[i][j] *= beta;  // Apply beta scaling to C
            for (int p = 0; p < input_width; ++p) {
                output_naive[i][j] += alpha * A[i][p] * B[p][j];
            }
            output_naive[i][j] += bias[j];  // Add bias
            output_naive[i][j] = max(output_naive[i][j], 0.0f);  // Apply ReLU
        }
    }
}

int main() {
    srand(time(0));

    // Initialize matrices A and B with random values
    vector<vector<float>> A(input_height, vector<float>(input_width));
    vector<vector<float>> B(input_height, vector<float>(input_width));
    for (int i = 0; i < input_height; ++i) {
        for (int j = 0; j < input_width; ++j) {
            A[i][j] = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 100;
            B[i][j] = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 100;
        }
    }

    // Run and time the naive version
    auto start_naive = high_resolution_clock::now();
    gemm_bias_relu_naive(A, B);
    auto end_naive = high_resolution_clock::now();
    auto duration_naive = duration_cast<milliseconds>(end_naive - start_naive);
    cout << "Naive version took " << duration_naive.count() << " milliseconds." << endl;

    // Run and time the tiled version
    auto start_tiled = high_resolution_clock::now();
    gemm_bias_relu_ioopt(A, B);
    auto end_tiled = high_resolution_clock::now();
    auto duration_tiled = duration_cast<milliseconds>(end_tiled - start_tiled);
    cout << "Ioopt version took " << duration_tiled.count() << " milliseconds." << endl;

    return 0;
}