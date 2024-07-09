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
vector<vector<float>> output_pluto(input_height, vector<float>(input_width, 0));

// Pluto version gemm_bias_relu
void gemm_bias_relu_pluto(const vector<vector<float>>& A, const vector<vector<float>>& B);

// Tiled version of gemm_bias_relu
void gemm_bias_relu_tiled(const vector<vector<float>>& A, const vector<vector<float>>& B, int tile_size_m, int tile_size_n, int tile_size_k);

// Naive version of gemm_bias_relu
void gemm_bias_relu_naive(const vector<vector<float>>& A, const vector<vector<float>>& B) {
    // Perform matrix multiplication alpha * A * B + beta * C with bias addition and ReLU activation
    for (int i = 0; i < input_height; ++i) {
        for (int j = 0; j < input_width; ++j) {
            output_naive[i][j] = output_naive[i][j] * beta;  // Apply beta scaling to C
            for (int p = 0; p < input_width; ++p) { // Use input_width instead of input_height
                output_naive[i][j] += alpha * A[i][p] * B[p][j];
            }
            output_naive[i][j] += bias[j]; // Add bias
            output_naive[i][j] = max(output_naive[i][j], 0.0f); // Apply ReLU
        }
    }
}

// Function to compare outputs of naive and tiled versions
bool compare_outputs(const vector<vector<float>>& output1, const vector<vector<float>>& output2) {
    if (output1.size() != output2.size()) return false;
    for (size_t i = 0; i < output1.size(); ++i) {
        if (output1[i].size() != output2[i].size()) return false;
        for (size_t j = 0; j < output1[i].size(); ++j) {
            if (output1[i][j] != output2[i][j]) return false;
        }
    }
    return true;
}

int main() {
    // Seed for random number generation
    srand(time(0));

    // Initialize the input matrix with random values
    vector<vector<float>> A(input_height, vector<float>(input_width));
    for (int i = 0; i < input_height; ++i) {
        for (int j = 0; j < input_width; ++j) {
            A[i][j] = static_cast<float>(rand()) / RAND_MAX * 100;
        }
    }

    vector<vector<float>> B(input_height, vector<float>(input_width));
    for (int i = 0; i < input_height; ++i) {
        for (int j = 0; j < input_width; ++j) {
            B[i][j] = static_cast<float>(rand()) / RAND_MAX * 100;
        }
    }

    // Perform naive gemm_bias_relu and measure time
    auto start_naive = high_resolution_clock::now();
    gemm_bias_relu_naive(A, B);
    auto end_naive = high_resolution_clock::now();
    auto duration_naive = duration_cast<microseconds>(end_naive - start_naive);
    cout << "Naive version took " << duration_naive.count() << " microseconds." << endl;

    // Perform Pluto gemm_bias_relu and measure time
    auto start_pluto = high_resolution_clock::now();
    gemm_bias_relu_pluto(A, B);
    auto end_pluto = high_resolution_clock::now();
    auto duration_pluto = duration_cast<microseconds>(end_pluto - start_pluto);
    cout << "Pluto version took " << duration_pluto.count() << " microseconds." << endl;

    // Perform tiled gemm_bias_relu and measure time
    auto start_tiled = high_resolution_clock::now();
    gemm_bias_relu_tiled(A, B, 128, 128, 128); // Uncomment and implement if necessary
    auto end_tiled = high_resolution_clock::now();
    auto duration_tiled = duration_cast<microseconds>(end_tiled - start_tiled);
    cout << "Tiled version took " << duration_tiled.count() << " microseconds." << endl;

    // Compare outputs
    bool outputs_match = compare_outputs(output_naive, output_tiled);
    cout << "Outputs match: naive v. tiled " << (outputs_match ? "Yes" : "No") << endl;

    outputs_match = compare_outputs(output_naive, output_pluto);
    cout << "Outputs match: naive v. pluto " << (outputs_match ? "Yes" : "No") << endl;

    return 0;
}

