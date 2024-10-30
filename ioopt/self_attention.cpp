#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm> 
#include <numeric> 
#include <cstdlib>   
#include <ctime>     
#include <chrono>    

using namespace std;

using Matrix = vector<vector<double>>;

const int SEQ_LEN = 4096;
const int EMB_DIM = 64;

Matrix Q(SEQ_LEN, vector<double>(EMB_DIM));
Matrix K(SEQ_LEN, vector<double>(EMB_DIM));
Matrix V(SEQ_LEN, vector<double>(EMB_DIM));

Matrix matmul(const Matrix& A, const Matrix& B) {
    int m = A.size();
    int n = B[0].size();
    int k = B.size();
    Matrix C(m, vector<double>(n, 0.0));

    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            for (int p = 0; p < k; ++p) {
                C[i][j] += A[i][p] * B[p][j];
            }
        }
    }
    return C;
}

void softmax(Matrix& mat) {
    for (auto& row : mat) {
        double max_val = *max_element(row.begin(), row.end());
        double sum = 0.0;
        for (auto& val : row) {
            val = exp(val - max_val);
            sum += val;
        }
        for (auto& val : row) {
            val /= sum;
        }
    }
}

Matrix self_attention_naive(const Matrix& Q, const Matrix& K, const Matrix& V) {
    int m = Q.size();
    int d_k = K[0].size();
    Matrix K_T(d_k, vector<double>(K.size()));

    for (int i = 0; i < K.size(); ++i) {
        for (int j = 0; j < K[0].size(); ++j) {
            K_T[j][i] = K[i][j];
        }
    }

    Matrix QK_T = matmul(Q, K_T);
    double scale = 1.0 / sqrt(d_k);
    int qk_size = QK_T.size();

    for (int i = 0; i < qk_size; ++i) {
        for (int j = 0; j < qk_size; ++j) {
            QK_T[i][j] *= scale;
        }
    }

    softmax(QK_T);
    return matmul(QK_T, V);
}

Matrix self_attention_tiled(const Matrix& Q, const Matrix& K, const Matrix& V) {
    int m = Q.size();
    int d_k = K[0].size();
    Matrix K_T(d_k, vector<double>(K.size()));

    // Tiled transposition of K
    for (int i3 = 0; i3 < K.size(); i3 += 5) {
        int Si3 = min(5, SEQ_LEN - i3);
        for (int i2 = i3; i2 < i3 + Si3; i2 += 3) {
            int Si2 = min(3, i3 + Si3 - i2);
            for (int i1 = i2; i1 < i2 + Si2; i1 += 1)
                for (int j1 = 0; j1 < SEQ_LEN; j1 += 98)
                    for (int k = 0; k < EMB_DIM; k += 1) {
                        int Sj1 = min(98, SEQ_LEN - j1);
                        for (int j = j1; j < j1 + Sj1; j += 1) {
                            K_T[k][i1] = K[i1][k];
                        }
                    }
        }
    }

    Matrix QK_T = matmul(Q, K_T);
    double scale = 1.0 / sqrt(d_k);
    int qk_size = QK_T.size();

    // Tiled scaling of QK_T
    for (int i3 = 0; i3 < qk_size; i3 += 5) {
        int Si3 = min(5, qk_size - i3);
        for (int i2 = i3; i2 < i3 + Si3; i2 += 3) {
            int Si2 = min(3, i3 + Si3 - i2);
            for (int i1 = i2; i1 < i2 + Si2; i1 += 1)
                for (int j1 = 0; j1 < qk_size; j1 += 98) {
                    int Sj1 = min(98, qk_size - j1);
                    for (int j = j1; j < j1 + Sj1; j += 1) {
                        QK_T[i1][j] *= scale;
                    }
                }
        }
    }

    softmax(QK_T);
    return matmul(QK_T, V);
}

int main() {
    srand(static_cast<unsigned>(time(0)));

    // Initialize matrices Q, K, V with random values
    for (int i = 0; i < SEQ_LEN; ++i) {
        for (int j = 0; j < EMB_DIM; ++j) {
            Q[i][j] = static_cast<double>(rand()) / RAND_MAX;
            K[i][j] = static_cast<double>(rand()) / RAND_MAX;
            V[i][j] = static_cast<double>(rand()) / RAND_MAX;
        }
    }

    // Time and execute self_attention_naive
    auto start_naive = chrono::high_resolution_clock::now();
    Matrix output_naive = self_attention_naive(Q, K, V);
    auto end_naive = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duration_naive = end_naive - start_naive;
    cout << "Naive version took " << duration_naive.count() << " ms" << endl;

    // Time and execute self_attention_tiled
    auto start_tiled = chrono::high_resolution_clock::now();
    Matrix output_tiled = self_attention_tiled(Q, K, V);
    auto end_tiled = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duration_tiled = end_tiled - start_tiled;
    cout << "Tiled version took " << duration_tiled.count() << " ms" << endl;

    return 0;
}