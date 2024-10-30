#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <ctime>
#include <chrono>

#define input_height 1000
#define input_width 1000

using namespace std;
using namespace std::chrono;

inline int ceild(int n, int d) {
    return static_cast<int>(ceil(static_cast<float>(n) / static_cast<float>(d)));
}

inline int floord(int n, int d) {
    return static_cast<int>(floor(static_cast<float>(n) / static_cast<float>(d)));
}

inline int max(int x, int y) { return std::max(x, y); }
inline int min(int x, int y) { return std::min(x, y); }

vector<vector<float>> output_naive(input_height, vector<float>(input_width, 0));

void matrix_multiply_naive(const vector<vector<float>> &A, const vector<vector<float>> &B) {
    // Initialize output_naive to zero
    for (size_t i = 0; i < input_height; ++i)
        fill(output_naive[i].begin(), output_naive[i].end(), 0.0f);

    for (size_t i = 0; i < input_height; ++i) {
        for (size_t j = 0; j < input_width; ++j) {
            for (size_t k = 0; k < input_width; ++k) {
                output_naive[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}


int main() {
    // Seed for random number generation
    srand(static_cast<unsigned>(time(0)));

    // Initialize the input matrices with random values
    vector<vector<float>> A(input_height, vector<float>(input_width));
    vector<vector<float>> B(input_height, vector<float>(input_width));
    for (int i = 0; i < input_height; ++i) {
        for (int j = 0; j < input_width; ++j) {
            A[i][j] = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 100;
            B[i][j] = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 100;
        }
    }

    // Perform naive matrix multiplication and measure time
    auto start_naive = high_resolution_clock::now();
    matrix_multiply_naive(A, B);
    auto end_naive = high_resolution_clock::now();
    auto duration_naive = duration_cast<milliseconds>(end_naive - start_naive);
    cout << "Naive version took " << duration_naive.count() << " milliseconds." << endl;

    return 0;
}
