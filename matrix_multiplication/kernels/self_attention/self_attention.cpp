#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm> 
#include <numeric> 
#include <cstdlib>   // For rand()
#include <ctime>     // For time()
#include <chrono>    // For timing

using namespace std;

// Define the matrix type for simplicity
using Matrix = vector<vector<double>>;

const int SEQ_LEN = 4096;   // 10^5
const int EMB_DIM = 64;     // Embedding dims

// Initialize matrices Q, K, V outside main function
Matrix Q(SEQ_LEN, vector<double>(EMB_DIM));
Matrix K(SEQ_LEN, vector<double>(EMB_DIM));
Matrix V(SEQ_LEN, vector<double>(EMB_DIM));

// Function to perform matrix multiplication
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

// Function to perform softmax on a matrix row-wise
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

Matrix self_attention_pluto(const Matrix& Q, const Matrix& K, const Matrix& V) {
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

    int t1, t2, t3, t4;
    int lbv, ubv;
    int tile_size = 32;

    for (t1 = 0; t1 <= (qk_size - 1) / tile_size; t1++) {
        for (t2 = 0; t2 <= (qk_size - 1) / tile_size; t2++) {
            for (t3 = tile_size * t1; t3 <= min(qk_size - 1, tile_size * t1 + tile_size - 1); t3++) {
                lbv = tile_size * t2;
                ubv = min(qk_size - 1, tile_size * t2 + tile_size - 1);

                for (t4 = lbv; t4 <= ubv; t4++) {
                    QK_T[t3][t4] *= scale;
                }
            }
        }
    }

    softmax(QK_T);

    return matmul(QK_T, V);
}

Matrix self_attention_expt(const Matrix& Q, const Matrix& K, const Matrix& V, int tile_size_t1, int tile_size_t2) {
    int m = Q.size();
    int d_k = K[0].size();
    int qk_size = m;

    // Transpose K to get K_T
    Matrix K_T(d_k, vector<double>(K.size()));
    for (int i = 0; i < K.size(); ++i) {
        for (int j = 0; j < d_k; ++j) {
            K_T[j][i] = K[i][j];
        }
    }

    // Compute QK^T
    Matrix QK_T = matmul(Q, K_T);

    double scale = 1.0 / sqrt(d_k);

    int t1, t2, t3, t4;
    int lbv, ubv;

    // Parametric tiling loops
    for (t1 = 0; t1 <= qk_size / tile_size_t1; t1++) {
        for (t2 = 0; t2 <= qk_size / tile_size_t2; t2++) {
            if ((qk_size >= 2) && (t1 == 0) && (t2 == 0)) {
                QK_T[0][0] *= scale;
                lbv = 1;
                ubv = min(tile_size_t2 - 1, qk_size - 1);

                for (t4 = lbv; t4 <= ubv; t4++) {
                    QK_T[0][t4] *= scale;
                }
            }
            if ((qk_size == 1) && (t1 == 0) && (t2 == 0)) {
                QK_T[0][0] *= scale;
            }
            if ((t1 == 0) && (t2 >= 1) && (t2 <= (qk_size - 1) / tile_size_t2)) {
                lbv = tile_size_t2 * t2;
                ubv = min(qk_size - 1, tile_size_t2 * t2 + tile_size_t2 - 1);

                for (t4 = lbv; t4 <= ubv; t4++) {
                    QK_T[0][t4] *= scale;
                }
            }
            if (t2 <= (qk_size - 1) / tile_size_t2) {
                for (t3 = max(1, tile_size_t1 * t1); t3 <= min(qk_size - 1, tile_size_t1 * t1 + tile_size_t1 - 1); t3++) {
                    lbv = tile_size_t2 * t2;
                    ubv = min(qk_size - 1, tile_size_t2 * t2 + tile_size_t2 - 1);

                    for (t4 = lbv; t4 <= ubv; t4++) {
                        QK_T[t3][t4] *= scale;
                    }
                }
            }
        }
    }

    // Apply softmax to QK_T
    softmax(QK_T);

    // Compute the output
    return matmul(QK_T, V);
}

int main() {
    srand(static_cast<unsigned>(time(0)));

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
    cout << "Execution time of self_attention_naive: " << duration_naive.count() << " ms" << endl;

    // Time and execute self_attention_pluto
    auto start_pluto = chrono::high_resolution_clock::now();
    Matrix output_pluto = self_attention_pluto(Q, K, V);
    auto end_pluto = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duration_pluto = end_pluto - start_pluto;
    cout << "Execution time of self_attention_pluto: " << duration_pluto.count() << " ms" << endl;

    // Time and execute self_attention_expt
    auto start_expt = chrono::high_resolution_clock::now();
    Matrix output_expt = self_attention_expt(Q, K, V, 64, 128);
    auto end_expt = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duration_expt = end_expt - start_expt;
    cout << "Execution time of self_attention_expt: " << duration_expt.count() << " ms" << endl;

    // Compare outputs
    bool same_naive_pluto = true;
    bool same_naive_expt = true;

    for (int i = 0; i < output_naive.size(); ++i) {
        for (int j = 0; j < output_naive[0].size(); ++j) {
            if (abs(output_naive[i][j] - output_pluto[i][j]) > 1e-6) {
                same_naive_pluto = false;
            }
            if (abs(output_naive[i][j] - output_expt[i][j]) > 1e-6) {
                same_naive_expt = false;
            }
        }
    }

    if (same_naive_pluto) {
        cout << "Outputs of naive and pluto methods are the same." << endl;
    } else {
        cout << "Outputs of naive and pluto methods differ." << endl;
    }

    if (same_naive_expt) {
        cout << "Outputs of naive and expt methods are the same." << endl;
    } else {
        cout << "Outputs of naive and expt methods differ." << endl;
    }

    return 0;
}