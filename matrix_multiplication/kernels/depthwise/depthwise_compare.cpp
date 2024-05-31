#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <chrono>

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
  return static_cast<int>(
      std::floor(static_cast<double>(n) / static_cast<double>(d)));
}


void depthwise_conv2d(
    const vector<vector<vector<vector<float>>>>& input,
    const vector<vector<vector<vector<float>>>>& filters,
    vector<vector<vector<vector<float>>>>& output,
    int batch_size, int input_channels, int output_height, int output_width,
    int filter_size, int stride, int padding, int dilation) {
    
    for (int n = 0; n < batch_size; ++n) {
        for (int c = 0; c < input_channels; ++c) {
            for (int i = 0; i < output_height; ++i) {
                for (int j = 0; j < output_width; ++j) {
                    output[n][c][i][j] = 0.0;
                    for (int ki = 0; ki < filter_size; ++ki) {
                        for (int kj = 0; kj < filter_size; ++kj) {
                            int ni = i + ki * dilation - padding;
                            int nj = j + kj * dilation - padding;
                            if (ni >= 0 && ni < output_height && nj >= 0 && nj < output_width) {
                                output[n][c][i][j] += input[n][c][ni][nj] * filters[c][0][ki][kj];
                            }
                        }
                    }
                }
            }
        }
    }
}

