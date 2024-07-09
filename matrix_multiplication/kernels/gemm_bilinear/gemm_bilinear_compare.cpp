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
vector<vector<float>> output_pluto(input_height, vector<float>(input_width, 0));

void gemm_bilinear_pluto(const vector<vector<float>>& A, 
                         const vector<vector<float>>& B,
                         vector<vector<float>>& C,
                         int tile_size_t2, int tile_size_t3, 
                         int tile_size_t4);

void gemm_bilinear_naive(const vector<vector<float>>& A, 
                   const vector<vector<float>>& B) {
    // Temporary matrix to store the result of alpha * A * B
    vector<vector<float>> temp(input_height, vector<float>(input_width, 0.0));

    // Perform matrix multiplication alpha * A * B
    for (int i = 0; i < input_height; ++i) {
        for (int j = 0; j < input_width; ++j) {
            for (int p = 0; p < input_height; ++p) {
                temp[i][j] += alpha * A[i][p] * B[p][j];
            }
        }
    }

    // Update matrix C with the result of alpha * A * B + beta * C
    for (int i = 0; i < input_height; ++i) {
        for (int j = 0; j < input_width; ++j) {
            output_naive[i][j] = temp[i][j] + beta * output_naive[i][j];
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
    gemm_bilinear_naive(A, B);
    auto end_naive = high_resolution_clock::now();
    auto duration_naive = duration_cast<microseconds>(end_naive - start_naive);
    cout << "Naive version took " << duration_naive.count() << " microseconds." << endl;

    // Perform Pluto gemm_bias_relu and measure time
    auto start_pluto = high_resolution_clock::now();
    gemm_bilinear_pluto(A, B, output_pluto, 32, 32, 32);
    auto end_pluto = high_resolution_clock::now();
    auto duration_pluto = duration_cast<microseconds>(end_pluto - start_pluto);
    cout << "Pluto version took " << duration_pluto.count() << " microseconds." << endl;

    // Perform tiled gemm_bias_relu and measure time
    auto start_tiled = high_resolution_clock::now();
    gemm_bilinear_pluto(A, B, output_tiled, 32, 64, 128);
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


void gemm_bilinear_pluto(const vector<vector<float>>& A, 
                         const vector<vector<float>>& B, 
                         vector<vector<float>>& C,
                         int tile_size_t2, int tile_size_t3, int tile_size_t4) {
    // Temporary matrix to store the result of alpha * A * B
    vector<vector<float>> temp(input_height, vector<float>(input_width, 0.0));

    // Perform matrix multiplication alpha * A * B using tiling
    for (int t2 = 0; t2 <= floord(input_height - 1, tile_size_t2); ++t2) {
        for (int t3 = 0; t3 <= floord(input_width - 1, tile_size_t3); ++t3) {
            for (int t4 = 0; t4 <= floord(input_height - 1, tile_size_t4); ++t4) {
                for (int i = tile_size_t2 * t2; i <= min(input_height - 1, tile_size_t2 * t2 + tile_size_t2 - 1); ++i) {
                    for (int k = tile_size_t4 * t4; k <= min(input_height - 1, tile_size_t4 * t4 + tile_size_t4 - 1); ++k) {
                        int lbv = tile_size_t3 * t3;
                        int ubv = min(input_width - 1, tile_size_t3 * t3 + tile_size_t3 - 1);
                        #pragma ivdep
                        #pragma vector always
                        for (int j = lbv; j <= ubv; ++j) {
                            temp[i][j] += alpha * A[i][k] * B[k][j];
                        }
                    }
                }
            }
        }
    }

    // Update matrix C with the result of alpha * A * B + beta * C using tiling
    for (int t2 = 0; t2 <= floord(input_height - 1, tile_size_t2); ++t2) {
        for (int t3 = 0; t3 <= floord(input_width - 1, tile_size_t3); ++t3) {
            for (int i = tile_size_t2 * t2; i <= min(input_height - 1, tile_size_t2 * t2 + tile_size_t2 - 1); ++i) {
                int lbv = tile_size_t3 * t3;
                int ubv = min(input_width - 1, tile_size_t3 * t3 + tile_size_t3 - 1);
                for (int j = lbv; j <= ubv; ++j) {
                    C[i][j] = temp[i][j] + beta * C[i][j];
                }
            }
        }
    }
}
