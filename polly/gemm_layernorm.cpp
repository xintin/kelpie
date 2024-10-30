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

int main() {
    // Seed for random number generation
    srand(time(0));

    vector<vector<float>> A(input_height, vector<float>(input_width));
    for (int i = 0; i < input_height; ++i) {
        for (int j = 0; j < input_width; ++j) {
            A[i][j] = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 100;
        }
    }

    vector<vector<float>> B(input_height, vector<float>(input_width));
    for (int i = 0; i < input_height; ++i) {
        for (int j = 0; j < input_width; ++j) {
            B[i][j] = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 100;
        }
    }

    auto start_naive = high_resolution_clock::now();
    gemm_layernorm_naive(A, B);
    auto end_naive = high_resolution_clock::now();
    auto duration_naive = duration_cast<milliseconds>(end_naive - start_naive);
    cout << "Naive version took " << duration_naive.count() << " milliseconds." << endl;

   
    return 0;
}