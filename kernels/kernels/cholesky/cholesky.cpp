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

inline int floord(int a, int b) {
    return a / b;
}

inline int ceild(int a, int b) {
    return (a + b - 1) / b;
}

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

// Tiled Cholesky decomposition function
void cholesky_tiled(Matrix& a, vector<double>& fdot, int nSlip, int t2, int t3, int t4, int t5) {
  int t1, t6;
  int lbv, ubv;

  for (t2=0;t2<=floord(nSlip-1,32);t2++) {
    lbv=max(1,32*t2);
    ubv=min(nSlip-1,32*t2+32-1);

    for (t3=lbv;t3<=ubv;t3++) {
      fdot[t3] = 0.0;;
    }
  }
  for (t2=0;t2<=floord(nSlip-1,32);t2++) {
    for (t3=t2;t3<=min(floord(nSlip-1,16),floord(32*t2+nSlip+30,32));t3++) {
      if (16*t3 == nSlip-1) {
        if ((nSlip+15)%16 == 0) {
          a[(nSlip-1)][(nSlip-1)] = a[(nSlip-1)][(nSlip-1)] - fdot[(nSlip-1)];;
        }
      }
      for (t4=max(max(1,32*t2),32*t3-nSlip+1);t4<=min(32*t2+31,16*t3-1);t4++) {
        for (t5=32*t3;t5<=min(32*t3+31,t4+nSlip-1);t5++) {
          fdot[t4] = 0.0;;
          for (t6=0;t6<=t4-1;t6++) {
            fdot[t4] += a[t4][t6] * a[t6][(-t4+t5)];;
          }
          a[t4][(-t4+t5)] = a[t4][(-t4+t5)] - fdot[t4];;
          fdot[t4] = 0.0;;
          for (t6=t4;t6<=2*t4-1;t6++) {
            fdot[t4] += a[(-t4+t5)][(-t4+t6)] * a[(-t4+t6)][t4];;
          }
          a[(-t4+t5)][t4] = (a[(-t4+t5)][t4] - fdot[t4]) / a[t4][t4];;
        }
      }
      if ((t2 <= floord(t3,2)) && (t2 >= ceild(t3-1,2)) && (t3 >= 1) && (t3 <= floord(nSlip-2,16))) {
        a[16*t3][16*t3] = a[16*t3][16*t3] - fdot[16*t3];;
        for (t5=32*t3+1;t5<=min(32*t3+31,16*t3+nSlip-1);t5++) {
          fdot[16*t3] = 0.0;;
          for (t6=0;t6<=16*t3-1;t6++) {
            fdot[16*t3] += a[16*t3][t6] * a[t6][(-16*t3+t5)];;
          }
          a[16*t3][(-16*t3+t5)] = a[16*t3][(-16*t3+t5)] - fdot[16*t3];;
          fdot[16*t3] = 0.0;;
          for (t6=16*t3;t6<=32*t3-1;t6++) {
            fdot[16*t3] += a[(-16*t3+t5)][(-16*t3+t6)] * a[(-16*t3+t6)][16*t3];;
          }
          a[(-16*t3+t5)][16*t3] = (a[(-16*t3+t5)][16*t3] - fdot[16*t3]) / a[16*t3][16*t3];;
        }
      }
      for (t4=max(32*t2,16*t3+1);t4<=min(min(nSlip-2,32*t2+31),16*t3+15);t4++) {
        for (t5=max(32*t3,t4);t5<=2*t4-1;t5++) {
          fdot[t4] += a[t4][(-t4+t5)] * a[(-t4+t5)][t4];;
        }
        a[t4][t4] = a[t4][t4] - fdot[t4];;
        for (t5=2*t4+1;t5<=min(32*t3+31,t4+nSlip-1);t5++) {
          fdot[t4] = 0.0;;
          for (t6=0;t6<=t4-1;t6++) {
            fdot[t4] += a[t4][t6] * a[t6][(-t4+t5)];;
          }
          a[t4][(-t4+t5)] = a[t4][(-t4+t5)] - fdot[t4];;
          fdot[t4] = 0.0;;
          for (t6=t4;t6<=2*t4-1;t6++) {
            fdot[t4] += a[(-t4+t5)][(-t4+t6)] * a[(-t4+t6)][t4];;
          }
          a[(-t4+t5)][t4] = (a[(-t4+t5)][t4] - fdot[t4]) / a[t4][t4];;
        }
      }
      if ((t2 >= ceild(nSlip-32,32)) && (t3 <= floord(nSlip-2,16)) && (t3 >= ceild(nSlip-16,16))) {
        for (t5=max(32*t3,nSlip-1);t5<=2*nSlip-3;t5++) {
          fdot[(nSlip-1)] += a[(nSlip-1)][(t5-nSlip+1)] * a[(t5-nSlip+1)][(nSlip-1)];;
        }
        a[(nSlip-1)][(nSlip-1)] = a[(nSlip-1)][(nSlip-1)] - fdot[(nSlip-1)];;
      }
      for (t4=max(32*t2,16*t3+16);t4<=min(nSlip-1,32*t2+31);t4++) {
        for (t5=max(32*t3,t4);t5<=32*t3+31;t5++) {
          fdot[t4] += a[t4][(-t4+t5)] * a[(-t4+t5)][t4];;
        }
      }
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
    vector<double> fdot_tiled(NSLIP, 0.0);

    // Time Cholesky decomposition naively
    auto start_naive = chrono::high_resolution_clock::now();
    cholesky_naive(a_naive, fdot_naive, NSLIP);
    auto end_naive = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duration_naive = end_naive - start_naive;
    cout << "Execution time of cholesky_naive: " << duration_naive.count() << " ms\n";

    int tile_size_t2 = 32;
    int tile_size_t3 = 16;
    int tile_size_t4 = 64;
    int tile_size_t5 = 96;

    // Time Cholesky decomposition with tiling
    auto start_tiled = chrono::high_resolution_clock::now();
    cholesky_tiled(a_tiled, fdot_tiled, NSLIP, tile_size_t2, tile_size_t3, tile_size_t4, tile_size_t5);
    auto end_tiled = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duration_tiled = end_tiled - start_tiled;
    cout << "Execution time of cholesky_tiled: " << duration_tiled.count() << " ms\n";

    bool same_outputs = true;
    for(int i = 0; i < NSLIP && same_outputs; i++) {
        for(int j = 0; j < NSLIP; j++) {
            if(abs(a_naive[i][j] - a_tiled[i][j]) > 1e-6) { 
                same_outputs = false;
                cout << "Difference found at (" << i << ", " << j << ")\n";
                break;
            }
        }
    }

    if(same_outputs) {
        cout << "Outputs of cholesky_naive and cholesky_tiled are the same." << endl;
    }
    else {
        cout << "Outputs of cholesky_naive and cholesky_tiled differ." << endl;
    }

    return 0;
}