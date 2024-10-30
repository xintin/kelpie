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

void cholesky_ioopt(Matrix& a, vector<double>& fdot, int nSlip) {
    for (int i3 = 0; i3 < nSlip; i3 += 22) {
        int Si3 = min(22, nSlip - i3);  // Values in {10, 22}
        for (int i2 = i3; i2 < i3 + Si3; i2 += 14) {
            for (int k1 = 0; k1 < nSlip; k1 += 575) {
                int Si2 = min(14, i3 + Si3 - i2);  // Values in {8, 10, 14}
                for (int i1 = i2; i1 < i2 + Si2; i1 += 1) {
                    for (int j1 = 0; j1 < nSlip; j1 += 334) {
                        int Sk1 = min(575, nSlip - k1);  // Values in {425, 575}
                        for (int k = k1; k < k1 + Sk1; k += 1) {
                            int Sj1 = min(334, nSlip - j1);  // Values in {332, 334}
                            for (int j = j1; j < j1 + Sj1; j += 1) {
                                if (i1 > 0) {  // Ensure no out-of-bounds access
                                    fdot[i1] = 0.0;
                                    for (int k = 0; k < i1; k++) {
                                        fdot[i1] += a[i1][k] * a[k][i1];
                                    }
                                    a[i1][i1] -= fdot[i1];

                                    if (j > i1) {  // Only proceed if j > i1 for upper matrix
                                        fdot[i1] = 0.0;
                                        for (int k = 0; k < i1; k++) {
                                            fdot[i1] += a[i1][k] * a[k][j];
                                        }
                                        a[i1][j] -= fdot[i1];

                                        fdot[i1] = 0.0;
                                        for (int k = 0; k < i1; k++) {
                                            fdot[i1] += a[j][k] * a[k][i1];
                                        }
                                        a[j][i1] = (a[j][i1] - fdot[i1]) / a[i1][i1];
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

int main() {
    // Initialize random seed
    srand(static_cast<unsigned>(time(0)));

    Matrix a_naive(NSLIP, vector<double>(NSLIP, 0.0));

    for (int i = 0; i < NSLIP; i++) {
        for (int j = 0; j < NSLIP; j++) {
            double value = static_cast<double>(rand()) / RAND_MAX;
            a_naive[i][j] = value;
        }
    }

    vector<double> fdot_naive(NSLIP, 0.0);

    auto start_naive = chrono::high_resolution_clock::now();
    cholesky_ioopt(a_naive, fdot_naive, NSLIP);
    auto end_naive = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duration_naive = end_naive - start_naive;
    cout << "Execution time of cholesky_ioopt: " << duration_naive.count() << " ms\n";

    return 0;
}