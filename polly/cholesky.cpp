#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <tuple>
#include <cfloat>
#include <cmath>
#include <chrono>

#define NSLIP 1000

using namespace std;

using Matrix = vector<vector<double>>;

void cholesky_naive(Matrix& a, vector<double>& fdot, int nSlip) {
    for(int i = 1; i < nSlip; i++) {
        fdot[i] = 0.0;
        for(int k = 0; k < i; k++) {
            fdot[i] += a[i][k] * a[k][i];
        }
        a[i][i] = a[i][i] - fdot[i];
        for(int j = i+1; j < nSlip; j++) {
            fdot[i] = 0.0;
            for(int k = 0; k < i; k++) {
                fdot[i] += a[i][k] * a[k][j];
            }
            a[i][j] = a[i][j] - fdot[i];
            fdot[i] = 0.0;
            for(int k = 0; k < i; k++) {
                fdot[i] += a[j][k] * a[k][i];
            }
            a[j][i] = (a[j][i] - fdot[i]) / a[i][i];
        }
    }
}

int main() {
    // Initialize random seed
    srand(static_cast<unsigned>(time(0)));

    Matrix a_naive(NSLIP, vector<double>(NSLIP, 0.0));
    Matrix a_tiled(NSLIP, vector<double>(NSLIP, 0.0));
    for(int i = 0; i < NSLIP; i++) {
        for(int j = 0; j < NSLIP; j++) {
            double value = static_cast<double>(rand()) / RAND_MAX;
            a_naive[i][j] = value;
            a_tiled[i][j] = value;
        }
    }

    vector<double> fdot_naive(NSLIP, 0.0);

    auto start_naive = chrono::high_resolution_clock::now();
    cholesky_naive(a_naive, fdot_naive, NSLIP);
    auto end_naive = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duration_naive = end_naive - start_naive;
    cout << "Execution time of cholesky_naive: " << duration_naive.count() << " ms\n";

    return 0;
}