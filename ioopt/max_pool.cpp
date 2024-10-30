#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <chrono>

using namespace std;
using namespace std::chrono;

#define input_height 10000
#define input_width 10000
#define pool_size 2

void max_pooling_tiled(const std::vector<std::vector<float>>& input, std::vector<std::vector<float>>& output) {
    int output_height = (input_height - pool_size) + 1;
    int output_width = (input_width - pool_size) + 1;
    float max_val, current_val;

    for (int j3 = 0; j3 < output_width; j3 += 115)
        for (int i3 = 0; i3 < output_height; i3 += 115) {
            int Sj3 = min(115, output_width - j3);
            for (int j2 = j3; j2 < j3 + Sj3; j2 += 110) {
                int Si3 = min(115, output_height - i3);
                for (int i2 = i3; i2 < i3 + Si3; i2 += 110) {
                    int Sj2 = min(110, j3 + Sj3 - j2);
                    for (int j1 = j2; j1 < j2 + Sj2; j1 += 13) {
                        int Si2 = min(110, i3 + Si3 - i2);
                        for (int i1 = i2; i1 < i2 + Si2; i1 += 13)
                            for (int kj = 0; kj < pool_size; ++kj)
                                for (int ki = 0; ki < pool_size; ++ki) {
                                    int Sj1 = min(13, j2 + Sj2 - j1);
                                    for (int j = j1; j < j1 + Sj1; j += 1) {
                                        int Si1 = min(13, i2 + Si2 - i1);
                                        for (int i = i1; i < i1 + Si1; i += 1) {
                                            max_val = -1e10;
                                            for (int ki_inner = 0; ki_inner < pool_size; ++ki_inner) {
                                                for (int kj_inner = 0; kj_inner < pool_size; ++kj_inner) {
                                                    current_val = input[i + ki_inner][j + kj_inner];
                                                    max_val = max(current_val, max_val);
                                                }
                                            }
                                            output[i][j] = max_val;
                                        }
                                    }
                                }
                    }
                }
            }
        }
}

void max_pooling(const std::vector<std::vector<float>>& input, std::vector<std::vector<float>>& output) {
    int output_height = (input_height - pool_size) + 1;
    int output_width = (input_width - pool_size) + 1;
    float max_val, current_val;

    for (int i = 0; i < output_height; ++i) {
        for (int j = 0; j < output_width; ++j) {
            max_val = -1e10;
            for (int ki = 0; ki < pool_size; ++ki) {
                for (int kj = 0; kj < pool_size; ++kj) {
                    current_val = input[i + ki][j + kj];
                    max_val = max(current_val, max_val);
                }
            }
            output[i][j] = max_val;
        }
    }
}

int main() {
    srand(time(0));

    std::vector<std::vector<float>> input(input_height, std::vector<float>(input_width));
    for (int i = 0; i < input_height; ++i) {
        for (int j = 0; j < input_width; ++j) {
            input[i][j] = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 100;
        }
    }

    int output_height = (input_height - pool_size) + 1;
    int output_width = (input_width - pool_size) + 1;

    std::vector<std::vector<float>> output_non_tiled(output_height, std::vector<float>(output_width, 0));
    std::vector<std::vector<float>> output_tiled(output_height, std::vector<float>(output_width, 0));

    auto start_non_tiled = high_resolution_clock::now();
    max_pooling(input, output_non_tiled);
    auto end_non_tiled = high_resolution_clock::now();
    auto duration_non_tiled = duration_cast<milliseconds>(end_non_tiled - start_non_tiled);
    cout << "Non-tiled version took " << duration_non_tiled.count() << " milliseconds." << endl;

    auto start_tiled = high_resolution_clock::now();
    max_pooling_tiled(input, output_tiled);
    auto end_tiled = high_resolution_clock::now();
    auto duration_tiled = duration_cast<milliseconds>(end_tiled - start_tiled);
    cout << "Tiled version took " << duration_tiled.count() << " milliseconds." << endl;

    return 0;
}