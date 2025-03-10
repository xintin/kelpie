#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <tuple>
#include <cfloat>
#include <cmath>
#include <chrono>

using namespace std;

// Define constants
constexpr int M = 1024; // Width of the image
constexpr int N = 768;  // Height of the image
constexpr int T = 1;    // To translate k (Set to 1 for a 3-element kernel)

// Function to apply 2D Gaussian blur
void apply_gaussian_blur(
    const vector<vector<int>>& in_image,
    vector<vector<int>>& gauss_image,
    const vector<int>& Gauss,
    const vector<int>& tot
) {
    // Temporary images and accumulators
    vector<vector<int>> g_tmp_image(N, vector<int>(M, 0));
    vector<vector<vector<int>>> g_acc1(N, vector<vector<int>>(M, vector<int>(4, 0)));
    vector<vector<vector<int>>> g_acc2(N, vector<vector<int>>(M, vector<int>(4, 0)));

    int gauss_index, in_x, tmp_y, x, y, k;
    
    // First Pass: Horizontal Blur
    for(x = 1; x < N - 2; x++) { // x from 1 to N-3 to prevent x + 2 >= N
        for(y = 0; y < M; y++) {
            g_acc1[x][y][0] = 0;
            for(k = 0; k <= 2; k++) { // k from 0 to 2
                gauss_index = k - T + 1; // Adjusted indexing: Gauss[0] = Gauss[-1], Gauss[1] = Gauss[0], Gauss[2] = Gauss[1], Gauss[3] = Gauss[2]
                // Boundary check for Gauss indexing
                in_x = x + k;
                
                g_acc1[x][y][k + 2 - T] = g_acc1[x][y][k + 1 - T] + in_image[x+k][y] * Gauss[k - T + 1];
            }
            // Normalize by tot[3]
            g_tmp_image[x][y] = g_acc1[x][y][3] / tot[3];
        }
    }

    // Second Pass: Vertical Blur
    for(x = 1; x < N - 2; x++) { // x from 1 to N-3 to align with horizontal blur
        for(y = 1; y < M - 1; y++) { // y from 1 to M-2
            g_acc2[x][y][0] = 0;
            for(k = 0; k <= 2; k++) { // k from 0 to 2
                gauss_index = k - T + 1; // Adjusted indexing

                g_acc2[x][y][k + 2 - T] = g_acc2[x][y][k + 1 - T] + g_tmp_image[x][y + k - T] * Gauss[k - T + 1];
            }
            gauss_image[x][y] = g_acc2[x][y][3] / tot[3];
        }
    }
}

int main(int argc, char* argv[]) {
    // Seed for reproducibility
    srand(static_cast<unsigned>(time(0)));

    // Initialize input and output images
    vector<vector<int>> in_image(N, vector<int>(M, 0));      // Input image
    vector<vector<int>> gauss_image(N, vector<int>(M, 0));   // Output image

    // Initialize Gaussian kernel with padding for Gauss[-1]
    // Gauss[0] = Gauss[-1], Gauss[1] = Gauss[0], Gauss[2] = Gauss[1], Gauss[3] = Gauss[2]
    vector<int> Gauss = {0, 1, 2, 1}; // Example kernel: [Gauss[-1], Gauss[0], Gauss[1], Gauss[2]]

    // Initialize tot array
    // tot[0] = 0
    // tot[1] = tot[0] + Gauss[1] = 0 + 1 = 1
    // tot[2] = tot[1] + Gauss[2] = 1 + 2 = 3
    // tot[3] = tot[2] + Gauss[3] = 3 + 1 = 4
    vector<int> tot(4, 0);
    tot[0] = 0;
    for(int k = 0; k <= 2; k++) { // k from 0 to 2
        int index = k + 1;        // 1,2,3
        int prev_index = k;        // 0,1,2
        if(index >= static_cast<int>(tot.size()) || prev_index >= static_cast<int>(tot.size())) {
            cerr << "Error: tot indexing out of bounds during initialization.\n";
            exit(EXIT_FAILURE);
        }
        tot[index] = tot[prev_index] + Gauss[k +1];
    }

    // Initialize in_image with sample data (for demonstration)
    for(int x = 0; x < N; x++) {
        for(int y = 0; y < M; y++) {
            in_image[x][y] = rand() % 256; // Example: random pixel values between 0 and 255
        }
    }

    // Apply Gaussian blur
    apply_gaussian_blur(in_image, gauss_image, Gauss, tot);


    for(int x = 0; x < min(N, 10); x++) {
        for(int y = 0; y < min(M, 10); y++) {
            cout << gauss_image[x][y] << " ";
        }
        cout << endl;
    }


    return 0;
}