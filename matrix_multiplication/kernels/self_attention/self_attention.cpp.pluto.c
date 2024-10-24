#include <math.h>
#define ceild(n,d)  ceil(((double)(n))/((double)(d)))
#define floord(n,d) floor(((double)(n))/((double)(d)))
#define max(x,y)    ((x) > (y)? (x) : (y))
#define min(x,y)    ((x) < (y)? (x) : (y))

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm> 
#include <numeric> 
#include <cstdlib>   // For rand()
#include <ctime>     // For time()

using namespace std;

// Define the matrix type for simplicity
using Matrix = vector<vector<double>>;

const int SEQ_LEN = 100000; // 10^5
const int EMB_DIM = 64;     // Embedding dimension

// Initialize matrices Q, K, V outside main function
Matrix Q(SEQ_LEN, vector<double>(EMB_DIM));
Matrix K(SEQ_LEN, vector<double>(EMB_DIM));
Matrix V(SEQ_LEN, vector<double>(EMB_DIM));

// Function to perform matrix multiplication
Matrix matmul(const Matrix& A, const Matrix& B) {
    int m = A.size();
    int n = B[0].size();
    int k = B[0].size();
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


Matrix self_attention_pluto(const Matrix& Q, const Matrix& K, const Matrix& V) {
    // Calculate QK^T
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

  for (t1=0;t1<=floord(qk_size-1,32);t1++) {
    for (t2=0;t2<=floord(qk_size-1,32);t2++) {
      for (t3=32*t1;t3<=min(qk_size-1,32*t1+31);t3++) {
        lbv=32*t2;
        ubv=min(qk_size-1,32*t2+31);

        for (t4=lbv;t4<=ubv;t4++) {
          QK_T[t3][t4] *= scale;;
        }
      }
    }
  }

    softmax(QK_T);

    return matmul(QK_T, V);
}

Matrix self_attention_naive(const Matrix& Q, const Matrix& K, const Matrix& V) {
    // Calculate QK^T
    int m = Q.size();
    int d_k = K[0].size();
    Matrix K_T(d_k, vector<double>(K.size()));
    for (int i = 0; i < K.size(); ++i) {
        for (int j = 0; j < K[0].size(); ++j) {
            K_T[j][i] = K[i][j];
        }
    }
    Matrix QK_T = matmul(Q, K_T);

    // Scale QK_T by the square root of the dimension of the key vectors
    double scale = 1.0 / sqrt(d_k);
    int qk_size = QK_T.size();

    for (int i = 0; i < qk_size; ++i) {
        for (int j = 0; j < qk_size; ++j) {
            QK_T[i][j] *= scale;
        }
    }


    // Apply softmax to QK_T
    softmax(QK_T);

    // Calculate the output as QK_T * V
    return matmul(QK_T, V);
}

Matrix self_attention_expt(const Matrix& Q, const Matrix& K, const Matrix& V) {
    // Calculate QK^T
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
  t1 = 64;
  t2 = 128;

for (i1 = 0; i1 <= (qk_size - 1) / t1; i1++) {
    for (i2 = 0; i2 <= (qk_size - 1) / t2; i2++) {
      for (i3 = t1 * i1; i3 <= std::min(qk_size - 1, t1 * i1 + t1 - 1); i3++) {
        lbv = t2 * i2;
        ubv = std::min(qk_size - 1, t2 * i2 + t2 - 1);

        for (i4 = lbv; i4 <= ubv; i4++) {
          QK_T[i3][i4] *= scale;
        }
      }
    }
  }

    softmax(QK_T);

    return matmul(QK_T, V);
}

int main() {
    // Initialize random seed
    std::srand(static_cast<unsigned>(std::time(0)));

    // Populate Q, K, V with random numbers
    for (int i = 0; i < SEQ_LEN; ++i) {
        for (int j = 0; j < EMB_DIM; ++j) {
            Q[i][j] = static_cast<double>(std::rand()) / RAND_MAX;
            K[i][j] = static_cast<double>(std::rand()) / RAND_MAX;
            V[i][j] = static_cast<double>(std::rand()) / RAND_MAX;
        }
    }

    Matrix output = self_attention(Q, K, V);

    return 0;
}
