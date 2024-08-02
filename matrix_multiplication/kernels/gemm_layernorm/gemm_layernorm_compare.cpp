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
#define epsilon 0.00001

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
vector<vector<float>> output_pluto(input_height, vector<float>(input_width, 0));

// Tiled version with user-defined tile sizes
void gemm_layernorm_tiled(const vector<vector<float>>& A, const vector<vector<float>>& B, vector<vector<float>>& C, int tile_size1, int tile_size2, int tile_size3);

// Naive version of gemm_bias_relu
void gemm_layernorm_naive(const vector<vector<float>>& A, const vector<vector<float>>& B) {
    // Perform matrix multiplication alpha * A * B + beta * C
    for (int i = 0; i < input_height; ++i) {
        for (int j = 0; j < input_width; ++j) {
            output_naive[i][j] = output_naive[i][j] * beta;
            for (int p = 0; p < input_width; ++p) {
                output_naive[i][j] += alpha * A[i][p] * B[p][j];
            }
        }
    }

    float mean = 0.0;
    float variance = 0.0;

    // Layer normalization
    for (int i = 0; i < input_height; ++i) {
        mean = 0.0;
        variance = 0.0;

        // Calculate mean
        for (int j = 0; j < input_width; ++j) {
            mean = mean + output_naive[i][j];
        }
        mean = mean / input_width;

        // Calculate variance
        for (int j = 0; j < input_width; ++j) {
            variance = variance + (output_naive[i][j] - mean) * (output_naive[i][j] - mean);
        }
        variance = variance / input_width;

        // Normalize
        for (int j = 0; j < input_width; ++j) {
            output_naive[i][j] = (output_naive[i][j] - mean) / sqrt(variance + epsilon);
        }
    }
}

// Function to compare outputs of naive and tiled versions
bool compare_outputs(const vector<vector<float>>& output1, const vector<vector<float>>& output2) {
    if (output1.size() != output2.size()) return false;
    for (size_t i = 0; i < output1.size(); ++i) {
        if (output1[i].size() != output2[i].size()) return false;
        for (size_t j = 0; j < output1[i].size(); ++j) {
            if (abs(output1[i][j] - output2[i][j]) > epsilon) return false;
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
    gemm_layernorm_naive(A, B);
    auto end_naive = high_resolution_clock::now();
    auto duration_naive = duration_cast<microseconds>(end_naive - start_naive);
    cout << "Naive version took " << duration_naive.count() << " microseconds." << endl;

    // Perform Pluto gemm_bias_relu and measure time
    auto start_pluto = high_resolution_clock::now();
    gemm_layernorm_tiled(A, B, output_pluto, 32, 32, 32);
    auto end_pluto = high_resolution_clock::now();
    auto duration_pluto = duration_cast<microseconds>(end_pluto - start_pluto);
    cout << "Pluto version took " << duration_pluto.count() << " microseconds." << endl;

    // Perform tiled gemm_bias_relu and measure time
    auto start_tiled = high_resolution_clock::now();
    gemm_layernorm_tiled(A, B, output_tiled, 64, 128, 32);
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

void gemm_layernorm_tiled(const vector<vector<float>>& A, const vector<vector<float>>& B, vector<vector<float>>& C, int tile_size1, int tile_size2, int tile_size3) {
    int lbv, ubv;
    float mean = 0.0;
    float variance = 0.0;

    for (int t2 = 0; t2 <= floord(input_height - 1, tile_size1); t2++) {
        for (int t4 = 0; t4 <= floord(input_width - 1, tile_size2); t4++) {
            for (int t5 = tile_size1 * t2; t5 <= min(input_height - 1, tile_size1 * t2 + tile_size1 - 1); t5++) {
                lbv = tile_size2 * t4;
                ubv = min(input_width - 1, tile_size2 * t4 + tile_size2 - 1);
                for (int t7 = lbv; t7 <= ubv; t7++) {
                    C[t5][t7] = C[t5][t7] * beta;
                }
            }
        }
    }

    for (int t2 = 0; t2 <= floord(input_height - 1, tile_size1); t2++) {
        for (int t4 = 0; t4 <= floord(input_width - 1, tile_size2); t4++) {
            for (int t5 = 0; t5 <= floord(input_width - 1, tile_size3); t5++) {
                for (int t6 = tile_size1 * t2; t6 <= min(input_height - 1, tile_size1 * t2 + tile_size1 - 1); t6++) {
                    for (int t8 = tile_size3 * t5; t8 <= min(input_width - 1, tile_size3 * t5 + tile_size3 - 1); t8++) {
                        lbv = tile_size2 * t4;
                        ubv = min(input_width - 1, tile_size2 * t4 + tile_size2 - 1);
                        for (int t9 = lbv; t9 <= ubv; t9++) {
                            C[t6][t9] = C[t6][t9] + alpha * A[t6][t8] * B[t8][t9];
                        }
                    }
                }
            }
        }
    }

    for (int t2 = 0; t2 <= input_height - 1; t2++) {
        mean = 0.0;
        variance = 0.0;
        for (int t4 = 0; t4 <= input_width - 1; t4++) {
            mean += C[t2][t4];
        }
        mean = mean / input_width;
        for (int t4 = 0; t4 <= input_width - 1; t4++) {
            variance += (C[t2][t4] - mean) * (C[t2][t4] - mean);
        }
        variance = variance / input_width;
        for (int t4 = 0; t4 <= floord(input_width - 1, tile_size2); t4++) {
            lbv = tile_size2 * t4;
            ubv = min(input_width - 1, tile_size2 * t4 + tile_size2 - 1);
            for (int t5 = lbv; t5 <= ubv; t5++) {
                C[t2][t5] = (C[t2][t5] - mean) / sqrt(variance + epsilon);
            }
        }
    }
}
