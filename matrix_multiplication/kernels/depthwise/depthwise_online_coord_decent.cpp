#include <algorithm>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <tuple>
#include <vector>

#define BATCH_SIZE 128
#define CHANNELS 84
#define HEIGHT 83
#define WIDTH 83
#define FILTER_SIZE 5
#define STRIDE 1
#define PADDING 1
#define DILATION 1

using namespace std;

std::vector<std::vector<std::vector<std::vector<float>>>>
    input(BATCH_SIZE,
          std::vector<std::vector<std::vector<float>>>(
              CHANNELS, std::vector<std::vector<float>>(
                            HEIGHT + 2 * PADDING,
                            std::vector<float>(WIDTH + 2 * PADDING,
                                               1.0f)))); // Padded input

std::vector<std::vector<std::vector<std::vector<float>>>>
    filters(CHANNELS,
            std::vector<std::vector<std::vector<float>>>(
                1, std::vector<std::vector<float>>(
                       FILTER_SIZE, std::vector<float>(FILTER_SIZE, 1.0f))));

int output_height =
    (HEIGHT + 2 * PADDING - DILATION * (FILTER_SIZE - 1) - 1) / STRIDE + 1;
int output_width =
    (WIDTH + 2 * PADDING - DILATION * (FILTER_SIZE - 1) - 1) / STRIDE + 1;

std::vector<std::vector<std::vector<std::vector<float>>>> output(
    BATCH_SIZE,
    std::vector<std::vector<std::vector<float>>>(
        CHANNELS, std::vector<std::vector<float>>(
                      output_height, std::vector<float>(output_width, 0.0f))));

//const std::vector<int> allowed_values = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512};
const std::vector<int> allowed_values = {128, 256};

inline int ceild(int n, int d) {
  return static_cast<int>(
      std::ceil(static_cast<double>(n) / static_cast<double>(d)));
}
inline int floord(int n, int d) {
  return static_cast<int>(
      std::floor(static_cast<double>(n) / static_cast<double>(d)));
}


