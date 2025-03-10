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

void max_pooling(const std::vector<std::vector<float>>& input, std::vector<std::vector<float>>& output) {
    int output_height = (input_height - pool_size) + 1;
    int output_width = (input_width - pool_size) + 1;
    float max_val, current_val;

    for (int i = 0; i < output_height; ++i) {
        for (int j = 0; j < output_width; ++j) {
            // Initialize max_val to a small number
            max_val = -1e10;
            // Affine expressions for pooling window
            for (int ki = 0; ki < pool_size; ++ki) {
                for (int kj = 0; kj < pool_size; ++kj) {
                    current_val = input[i + ki][j + kj];
                    max_val = (current_val > max_val) ? current_val : max_val;
                }
            }
            output[i][j] = max_val;
        }
    }
}

int main() {

    // Seed for random number generation
    std::srand(std::time(0));

    // Initialize the input matrix with random values
    std::vector<std::vector<float>> input(input_height, std::vector<float>(input_width));
    for (int i = 0; i < input_height; ++i) {
        for (int j = 0; j < input_width; ++j) {
            input[i][j] = static_cast<float>(std::rand()) / RAND_MAX * 100;
        }
    }

    int output_height = (input_height - pool_size) + 1;
    int output_width = (input_width - pool_size) + 1;

    std::vector<std::vector<float>> output(output_height, std::vector<float>(output_width, 0));

    // Perform non-tiled depthwise convolution and measure time
    auto start = high_resolution_clock::now();
    max_pooling(input, output);
    auto end = high_resolution_clock::now();
    auto duration_non_tiled = duration_cast<milliseconds>(end - start);
    cout << "Non-tiled version took " << duration_non_tiled.count()
        << " milliseconds." << endl;

    return 0;
}
