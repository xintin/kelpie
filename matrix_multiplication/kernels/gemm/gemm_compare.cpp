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

void matrix_multiply_naive(const std::vector<std::vector<float>> &A,
                           const std::vector<std::vector<float>> &B) {
  for (size_t i = 0; i < input_height; ++i) {
    for (size_t j = 0; j < input_width; ++j) {
      for (size_t k = 0; k < input_height; ++k) {
        output_naive[i][j] += A[i][k] * B[k][j];
      }
    }
  }
}

void matrix_multiply_tiled_pluto(const std::vector<std::vector<float>> &A,
                                 const std::vector<std::vector<float>> &B) {
  int lbv, ubv;
  int t1, t2, t3, t4, t5, t6;

  for (t1 = 0; t1 <= (input_height - 1) / 32; t1++) {
    for (t2 = 0; t2 <= (input_width - 1) / 32; t2++) {
      for (t3 = 0; t3 <= (input_height - 1) / 32; t3++) {
        for (t4 = 32 * t1; t4 <= std::min(input_height - 1, 32 * t1 + 31); t4++) {
          for (t5 = 32 * t3; t5 <= std::min(input_height - 1, 32 * t3 + 31); t5++) {
            lbv = 32 * t2;
            ubv = std::min(input_width - 1, 32 * t2 + 31);
            for (t6 = lbv; t6 <= ubv; t6++) {
              output_pluto[t4][t6] += A[t4][t5] * B[t5][t6];
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
    matrix_multiply_naive(A, B);
    auto end_naive = high_resolution_clock::now();
    auto duration_naive = duration_cast<milliseconds>(end_naive - start_naive);
    cout << "Naive version took " << duration_naive.count() << " milliseconds." << endl;

    // Perform Pluto gemm_bias_relu and measure time
    auto start_pluto = high_resolution_clock::now();
    matrix_multiply_tiled_pluto(A, B);
    auto end_pluto = high_resolution_clock::now();
    auto duration_pluto = duration_cast<milliseconds>(end_pluto - start_pluto);
    cout << "Pluto version took " << duration_pluto.count() << " milliseconds." << endl;

    // Compare outputs
    bool outputs_match = compare_outputs(output_naive, output_tiled);
    cout << "Outputs match: naive v. tiled " << (outputs_match ? "Yes" : "No") << endl;

    outputs_match = compare_outputs(output_naive, output_pluto);
    cout << "Outputs match: naive v. pluto " << (outputs_match ? "Yes" : "No") << endl;

    return 0;
}