void depthwise_conv2d_tiled(
    const std::vector<std::vector<std::vector<std::vector<float>>>> &input,
    const std::vector<std::vector<std::vector<std::vector<float>>>> &filters,
    std::vector<std::vector<std::vector<std::vector<float>>>> &output,
    int batch_size, int input_channels, int output_height, int output_width,
    int filter_size, int stride, int padding, int dilation, int tile_size_batch,
    int tile_size_channel, int tile_size_height, int tile_size_width) {

  int t2, t3, t4, t5, t6, t7, t8, t9;
  int lbv, ubv;

  if ((batch_size >= 1) && (input_channels >= 1) && (output_height >= 1) &&
      (output_width >= 1)) {
    for (t2 = 0; t2 <= floord(batch_size - 1, tile_size_batch); t2++) {
      for (t3 = 0; t3 <= floord(input_channels - 1, tile_size_channel); t3++) {
        for (t4 = 0; t4 <= floord(output_height - 1, tile_size_height); t4++) {
          for (t5 = 0; t5 <= floord(output_width - 1, tile_size_width); t5++) {
            for (t6 = tile_size_batch * t2;
                 t6 <= min(batch_size - 1,
                           tile_size_batch * t2 + tile_size_batch - 1);
                 t6++) {
              for (t7 = tile_size_channel * t3;
                   t7 <= min(input_channels - 1,
                             tile_size_channel * t3 + tile_size_channel - 1);
                   t7++) {
                for (t8 = tile_size_height * t4;
                     t8 <= min(output_height - 1,
                               tile_size_height * t4 + tile_size_height - 1);
                     t8++) {
                  lbv = tile_size_width * t5;
                  ubv = min(output_width - 1,
                            tile_size_width * t5 + tile_size_width - 1);
                  for (t9 = lbv; t9 <= ubv; t9++) {
                    output[t6][t7][t8][t9] = 0.0;
                    for (int ki = 0; ki < filter_size; ++ki) {
                      for (int kj = 0; kj < filter_size; ++kj) {
                        int ni = t8 + ki * dilation - padding;
                        int nj = t9 + kj * dilation - padding;
                        if (ni >= 0 && ni < output_height && nj >= 0 &&
                            nj < output_width) {
                          output[t6][t7][t8][t9] +=
                              input[t6][t7][ni][nj] * filters[t7][0][ki][kj];
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
}

void initialize_matrices() {
  for (int n = 0; n < BATCH_SIZE; ++n) {
    for (int c = 0; c < CHANNELS; ++c) {
      for (int i = 0; i < HEIGHT; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
          input[n][c][i + PADDING][j + PADDING] = 1.0f;
        }
      }
    }
  }
}

int measure_performance(int ts1, int ts2, int ts3, int ts4, int runs) {
  long long total_duration = 0;

  for (int i = 0; i < runs; ++i) {
    auto start = std::chrono::high_resolution_clock::now();
    depthwise_conv2d_tiled(input, filters, output, BATCH_SIZE, CHANNELS,
                           output_height, output_width, FILTER_SIZE, STRIDE,
                           PADDING, DILATION, ts1, ts2, ts3, ts4);
    auto stop = std::chrono::high_resolution_clock::now();

    total_duration +=
        std::chrono::duration_cast<std::chrono::milliseconds>(stop - start)
            .count();
  }

  return total_duration / runs;
}

std::vector<std::tuple<int, int, int, int>> find_neighbors(int ts1, int ts2,
                                                           int ts3, int ts4) {
  std::vector<std::tuple<int, int, int, int>> neighbors;

  for (int i = 0; i < 4; ++i) {
    int current = (i == 0) ? ts1 : (i == 1) ? ts2 : (i == 2) ? ts3 : ts4;
    auto it = std::find(allowed_values.begin(), allowed_values.end(), current);
    if (it != allowed_values.end()) {
      if (it != allowed_values.begin()) {
        int prev1 = *(it - 1);
        neighbors.emplace_back((i == 0) ? prev1 : ts1, (i == 1) ? prev1 : ts2,
                               (i == 2) ? prev1 : ts3, (i == 3) ? prev1 : ts4);
        if (it - 1 != allowed_values.begin()) {
          int prev2 = *(it - 2);
          neighbors.emplace_back((i == 0) ? prev2 : ts1, (i == 1) ? prev2 : ts2,
                                 (i == 2) ? prev2 : ts3,
                                 (i == 3) ? prev2 : ts4);
        }
      }
      if (it + 1 != allowed_values.end()) {
        int next1 = *(it + 1);
        neighbors.emplace_back((i == 0) ? next1 : ts1, (i == 1) ? next1 : ts2,
                               (i == 2) ? next1 : ts3, (i == 3) ? next1 : ts4);
        if (it + 2 != allowed_values.end()) {
          int next2 = *(it + 2);
          neighbors.emplace_back((i == 0) ? next2 : ts1, (i == 1) ? next2 : ts2,
                                 (i == 2) ? next2 : ts3,
                                 (i == 3) ? next2 : ts4);
        }
      }
    }
  }

  std::cout << "Neighbor size: " << neighbors.size() << "\n";
  return neighbors;
}

std::tuple<int, int, int, int, int> move_to_minimum(int ts1, int ts2, int ts3,
                                                    int ts4, int runs) {
  auto neighbors = find_neighbors(ts1, ts2, ts3, ts4);
  int best_performance = measure_performance(ts1, ts2, ts3, ts4, runs);
  std::tuple<int, int, int, int> best_config =
      std::make_tuple(ts1, ts2, ts3, ts4);

  for (auto &neighbor : neighbors) {
    int n1 = std::get<0>(neighbor), n2 = std::get<1>(neighbor),
        n3 = std::get<2>(neighbor), n4 = std::get<3>(neighbor);
    int perf = measure_performance(n1, n2, n3, n4, runs);
    if (perf < best_performance) {
      best_performance = perf;
      best_config = neighbor;
    }
  }

  return std::make_tuple(std::get<0>(best_config), std::get<1>(best_config),
                         std::get<2>(best_config), std::get<3>(best_config),
                         best_performance);
}

std::tuple<int, int, int, int, int>
coordinate_descent(int ts1, int ts2, int ts3, int ts4, int runs) {
  std::cout << "Starting position: (" << ts1 << ", " << ts2 << ", " << ts3
            << ", " << ts4 << ") with performance: "
            << measure_performance(ts1, ts2, ts3, ts4, runs) << " ms\n";
  auto current_position = move_to_minimum(ts1, ts2, ts3, ts4, runs);
  int current_performance = std::get<4>(current_position);

  while (true) {
    auto next_position = move_to_minimum(
        std::get<0>(current_position), std::get<1>(current_position),
        std::get<2>(current_position), std::get<3>(current_position), runs);
    int next_performance = std::get<4>(next_position);
    if (next_performance >= current_performance) {
      return current_position;
    }
    current_position = next_position;
    current_performance = next_performance;

    std::cout << "tile size (t1, t2, t3, t4): ("
              << std::get<0>(current_position) << ", "
              << std::get<1>(current_position) << ", "
              << std::get<2>(current_position) << ", "
              << std::get<3>(current_position)
              << ") with performance: " << current_performance << " ms\n";
  }
}

std::tuple<int, int, int, int, int> exhaustive_search(int runs) {
  int best_ts1 = 0, best_ts2 = 0, best_ts3 = 0, best_ts4 = 0,
      best_performance = INT_MAX;

  for (int ts1 : allowed_values) {
    for (int ts2 : allowed_values) {
      for (int ts3 : allowed_values) {
        for (int ts4 : allowed_values) {
          int performance = measure_performance(ts1, ts2, ts3, ts4, runs);

          std::cout << ts1 << " " << ts2 << " " << ts3 << " " << ts4 << " "
                    << performance << "\n";
          if (performance < best_performance) {
            best_performance = performance;
            best_ts1 = ts1;
            best_ts2 = ts2;
            best_ts3 = ts3;
            best_ts4 = ts4;
          }
        }
      }
    }
  }

  return std::make_tuple(best_ts1, best_ts2, best_ts3, best_ts4,
                         best_performance);
}

std::tuple<int, int, int, int, int> combined_exhaustive_search(int runs) {
  int best_ts1 = 0, best_ts2 = 0, best_ts3 = 0, best_ts4 = 0,
      best_performance = INT_MAX;

  for (int ts1 : allowed_values) {
    for (int ts2 : allowed_values) {
      for (int ts3 : allowed_values) {
        for (int ts4 : allowed_values) {
          int performance = measure_performance(ts1, ts2, ts3, ts4, runs);
          if (performance < best_performance) {
            best_performance = performance;
            best_ts1 = ts1;
            best_ts2 = ts2;
            best_ts3 = ts3;
            best_ts4 = ts4;
            std::cout << "tile size (t1, t2, t3, t4): " << best_ts1 << " "
                      << best_ts2 << " " << best_ts3 << " " << best_ts4 << " "
                      << best_performance << "ms \n";
          }
        }
      }
    }
  }

  return coordinate_descent(best_ts1, best_ts2, best_ts3, best_ts4, runs);
}

int main(int argc, char *argv[]) {
  int runs = std::atoi(argv[1]);
  if (runs <= 0) {
    std::cerr << "Number of runs must be a positive integer.\n";
    return 2;
  }

  initialize_matrices();
  int t1 = 1, t2 = 1, t3 = 1, t4 = 1;

  std::cout << "\nCoordinate Descent:\n";
  auto start_timer = std::chrono::high_resolution_clock::now();
  auto result = coordinate_descent(t1, t2, t3, t4, runs);
  auto end_timer = std::chrono::high_resolution_clock::now();
  auto duration_timer = std::chrono::duration_cast<std::chrono::milliseconds>(end_timer - start_timer);
  std::cout << "coordinate search took " << duration_timer.count() << " milliseconds to converge." << "\n";
  
  std::cout << "Converged to: (" << std::get<0>(result) << ", "
            << std::get<1>(result) << ", " << std::get<2>(result) << ", "
            << std::get<3>(result)
            << ") with performance: " << std::get<4>(result) << " milliseconds\n";

  std::cout << "\nExhaustive search:\n";
  start_timer = std::chrono::high_resolution_clock::now();
  result = exhaustive_search(runs);
  end_timer = std::chrono::high_resolution_clock::now();
  duration_timer = std::chrono::duration_cast<std::chrono::milliseconds>(end_timer - start_timer);
  std::cout << "exhaustive search took " << duration_timer.count() << " milliseconds to converge." << "\n";
  
  std::cout << "Converged to: (" << std::get<0>(result) << ", " <<
  std::get<1>(result) << ", " << std::get<2>(result) << ", " <<
  std::get<3>(result) << ") with performance: " << std::get<4>(result) << "milliseconds\n";

  return 0;
}
