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
vector<float> output_pluto(inp_size);

void initialize(vector<float>& input, vector<float>& exp_values, float& max_val) {
    srand(static_cast<unsigned>(time(0)));

    for (int i = 0; i < inp_size; ++i) {
        input[i] = static_cast<float>(rand()) / RAND_MAX * 100;
    }

    max_val = *max_element(input.begin(), input.end());
}

void softmax_naive(const vector<float>& input, vector<float>& exp_values, float max_val) {
    float sum_exp = 0.0;

    // Compute the exponential values and their sum
    for (int i = 0; i < inp_size; ++i) {
        exp_values[i] = exp(input[i] - max_val);
        sum_exp += exp_values[i];
    }

    // Compute the softmax values
    for (int i = 0; i < inp_size; ++i) {
        output_naive[i] = exp_values[i] / sum_exp;
    }
}

void softmax_pluto(const vector<float>& input, vector<float>& exp_values, float max_val) {
    int t2, t3;
    int lbv, ubv;
    float sum_exp = 0.0;

    // Compute the exponential values and their sum
    for (t2 = 0; t2 < inp_size; ++t2) {
        exp_values[t2] = exp(input[t2] - max_val);
        sum_exp += exp_values[t2];
    }

    // Compute the softmax values using tiling
    for (t2 = 0; t2 <= floord(inp_size - 1, 32); ++t2) {
        lbv = 32 * t2;
        ubv = min(inp_size - 1, 32 * t2 + 31);
        for (t3 = lbv; t3 <= ubv; ++t3) {
            output_pluto[t3] = exp_values[t3] / sum_exp;
        }
    }
}

void softmax_tiled(const vector<float>& input, vector<float>& exp_values, float max_val, int tile_size) {
    int t2, t3;
    int lbv, ubv;
    float sum_exp = 0.0;

    // Compute the exponential values and their sum
    for (t2 = 0; t2 < inp_size; ++t2) {
        exp_values[t2] = exp(input[t2] - max_val);
        sum_exp += exp_values[t2];
    }

    // Compute the softmax values using tiling
    for (t2 = 0; t2 <= floord(inp_size - 1, tile_size); ++t2) {
        lbv = tile_size * t2;
        ubv = min(inp_size - 1, tile_size * t2 + tile_size - 1);
        #pragma ivdep
        #pragma vector always
        for (t3 = lbv; t3 <= ubv; ++t3) {
            output_tiled[t3] = exp_values[t3] / sum_exp;
        }
    }
}

// Function to compare outputs of naive and tiled versions
bool compare_outputs(const vector<float>& output1, const vector<float>& output2) {
    if (output1.size() != output2.size()) return false;
    for (size_t i = 0; i < output1.size(); ++i) {
        if (output1[i] != output2[i]) return false;
    }
    return true;
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
    auto duration_naive = duration_cast<microseconds>(end_naive - start_naive);
    cout << "Naive version took " << duration_naive.count() << " microseconds." << endl;

    // Compute softmax using pluto method
    auto start_pluto = high_resolution_clock::now();
    softmax_pluto(input, exp_values, max_val);
    auto end_pluto = high_resolution_clock::now();
    auto duration_pluto = duration_cast<microseconds>(end_pluto - start_pluto);
    cout << "Pluto version took " << duration_pluto.count() << " microseconds." << endl;

    // Compute softmax using tiled method
    int tile_size = 32; // Example tile size
    auto start_tiled = high_resolution_clock::now();
    softmax_tiled(input, exp_values, max_val, tile_size);
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
