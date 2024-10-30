#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <ctime>
#include <chrono>

#define inp_size 10000000
#define ceild(n,d)  ceil(((double)(n))/((double)(d)))
#define floord(n,d) floor(((double)(n))/((double)(d)))
#define max(x,y)    ((x) > (y)? (x) : (y))
#define min(x,y)    ((x) < (y)? (x) : (y))

using namespace std;
using namespace std::chrono;

float max_val;

vector<float> output_naive(inp_size);
vector<float> output_tiled(inp_size);

void initialize(vector<float>& input, vector<float>& exp_values, float& max_val) {
    srand(static_cast<unsigned>(time(0)));

    for (int i = 0; i < inp_size; ++i) {
        input[i] = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 100;
    }

    max_val = *max_element(input.begin(), input.end());
}

void softmax_tiled(const vector<float>& input, vector<float>& exp_values, float max_val) {
    float sum_exp = 0.0;

    // Compute the exponential values and their sum using the tiling schedule
    for (int i3 = 0; i3 < inp_size; i3 += 2775) {
        int Si3 = min(2775, inp_size - i3);
        for (int i2 = i3; i2 < i3 + Si3; i2 += 1463) {
            int Si2 = min(1463, i3 + Si3 - i2);
            for (int i1 = i2; i1 < i2 + Si2; i1 += 93) {
                int Si1 = min(93, i2 + Si2 - i1);
                for (int i = i1; i < i1 + Si1; i += 1) {
                    exp_values[i] = exp(input[i] - max_val);
                    sum_exp += exp_values[i];
                }
            }
        }
    }

    // Compute the softmax values using the same tiling schedule
    for (int i3 = 0; i3 < inp_size; i3 += 2775) {
        int Si3 = min(2775, inp_size - i3);
        for (int i2 = i3; i2 < i3 + Si3; i2 += 1463) {
            int Si2 = min(1463, i3 + Si3 - i2);
            for (int i1 = i2; i1 < i2 + Si2; i1 += 93) {
                int Si1 = min(93, i2 + Si2 - i1);
                for (int i = i1; i < i1 + Si1; i += 1) {
                    output_tiled[i] = exp_values[i] / sum_exp;
                }
            }
        }
    }
}

void softmax_naive(const vector<float>& input, vector<float>& exp_values, float max_val) {
    float sum_exp = 0.0;

    for (int i = 0; i < inp_size; ++i) {
        exp_values[i] = exp(input[i] - max_val);
        sum_exp += exp_values[i];
    }

    for (int i = 0; i < inp_size; ++i) {
        output_naive[i] = exp_values[i] / sum_exp;
    }
}

int main() {
    vector<float> input(inp_size);
    vector<float> exp_values(inp_size);

    // Initialize the input and other necessary variables
    initialize(input, exp_values, max_val);

    // Compute softmax using naive method
    auto start_naive = high_resolution_clock::now();
    softmax_naive(input, exp_values, max_val);
    auto end_naive = high_resolution_clock::now();
    auto duration_naive = duration_cast<milliseconds>(end_naive - start_naive);
    cout << "Naive version took " << duration_naive.count() << " milliseconds." << endl;

    // Compute softmax using tiled method
    auto start_tiled = high_resolution_clock::now();
    softmax_tiled(input, exp_values, max_val);
    auto end_tiled = high_resolution_clock::now();
    auto duration_tiled = duration_cast<milliseconds>(end_tiled - start_tiled);
    cout << "Tiled version took " << duration_tiled.count() << " milliseconds." << endl;

    return 0;
}