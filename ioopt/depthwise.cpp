#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>

using namespace std;
using namespace std::chrono;

#define BATCH_SIZE 128
#define CHANNELS 84
#define HEIGHT 83
#define WIDTH 83
#define FILTER_SIZE 5
#define STRIDE 1
#define PADDING 1
#define DILATION 1

inline int floord(int n, int d) {
    return static_cast<int>(std::floor(static_cast<double>(n) / static_cast<double>(d)));
}

//generated schedule is exactly same as default
void depthwise_conv2d_ioopt(const vector<vector<vector<vector<float>>>> &input,
                            const vector<vector<vector<vector<float>>>> &filters,
                            vector<vector<vector<vector<float>>>> &output,
                            int batch_size, int input_channels, int output_height,
                            int output_width, int filter_size, int stride,
                            int padding, int dilation) {

    for (int c3 = 0; c3 < input_channels; c3 += 1)
        for (int n3 = 0; n3 < batch_size; n3 += 1)
            for (int i1 = 0; i1 < output_height; i1 += 1)
                for (int kj = 0; kj < filter_size; kj += 1)
                    for (int ki = 0; ki < filter_size; ki += 1)
                        for (int j = 0; j < output_width; j += 1) {
                            int ni = i1 + ki * dilation - padding;
                            int nj = j + kj * dilation - padding;
                            if (ni >= 0 && ni < output_height && nj >= 0 && nj < output_width) {
                                output[n3][c3][i1][j] += input[n3][c3][ni][nj] * filters[c3][0][ki][kj];
                            }
                        }
}

int main() {
    int batch_size = BATCH_SIZE;
    int input_channels = CHANNELS;
    int input_height = HEIGHT;
    int input_width = WIDTH;
    int filter_size = FILTER_SIZE;
    int stride = STRIDE;
    int padding = PADDING;
    int dilation = DILATION;

    vector<vector<vector<vector<float>>>> input(
        batch_size,
        vector<vector<vector<float>>>(
            input_channels,
            vector<vector<float>>(
                input_height + 2 * padding,
                vector<float>(input_width + 2 * padding, 1.0f)))); // Padded input

    for (int n = 0; n < batch_size; ++n) {
        for (int c = 0; c < input_channels; ++c) {
            for (int i = 0; i < input_height; ++i) {
                for (int j = 0; j < input_width; ++j) {
                    input[n][c][i + padding][j + padding] = 1.0f;
                }
            }
        }
    }

    vector<vector<vector<vector<float>>>> filters(
        input_channels,
        vector<vector<vector<float>>>(
            1, vector<vector<float>>(filter_size,
                                     vector<float>(filter_size, 1.0f))));

    int output_height = (input_height + 2 * padding - dilation * (filter_size - 1) - 1) / stride + 1;
    int output_width = (input_width + 2 * padding - dilation * (filter_size - 1) - 1) / stride + 1;

    vector<vector<vector<vector<float>>>> output_non_tiled(
        batch_size, vector<vector<vector<float>>>(
                        input_channels,
                        vector<vector<float>>(
                            output_height, vector<float>(output_width, 0.0f))));

    // Non-tiled execution
    auto start = high_resolution_clock::now();
    depthwise_conv2d_ioopt(input, filters, output_non_tiled, batch_size, input_channels,
                     output_height, output_width, filter_size, stride, padding, dilation);
    auto end = high_resolution_clock::now();
    auto duration_non_tiled = duration_cast<milliseconds>(end - start);
    cout << "IOOPT version took " << duration_non_tiled.count()
         << " milliseconds." << endl;

    return 0;
}