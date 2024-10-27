#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <climits>
#include <tuple>
#include <cfloat>
#include <iterator> 

using namespace std;

using Matrix = vector<vector<double>>;

const int SEQ_LEN = 4000;
const int EMB_DIM = 64;

Matrix Q(SEQ_LEN, vector<double>(EMB_DIM));
Matrix K(SEQ_LEN, vector<double>(EMB_DIM));
Matrix V(SEQ_LEN, vector<double>(EMB_DIM));

const vector<int> allowed_values = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512};

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

Matrix self_attention_pluto(const Matrix& Q, const Matrix& K, const Matrix& V, int tile_size_t1, int tile_size_t2) {
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

void initialize_matrices() {
    srand(static_cast<unsigned>(time(0)));
    for (int i = 0; i < SEQ_LEN; ++i) {
        for (int j = 0; j < EMB_DIM; ++j) {
            Q[i][j] = static_cast<double>(rand()) / RAND_MAX;
            K[i][j] = static_cast<double>(rand()) / RAND_MAX;
            V[i][j] = static_cast<double>(rand()) / RAND_MAX;
        }
    }
}

double measure_performance(int ts1, int ts2, int runs) {
    double total_duration = 0.0;

    for (int i = 0; i < runs; ++i) {
        auto start = chrono::high_resolution_clock::now();
        Matrix output = self_attention_expt(Q, K, V, ts1, ts2);
        auto stop = chrono::high_resolution_clock::now();

        total_duration += chrono::duration_cast<chrono::milliseconds>(stop - start).count();
    }

    return total_duration / runs;
}

vector<tuple<int, int>> find_neighbors(int x, int y, int max_dist) {
    vector<tuple<int, int>> neighbors;

    auto it_x = find(allowed_values.begin(), allowed_values.end(), x);
    auto it_y = find(allowed_values.begin(), allowed_values.end(), y);

    if (it_x == allowed_values.end() || it_y == allowed_values.end()) {
        // x or y not found in allowed_values
        return neighbors;
    }

    int index_x = it_x - allowed_values.begin();
    int index_y = it_y - allowed_values.begin();

    int size = allowed_values.size();

    for (int dx = -max_dist; dx <= max_dist; dx++) {
        for (int dy = -max_dist; dy <= max_dist; dy++) {
            if (dx == 0 && dy == 0) continue;
            int nx_index = index_x + dx;
            int ny_index = index_y + dy;
            if (nx_index >= 0 && nx_index < size &&
                ny_index >= 0 && ny_index < size) {
                int nx = allowed_values[nx_index];
                int ny = allowed_values[ny_index];
                neighbors.emplace_back(nx, ny);
            }
        }
    }

    return neighbors;
}

tuple<int, int, double> move_to_minimum(int x, int y, int runs, int max_distance) {
    double best_performance = measure_performance(x, y, runs);
    tuple<int, int> best_config = make_tuple(x, y);

    for (int distance = 1; distance <= max_distance; distance++) {
        auto neighbors = find_neighbors(x, y, distance);
        bool found_better = false;

        for (auto& neighbor : neighbors) {
            int nx = get<0>(neighbor), ny = get<1>(neighbor);
            double perf = measure_performance(nx, ny, runs);
            if (perf < best_performance) {
                cout << "Found better performance at (" << nx << ", " << ny << "): " << perf << " ms\n";
                best_performance = perf;
                best_config = neighbor;
                found_better = true;
            }
        }

        if (found_better) {
            return make_tuple(get<0>(best_config), get<1>(best_config), best_performance);
        }
    }

    return make_tuple(x, y, best_performance);
}

tuple<int, int, double> coordinate_descent(int x, int y, int runs, int max_distance) {
    auto current_position = make_tuple(x, y, measure_performance(x, y, runs));
    double current_performance = get<2>(current_position);

    while (true) {
        auto next_position = move_to_minimum(get<0>(current_position), get<1>(current_position), runs, max_distance);
        double next_performance = get<2>(next_position);
        if (next_performance >= current_performance) {
            return current_position;
        }
        current_position = next_position;
        current_performance = next_performance;
    }
}

tuple<int, int, double> exhaustive_search(int runs) {
    int best_ts1 = 0, best_ts2 = 0;
    double best_performance = DBL_MAX;

    for (int ts1 : allowed_values) {
        for (int ts2 : allowed_values) {
            double performance = measure_performance(ts1, ts2, runs);
            cout << "Performance at (" << ts1 << ", " << ts2 << "): " << performance << " ms\n";
            if (performance < best_performance) {
                best_performance = performance;
                best_ts1 = ts1;
                best_ts2 = ts2;
            }
        }
    }

    return make_tuple(best_ts1, best_ts2, best_performance);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <runs> <max_distance>\n";
        return 1;
    }

    int runs = atoi(argv[1]);
    int max_distance = atoi(argv[2]);
    if (runs <= 0 || max_distance <= 0) {
        cerr << "Number of runs and max_distance must be positive integers.\n";
        return 1;
    }

    initialize_matrices();

    auto start_naive = chrono::high_resolution_clock::now();
    Matrix output_naive = self_attention_naive(Q, K, V);
    auto end_naive = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duration_naive = end_naive - start_naive;
    cout << "Execution time of self_attention_naive: " << duration_naive.count() << " ms \n";

    initialize_matrices();

    auto start_pluto = chrono::high_resolution_clock::now();
    Matrix output_pluto = self_attention_pluto(Q, K, V, 32, 32);
    auto end_pluto = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duration_pluto = end_pluto - start_pluto;
    cout << "Execution time of self_attention_pluto: " << duration_pluto.count() << " ms \n";


    initialize_matrices();

    int x = 1, y = 1;

    auto start_timer = chrono::high_resolution_clock::now();
    auto result = coordinate_descent(x, y, runs, max_distance);
    auto end_timer = chrono::high_resolution_clock::now();
    auto duration_timer = chrono::duration_cast<chrono::milliseconds>(end_timer - start_timer);

    cout << "Coordinate Descent converged to: (" << get<0>(result) << ", " << get<1>(result)
         << ") with performance: " << get<2>(result) << " milliseconds\n";
    cout << "Coordinate search took " << duration_timer.count() << " milliseconds to converge.\n";

    initialize_matrices();

    start_timer = chrono::high_resolution_clock::now();
    result = exhaustive_search(runs);
    end_timer = chrono::high_resolution_clock::now();
    duration_timer = chrono::duration_cast<chrono::milliseconds>(end_timer - start_timer);

    cout << "Exhaustive search converged to: (" << get<0>(result) << ", " << get<1>(result)
         << ") with performance: " << get<2>(result) << " milliseconds\n";
    cout << "Exhaustive search took " << duration_timer.count() << " milliseconds to complete.\n";

    return 0;
}