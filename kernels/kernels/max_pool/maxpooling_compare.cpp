#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <cmath>

using namespace std;
using namespace std::chrono;

#define input_height 10000
#define input_width 10000
#define pool_size 2
#define tile_size_height 32
#define tile_size_width 64

// Pluto version
void max_pooling_pluto(const std::vector<std::vector<float>>& input, std::vector<std::vector<float>>& output);

// Naive version of max pooling
void max_pooling_naive(const vector<vector<float>>& input, vector<vector<float>>& output) {
    int output_height = input_height - pool_size + 1;
    int output_width = input_width - pool_size + 1;

    for (int i = 0; i < output_height; ++i) {
        for (int j = 0; j < output_width; ++j) {
            float max_val = -1e10;
            for (int ki = 0; ki < pool_size; ++ki) {
                for (int kj = 0; kj < pool_size; ++kj) {
                    float current_val = input[i + ki][j + kj];
                    max_val = max(current_val, max_val);
                }
            }
            output[i][j] = max_val;
        }
    }
}

// Tiled version of max pooling
void max_pooling_tiled(const vector<vector<float>>& input, vector<vector<float>>& output) {
    int output_height = input_height - pool_size + 1;
    int output_width = input_width - pool_size + 1;

    for (int ii = 0; ii < output_height; ii += tile_size_height) {
        for (int jj = 0; jj < output_width; jj += tile_size_width) {
            for (int i = ii; i < min(ii + tile_size_height, output_height); ++i) {
                for (int j = jj; j < min(jj + tile_size_width, output_width); ++j) {
                    float max_val = -1e10;
                    for (int ki = 0; ki < pool_size; ++ki) {
                        for (int kj = 0; kj < pool_size; ++kj) {
                            float current_val = input[i + ki][j + kj];
                            max_val = max(current_val, max_val);
                        }
                    }
                    output[i][j] = max_val;
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
    vector<vector<float>> input(input_height, vector<float>(input_width));
    for (int i = 0; i < input_height; ++i) {
        for (int j = 0; j < input_width; ++j) {
            input[i][j] = static_cast<float>(rand()) / RAND_MAX * 100;
        }
    }

    int output_height = input_height - pool_size + 1;
    int output_width = input_width - pool_size + 1;

    vector<vector<float>> output_naive(output_height, vector<float>(output_width, 0));
    vector<vector<float>> output_tiled(output_height, vector<float>(output_width, 0));
    vector<vector<float>> output_pluto(output_height, vector<float>(output_width, 0));


    // Perform naive max pooling and measure time
    auto start_naive = high_resolution_clock::now();
    max_pooling_naive(input, output_naive);
    auto end_naive = high_resolution_clock::now();
    auto duration_naive = duration_cast<microseconds>(end_naive - start_naive);
    cout << "Naive version took " << duration_naive.count() << " microseconds." << endl;

    // Perform tiled max pooling and measure time
    auto start_pluto = high_resolution_clock::now();
    max_pooling_pluto(input, output_pluto);
    auto end_pluto = high_resolution_clock::now();
    auto duration_pluto = duration_cast<microseconds>(end_pluto - start_pluto);
    cout << "Pluto version took " << duration_pluto.count() << " microseconds." << endl;


    // Perform tiled max pooling and measure time
    auto start_tiled = high_resolution_clock::now();
    max_pooling_tiled(input, output_tiled);
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


void max_pooling_pluto(const std::vector<std::vector<float>>& input, std::vector<std::vector<float>>& output) {
    int output_height = (input_height - pool_size) + 1;
    int output_width = (input_width - pool_size) + 1;
    float max_val, current_val;

/* Copyright (C) 1991-2021 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */
/* This header is separate from features.h so that the compiler can
   include it implicitly at the start of every compilation.  It must
   not itself include <features.h> or any other header that includes
   <features.h> because the implicit include comes before any feature
   test macros that may be defined in a source file before it first
   explicitly includes a system header.  GCC knows the name of this
   header in order to preinclude it.  */
/* glibc's intent is to support the IEC 559 math functionality, real
   and complex.  If the GCC (4.9 and later) predefined macros
   specifying compiler intent are available, use them to determine
   whether the overall intent is to support these features; otherwise,
   presume an older compiler has intent to support these features and
   define these macros by default.  */
/* wchar_t uses Unicode 10.0.0.  Version 10.0 of the Unicode Standard is
   synchronized with ISO/IEC 10646:2017, fifth edition, plus
   the following additions from Amendment 1 to the fifth edition:
   - 56 emoji characters
   - 285 hentaigana
   - 3 additional Zanabazar Square characters */
  int t1, t2, t4, t5;
/* Start of CLooG code */
if ((output_height >= 1) && (output_width >= 1)) {
  if (pool_size >= 1) {
    for (t1=0;t1<=output_height-1;t1++) {
      for (t2=0;t2<=output_width-1;t2++) {
        max_val = -1e10;;
        for (t4=0;t4<=pool_size-1;t4++) {
          for (t5=0;t5<=pool_size-1;t5++) {
            current_val = input[t1 + t4][t2 + t5];;
            max_val = (current_val > max_val) ? current_val : max_val;;
          }
        }
        output[t1][t2] = max_val;;
      }
    }
  }
  if (pool_size <= 0) {
    for (t1=0;t1<=output_height-1;t1++) {
      for (t2=0;t2<=output_width-1;t2++) {
        max_val = -1e10;;
        output[t1][t2] = max_val;;
      }
    }
  }
}
/* End of CLooG code */
}