void depthwise_conv2d_tiled(
    const vector<vector<vector<vector<float>>>>& input,
    const vector<vector<vector<vector<float>>>>& filters,
    vector<vector<vector<vector<float>>>>& output,
    int batch_size, int input_channels, int output_height, int output_width,
    int filter_size, int stride, int padding, int dilation,
    int tile_size_batch, int tile_size_channel, int tile_size_height, int tile_size_width) {
    
    int t2, t3, t4, t5, t6, t7, t8, t9;
    int lbv, ubv;

    /* Start of CLooG code */
    if ((batch_size >= 1) && (input_channels >= 1) && (output_height >= 1) && (output_width >= 1)) {
        for (t2 = 0; t2 <= floord(batch_size - 1, tile_size_batch); t2++) {
            for (t3 = 0; t3 <= floord(input_channels - 1, tile_size_channel); t3++) {
                for (t4 = 0; t4 <= floord(output_height - 1, tile_size_height); t4++) {
                    for (t5 = 0; t5 <= floord(output_width - 1, tile_size_width); t5++) {
                        for (t6 = tile_size_batch * t2; t6 <= min(batch_size - 1, tile_size_batch * t2 + tile_size_batch - 1); t6++) {
                            for (t7 = tile_size_channel * t3; t7 <= min(input_channels - 1, tile_size_channel * t3 + tile_size_channel - 1); t7++) {
                                for (t8 = tile_size_height * t4; t8 <= min(output_height - 1, tile_size_height * t4 + tile_size_height - 1); t8++) {
                                    lbv = tile_size_width * t5;
                                    ubv = min(output_width - 1, tile_size_width * t5 + tile_size_width - 1);
                                    for (t9 = lbv; t9 <= ubv; t9++) {
                                        output[t6][t7][t8][t9] = 0.0;
                                        for (int ki = 0; ki < filter_size; ++ki) {
                                            for (int kj = 0; kj < filter_size; ++kj) {
                                                int ni = t8 + ki * dilation - padding;
                                                int nj = t9 + kj * dilation - padding;
                                                if (ni >= 0 && ni < output_height && nj >= 0 && nj < output_width) {
                                                    output[t6][t7][t8][t9] += input[t6][t7][ni][nj] * filters[t7][0][ki][kj];
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    /* End of CLooG code */
}

bool compare_outputs(const vector<vector<vector<vector<float>>>>& output1, const vector<vector<vector<vector<float>>>>& output2) {
    if (output1.size() != output2.size()) return false;
    for (size_t n = 0; n < output1.size(); ++n) {
        if (output1[n].size() != output2[n].size()) return false;
        for (size_t c = 0; c < output1[n].size(); ++c) {
            if (output1[n][c].size() != output2[n][c].size()) return false;
            for (size_t i = 0; i < output1[n][c].size(); ++i) {
                if (output1[n][c][i].size() != output2[n][c][i].size()) return false;
                for (size_t j = 0; j < output1[n][c][i].size(); ++j) {
                    if (abs(output1[n][c][i][j] - output2[n][c][i][j]) > 1e-5) {
                        return false;
                    }
                }
            }
        }
    }
    return true;
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

    vector<vector<vector<vector<float>>>> input(batch_size,
        vector<vector<vector<float>>>(input_channels,
        vector<vector<float>>(input_height + 2 * padding, vector<float>(input_width + 2 * padding, 1.0f)))); // Padded input


    for (int n = 0; n < batch_size; ++n) {
        for (int c = 0; c < input_channels; ++c) {
            for (int i = 0; i < input_height; ++i) {
                for (int j = 0; j < input_width; ++j) {
                    input[n][c][i + padding][j + padding] = 1.0f; 
                }
            }
        }
    }

    vector<vector<vector<vector<float>>>> filters(input_channels,
        vector<vector<vector<float>>>(1,
        vector<vector<float>>(filter_size,
        vector<float>(filter_size, 1.0f)))); 

    int output_height = (input_height + 2 * padding - dilation * (filter_size - 1) - 1) / stride + 1;
    int output_width = (input_width + 2 * padding - dilation * (filter_size - 1) - 1) / stride + 1;

    vector<vector<vector<vector<float>>>> output_non_tiled(batch_size,
        vector<vector<vector<float>>>(input_channels,
        vector<vector<float>>(output_height, vector<float>(output_width, 0.0f))));

    vector<vector<vector<vector<float>>>> output_tiled(batch_size,
        vector<vector<vector<float>>>(input_channels,
        vector<vector<float>>(output_height, vector<float>(output_width, 0.0f))));

    // Perform non-tiled depthwise convolution and measure time
    auto start = high_resolution_clock::now();
    depthwise_conv2d(input, filters, output_non_tiled, batch_size, input_channels, output_height, output_width,
                     filter_size, stride, padding, dilation);
    auto end = high_resolution_clock::now();
    auto duration_non_tiled = duration_cast<microseconds>(end - start);
    cout << "Non-tiled version took " << duration_non_tiled.count() << " microseconds." << endl;

    // Test different tile sizes
    vector<tuple<int, int, int, int>> tile_sizes = {
        {1, 1, 1, 1},
        {32, 32, 32, 32},
        {4, 2, 16, 8},
    };

    for (auto [tile_size_batch, tile_size_channel, tile_size_height, tile_size_width] : tile_sizes) {
        cout << "Testing with tile sizes: (batch, channel, height, width) = ("
             << tile_size_batch << ", " << tile_size_channel << ", " << tile_size_height << ", " << tile_size_width << ")" << endl;

        for (int n = 0; n < batch_size; ++n) {
            for (int c = 0; c < input_channels; ++c) {
                for (int i = 0; i < output_height; ++i) {
                    fill(output_tiled[n][c][i].begin(), output_tiled[n][c][i].end(), 0.0f);
                }
            }
        }

        start = high_resolution_clock::now();
        depthwise_conv2d_tiled(input, filters, output_tiled, batch_size, input_channels, output_height, output_width,
                               filter_size, stride, padding, dilation, tile_size_batch, tile_size_channel, tile_size_height, tile_size_width);
        end = high_resolution_clock::now();
        auto duration_tiled = duration_cast<microseconds>(end - start);
        cout << "Tiled version took " << duration_tiled.count() << " microseconds." << endl;

        if (compare_outputs(output_non_tiled, output_tiled)) {
            cout << "Outputs match for tile sizes: (batch, channel, height, width) = ("
                 << tile_size_batch << ", " << tile_size_channel << ", " << tile_size_height << ", " << tile_size_width << ")" << endl;
        } else {
            cout << "Outputs do NOT match for tile sizes: (batch, channel, height, width) = ("
                 << tile_size_batch << ", " << tile_size_channel << ", " << tile_size_height << ", " << tile_size_width << ")" << endl;
        }
    }

    return 0;
}
