#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>

using namespace std;

const int N = 10068;
using Matrix = vector<vector<int>>;

void bidirec_2d_ioopt(Matrix& a) {
    for (int i3 = 0; i3 < N; i3 += 86)
        for (int j3 = 0; j3 < N; j3 += 86) {
            int Si3 = min(86, N - i3);  // Values in {86, 6}
            for (int i2 = i3; i2 < i3 + Si3; i2 += 51) {
                int Sj3 = min(86, N - j3);  // Values in {86, 6}
                for (int j2 = j3; j2 < j3 + Sj3; j2 += 51) {
                    int Si2 = min(51, i3 + Si3 - i2);  // Values in {35, 51, 6}
                    for (int i1 = i2; i1 < i2 + Si2; i1 += 22) {
                        int Sj2 = min(51, j3 + Sj3 - j2);  // Values in {35, 51, 6}
                        for (int j1 = j2; j1 < j2 + Sj2; j1 += 22) {
                            int Si1 = min(22, i2 + Si2 - i1);  // Values in {6, 13, 22, 7}
                            for (int i = i1; i < i1 + Si1; i += 1) {
                                int Sj1 = min(22, j2 + Sj2 - j1);  // Values in {6, 13, 22, 7}
                                for (int j = j1; j < j1 + Sj1; j += 1) {
                                    if (i > 0) {  // Boundary check to avoid out-of-bounds access
                                        a[i][j] += a[i - 1][j] + a[i - 1][N - 1 - j];
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
    Matrix a(N, vector<int>(N, 1)); 

    auto start = chrono::high_resolution_clock::now();
    bidirec_2d_ioopt(a);
    auto end = chrono::high_resolution_clock::now();

    chrono::duration<double, milli> duration = end - start;
    cout << "Execution time of bidirec_2d_ioopt: " << duration.count() << " ms" << endl;

    return 0;
}