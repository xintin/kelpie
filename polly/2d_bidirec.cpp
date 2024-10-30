#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <tuple>
#include <cfloat>

using namespace std;

const int N = 10068;

using Matrix = vector<vector<int>>;

void bidirec_2d_naive(Matrix& a) {
    for (int i = 1; i < N; i++) {
        for (int j = 0; j < N; j++) {
            a[i][j] += a[i - 1][j] + a[i - 1][N - 1 - j];
        }
    }
}


int main(int argc, char* argv[]) {
    Matrix a1(N, vector<int>(N));

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            a1[i][j] = i + j;
        }
    }

    auto start_naive = chrono::high_resolution_clock::now();
    bidirec_2d_naive(a1);
    auto end_naive = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duration_naive = end_naive - start_naive;
    cout << "Execution time of bidirec_naive_2d: " << duration_naive.count() << " ms" << endl;

    return 0;
}