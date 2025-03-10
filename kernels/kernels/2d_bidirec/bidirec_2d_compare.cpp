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

void bidirec_2d_pluto(Matrix& a, int tile_size) {
    int t1, t2, t3;
    int lbv, ubv;

    for (t1 = 1; t1 <= N - 1; t1++) {
        for (t2 = 0; t2 <= (N - 1) / tile_size; t2++) {
            lbv = tile_size * t2;
            ubv = min(N - 1, tile_size * t2 + tile_size - 1);

            for (t3 = lbv; t3 <= ubv; t3++) {
                a[t1][t3] += a[t1 - 1][t3] + a[t1 - 1][N - 1 - t3];
            }
        }
    }
}

void bidirec_2d_expt(Matrix& a, int tile_size_t2) {
    int t1, t2, t3;
    int lbv, ubv;

    for (t1 = 1; t1 <= N - 1; t1++) {
        for (t2 = 0; t2 <= (N - 1) / tile_size_t2; t2++) {
            lbv = tile_size_t2 * t2;
            ubv = min(N - 1, tile_size_t2 * t2 + tile_size_t2 - 1);

            for (t3 = lbv; t3 <= ubv; t3++) {
                a[t1][t3] += a[t1 - 1][t3] + a[t1 - 1][N - 1 - t3];
            }
        }
    }
}

int main(int argc, char* argv[]) {
    Matrix a1(N, vector<int>(N));
    Matrix a2(N, vector<int>(N));
    Matrix a3(N, vector<int>(N));

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            a1[i][j] = i + j;
        }
    }

    a2 = a1;
    a3 = a1;

    auto start_naive = chrono::high_resolution_clock::now();
    bidirec_2d_naive(a1);
    auto end_naive = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duration_naive = end_naive - start_naive;
    cout << "Execution time of bidirec_naive_2d: " << duration_naive.count() << " ms" << endl;

    int tile_size_pluto = 32;
    auto start_pluto = chrono::high_resolution_clock::now();
    bidirec_2d_pluto(a2, tile_size_pluto);
    auto end_pluto = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duration_pluto = end_pluto - start_pluto;
    cout << "Execution time of bidirec_2d_pluto with tile size " << tile_size_pluto << ": " << duration_pluto.count() << " ms" << endl;

    int tile_size_expt_t2 = 128; 
    auto start_expt = chrono::high_resolution_clock::now();
    bidirec_2d_expt(a3, tile_size_expt_t2);
    auto end_expt = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duration_expt = end_expt - start_expt;
    cout << "Execution time of bidirec_2d_expt with tile size " << tile_size_expt_t2 << ": " << duration_expt.count() << " ms" << endl;

    bool same_a1_a2 = true;
    for (int i = 0; i < N && same_a1_a2; i++) {
        if (!equal(a1[i].begin(), a1[i].end(), a2[i].begin())) {
            same_a1_a2 = false;
            cout << "Difference found between a1 and a2 at row " << i << endl;
            break;
        }
    }

    if (same_a1_a2) {
        cout << "Outputs of bidirec_naive_2d and bidirec_2d_pluto are the same." << endl;
    } else {
        cout << "Outputs of bidirec_naive_2d and bidirec_2d_pluto differ." << endl;
    }

    bool same_a1_a3 = true;
    for (int i = 0; i < N && same_a1_a3; i++) {
        if (!equal(a1[i].begin(), a1[i].end(), a3[i].begin())) {
            same_a1_a3 = false;
            cout << "Difference found between a1 and a3 at row " << i << endl;
            break;
        }
    }

    if (same_a1_a3) {
        cout << "Outputs of bidirec_naive_2d and bidirec_2d_expt are the same." << endl;
    } else {
        cout << "Outputs of bidirec_naive_2d and bidirec_2d_expt differ." << endl;
    }

    return 0;
}