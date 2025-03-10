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
vector<vector<float>> output_tiled(input_height, vector<float>(input_width, 0));
vector<vector<float>> output_pluto(input_height, vector<float>(input_width, 0));

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

void matrix_multiply_tiled_pluto(const vector<vector<float>> &A, const vector<vector<float>> &B) {
    // Initialize output_pluto to zero
    for (size_t i = 0; i < input_height; ++i)
        fill(output_pluto[i].begin(), output_pluto[i].end(), 0.0f);

    int lbv, ubv;
    int t1, t2, t3, t4, t5, t6;

    for (t1 = 0; t1 <= floord(input_height - 1, 32); t1++) {
        for (t2 = 0; t2 <= floord(input_width - 1, 32); t2++) {
            for (t3 = 0; t3 <= floord(input_width - 1, 32); t3++) {
                for (t4 = 32 * t1; t4 <= min(input_height - 1, 32 * t1 + 31); t4++) {
                    for (t5 = 32 * t3; t5 <= min(input_width - 1, 32 * t3 + 31); t5++) {
                        lbv = 32 * t2;
                        ubv = min(input_width - 1, 32 * t2 + 31);
                        for (t6 = lbv; t6 <= ubv; t6++) {
                            output_pluto[t4][t6] += A[t4][t5] * B[t5][t6];
                        }
                    }
                }
            }
        }
    }
}

void matrix_multiply_tiled_pluto_expt(const vector<vector<float>> &A,
                                      const vector<vector<float>> &B,
                                      int T4, int T5, int T6) {
    // Initialize output_tiled to zero
    for (size_t i = 0; i < input_height; ++i)
        fill(output_tiled[i].begin(), output_tiled[i].end(), 0.0f);

    int lbv, ubv;
    int t1, t2, t3, t4, t5, t6;

    for (t1 = 0; t1 <= floord(input_height - 1, T4); t1++) {
        for (t2 = 0; t2 <= floord(input_width - 1, T5); t2++) {
            for (t3 = 0; t3 <= floord(input_height - 1, T6); t3++) {
                for (t4 = T4 * t1; t4 <= min(input_height - 1, T4 * t1 + T4 - 1); t4++) {
                    for (t5 = T6 * t3; t5 <= min(input_height - 1, T6 * t3 + T6 - 1); t5++) {
                        lbv = T5 * t2;
                        ubv = min(input_width - 1, T5 * t2 + T5 - 1);
                        for (t6 = lbv; t6 <= ubv; t6++) {
                            output_tiled[t4][t6] += A[t4][t5] * B[t5][t6];
                        }
                    }
                }
            }
        }
    }
}

// Function to compare outputs of naive and tiled versions
bool compare_outputs(const vector<vector<float>>& output1, const vector<vector<float>>& output2) {
    if (output1.size() != output2.size()) return false;
    for (size_t i = 0; i < output1.size(); ++i) {
        if (output1[i].size() != output2[i].size()) return false;
        for (size_t j = 0; j < output1[i].size(); ++j) {
            if (abs(output1[i][j] - output2[i][j]) > 1e-5) return false;
        }
    }
    return true;
}

int main() {
    // Seed for random number generation
    srand(static_cast<unsigned>(time(0)));

    // Initialize the input matrices with random values
    vector<vector<float>> A(input_height, vector<float>(input_width));
    vector<vector<float>> B(input_height, vector<float>(input_width));
    for (int i = 0; i < input_height; ++i) {
        for (int j = 0; j < input_width; ++j) {
            A[i][j] = static_cast<float>(rand()) / RAND_MAX * 100;
            B[i][j] = static_cast<float>(rand()) / RAND_MAX * 100;
        }
    }

    // Perform naive matrix multiplication and measure time
    auto start_naive = high_resolution_clock::now();
    matrix_multiply_naive(A, B);
    auto end_naive = high_resolution_clock::now();
    auto duration_naive = duration_cast<milliseconds>(end_naive - start_naive);
    cout << "Naive version took " << duration_naive.count() << " milliseconds." << endl;

    // Perform tiled/pluto matrix multiplication and measure time
    auto start_pluto = high_resolution_clock::now();
    matrix_multiply_tiled_pluto(A, B);
    auto end_pluto = high_resolution_clock::now();
    auto duration_pluto = duration_cast<milliseconds>(end_pluto - start_pluto);
    cout << "Pluto version took " << duration_pluto.count() << " milliseconds." << endl;

    // Perform experimental tiled matrix multiplication and measure time
    auto start_tiled = high_resolution_clock::now();
    matrix_multiply_tiled_pluto_expt(A, B, 32, 64, 128);
    auto end_tiled = high_resolution_clock::now();
    auto duration_tiled = duration_cast<milliseconds>(end_tiled - start_tiled);
    cout << "Tiled version took " << duration_tiled.count() << " milliseconds." << endl;

    // Compare outputs
    bool outputs_match = compare_outputs(output_naive, output_tiled);
    cout << "Outputs match: naive v. tiled " << (outputs_match ? "Yes" : "No") << endl;

    outputs_match = compare_outputs(output_naive, output_pluto);
    cout << "Outputs match: naive v. pluto " << (outputs_match ? "Yes" : "No") << endl;

    return 0;
}
