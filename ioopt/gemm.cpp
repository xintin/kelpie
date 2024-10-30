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

void matrix_multiply_tiled(const vector<vector<float>> &A, const vector<vector<float>> &B) {
    // Initialize output_tiled to zero
    for (size_t i = 0; i < input_height; ++i)
        fill(output_tiled[i].begin(), output_tiled[i].end(), 0.0f);

    for (int k3 = 0; k3 < input_width; k3 += 178)
        for (int j3 = 0; j3 < input_width; j3 += 76)
            for (int i3 = 0; i3 < input_height; i3 += 51) {
                int Si3 = min(51, input_height - i3);
                for (int i2 = i3; i2 < i3 + Si3; i2 += 32) {
                    int Sj3 = min(76, input_width - j3);
                    for (int j1 = j3; j1 < j3 + Sj3; j1 += 23) {
                        int Si2 = min(32, i3 + Si3 - i2);
                        for (int i1 = i2; i1 < i2 + Si2; i1 += 23) {
                            int Sk3 = min(178, input_width - k3);
                            for (int k1 = k3; k1 < k3 + Sk3; k1 += 1) {
                                int Sj1 = min(23, j3 + Sj3 - j1);
                                for (int j = j1; j < j1 + Sj1; j += 1) {
                                    int Si1 = min(23, i2 + Si2 - i1);
                                    for (int i = i1; i < i1 + Si1; i += 1) {
                                        output_tiled[i][j] += A[i][k1] * B[k1][j];
                                    }
                                }
                            }
                        }
                    }
                }
            }
}

void matrix_multiply_naive(const vector<vector<float>> &A, const vector<vector<float>> &B) {
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
    srand(static_cast<unsigned>(time(0)));

    vector<vector<float>> A(input_height, vector<float>(input_width));
    vector<vector<float>> B(input_height, vector<float>(input_width));
    for (int i = 0; i < input_height; ++i) {
        for (int j = 0; j < input_width; ++j) {
            A[i][j] = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 100;
            B[i][j] = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 100;
        }
    }

    auto start_naive = high_resolution_clock::now();
    matrix_multiply_naive(A, B);
    auto end_naive = high_resolution_clock::now();
    auto duration_naive = duration_cast<milliseconds>(end_naive - start_naive);
    cout << "Naive version took " << duration_naive.count() << " milliseconds." << endl;

    auto start_tiled = high_resolution_clock::now();
    matrix_multiply_tiled(A, B);
    auto end_tiled = high_resolution_clock::now();
    auto duration_tiled = duration_cast<milliseconds>(end_tiled - start_tiled);
    cout << "Tiled version took " << duration_tiled.count() << " milliseconds." << endl;

    return 0;
}