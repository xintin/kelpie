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


    return 0;
}