void gemm_bias_relu_pluto(const vector<vector<float>>& A, const vector<vector<float>>& B) {
    int t2, t3, t4, t5, t6, t7;
    int lbv, ubv;
    
    // Beta scaling
    for (t2 = 0; t2 <= floord(input_height - 1, 32); t2++) {
        for (t3 = 0; t3 <= floord(input_width - 1, 32); t3++) {
            for (t4 = 32 * t2; t4 <= min(input_height - 1, 32 * t2 + 31); t4++) {
                lbv = 32 * t3;
                ubv = min(input_width - 1, 32 * t3 + 31);
                for (t5 = lbv; t5 <= ubv; t5++) {
                    output_pluto[t4][t5] *= beta;
                }
            }
        }
    }

    // Matrix multiplication
    for (t2 = 0; t2 <= floord(input_height - 1, 32); t2++) {
        for (t3 = 0; t3 <= floord(input_width - 1, 32); t3++) {
            for (t4 = 0; t4 <= floord(input_width - 1, 32); t4++) {
                for (t5 = 32 * t2; t5 <= min(input_height - 1, 32 * t2 + 31); t5++) {
                    for (t6 = 32 * t4; t6 <= min(input_width - 1, 32 * t4 + 31); t6++) {
                        lbv = 32 * t3;
                        ubv = min(input_width - 1, 32 * t3 + 31);
                        for (t7 = lbv; t7 <= ubv; t7++) {
                            output_pluto[t5][t7] += alpha * A[t5][t6] * B[t6][t7];
                        }
                    }
                }
            }
        }
    }

    // Add bias and apply ReLU
    for (t2 = 0; t2 <= floord(input_height - 1, 32); t2++) {
        for (t3 = 0; t3 <= floord(input_width - 1, 32); t3++) {
            for (t4 = 32 * t2; t4 <= min(input_height - 1, 32 * t2 + 31); t4++) {
                lbv = 32 * t3;
                ubv = min(input_width - 1, 32 * t3 + 31);
                for (t5 = lbv; t5 <= ubv; t5++) {
                    output_pluto[t4][t5] += bias[t5];
                    output_pluto[t4][t5] = max(output_pluto[t4][t5], 0.0f);
                }
            }
        }
    }
}

void gemm_bias_relu_tiled(const vector<vector<float>>& A, const vector<vector<float>>& B, int tile_size_m, int tile_size_n, int tile_size_k) {
    int t2, t3, t4, t5, t6, t7;
    int lbv, ubv;

    // Beta scaling
    for (t2 = 0; t2 <= floord(input_height - 1, tile_size_m); t2++) {
        for (t3 = 0; t3 <= floord(input_width - 1, tile_size_n); t3++) {
            for (t4 = tile_size_m * t2; t4 <= min(input_height - 1, tile_size_m * t2 + tile_size_m - 1); t4++) {
                lbv = tile_size_n * t3;
                ubv = min(input_width - 1, tile_size_n * t3 + tile_size_n - 1);
                for (t5 = lbv; t5 <= ubv; t5++) {
                    output_tiled[t4][t5] *= beta;
                }
            }
        }
    }

    // Matrix multiplication
    for (t2 = 0; t2 <= floord(input_height - 1, tile_size_m); t2++) {
        for (t3 = 0; t3 <= floord(input_width - 1, tile_size_n); t3++) {
            for (t4 = 0; t4 <= floord(input_width - 1, tile_size_k); t4++) {
                for (t5 = tile_size_m * t2; t5 <= min(input_height - 1, tile_size_m * t2 + tile_size_m - 1); t5++) {
                    for (t6 = tile_size_k * t4; t6 <= min(input_width - 1, tile_size_k * t4 + tile_size_k - 1); t6++) {
                        lbv = tile_size_n * t3;
                        ubv = min(input_width - 1, tile_size_n * t3 + tile_size_n - 1);
                        for (t7 = lbv; t7 <= ubv; t7++) {
                            output_tiled[t5][t7] += alpha * A[t5][t6] * B[t6][t7];
                        }
                    }
                }
            }
        }
    }

    // Add bias and apply ReLU
    for (t2 = 0; t2 <= floord(input_height - 1, tile_size_m); t2++) {
        for (t3 = 0; t3 <= floord(input_width - 1, tile_size_n); t3++) {
            for (t4 = tile_size_m * t2; t4 <= min(input_height - 1, tile_size_m * t2 + tile_size_m - 1); t4++) {
                lbv = tile_size_n * t3;
                ubv = min(input_width - 1, tile_size_n * t3 + tile_size_n - 1);
                for (t5 = lbv; t5 <= ubv; t5++) {
                    output_tiled[t4][t5] += bias[t5];
                    output_tiled[t4][t5] = max(output_tiled[t4][t5], 0.0f);
                }
            }
        }
    }
}
