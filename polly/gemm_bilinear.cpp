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



int main() {
    // Seed for random number generation
    srand(time(0));

    // Initialize the input matrix with random values
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

    // Perform naive gemm_bias_relu and measure time
    auto start_naive = high_resolution_clock::now();
    gemm_bilinear_naive(A, B);
    auto end_naive = high_resolution_clock::now();
    auto duration_naive = duration_cast<milliseconds>(end_naive - start_naive);
    cout << "Naive version took " << duration_naive.count() << " milliseconds." << endl;

    return 0;
}
