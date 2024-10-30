#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <tuple>
#include <cfloat>
#include <cmath>
#include <chrono>

using namespace std;

#define NSLIP 1000

inline int floord(int a, int b) {
    return a / b;
}

inline int ceild(int a, int b) {
    return (a + b - 1) / b;
}

using Matrix = vector<vector<double>>;

// Naive Cholesky decomposition function
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
            if (a[i][i] != 0.0) {
                a[j][i] = (a[j][i] - fdot[i]) / a[i][i];
            } else {
                cerr << "Division by zero encountered at a[" << i << "][" << i << "]." << endl;
                exit(EXIT_FAILURE);
            }
        }
    }
}

// Tiled Cholesky decomposition function
void cholesky_tiled(Matrix& a, vector<double>& fdot, int nSlip, int t2, int t3, int t4, int t5) {
    int t1, t6;
    int lbv, ubv;

    for (t2 = 0; t2 <= floord(nSlip-1,32); t2++) {
        lbv = max(1,32*t2);
        ubv = min(nSlip-1,32*t2+32-1);

        for (t3 = lbv; t3 <= ubv; t3++) {
            fdot[t3] = 0.0;
        }
    }
    for (t2 = 0; t2 <= floord(nSlip-1,32); t2++) {
        for (t3 = t2; t3 <= min(floord(nSlip-1,16), floord(32*t2+nSlip+30,32)); t3++) {
            if (16*t3 == nSlip-1) {
                if ((nSlip+15)%16 == 0) {
                    a[nSlip-1][nSlip-1] -= fdot[nSlip-1];
                }
            }
            for (t4 = max(max(1,32*t2),32*t3-nSlip+1); t4 <= min(32*t2+31,16*t3-1); t4++) {
                for (t5 = 32*t3; t5 <= min(32*t3+31,t4+nSlip-1); t5++) {
                    fdot[t4] = 0.0;
                    for (t6 = 0; t6 <= t4-1; t6++) {
                        int index = t5 - t4;
                        if (index < 0 || index >= nSlip) {
                            cerr << "Invalid index accessed at a[" << t4 << "][" << index << "]." << endl;
                            exit(EXIT_FAILURE);
                        }
                        fdot[t4] += a[t4][t6] * a[t6][index];
                    }
                    a[t4][t5 - t4] -= fdot[t4];
                    fdot[t4] = 0.0;
                    for (t6 = t4; t6 <= 2*t4-1 && t6 < nSlip; t6++) {
                        int row = t5 - t4;
                        int col = t6 - t4;
                        if (row < 0 || row >= nSlip || col < 0 || col >= nSlip) {
                            cerr << "Invalid index accessed at a[" << row << "][" << col << "]." << endl;
                            exit(EXIT_FAILURE);
                        }
                        fdot[t4] += a[row][col] * a[col][t4];
                    }
                    if (a[t4][t4] != 0.0) {
                        a[t5 - t4][t4] = (a[t5 - t4][t4] - fdot[t4]) / a[t4][t4];
                    } else {
                        cerr << "Division by zero encountered at a[" << t4 << "][" << t4 << "]." << endl;
                        exit(EXIT_FAILURE);
                    }
                }
            }
            if ((t2 <= floord(t3,2)) && (t2 >= ceild(t3-1,2)) && (t3 >= 1) && (t3 <= floord(nSlip-2,16))) {
                a[16*t3][16*t3] -= fdot[16*t3];
                for (t5 = 32*t3+1; t5 <= min(32*t3+31,16*t3+nSlip-1); t5++) {
                    fdot[16*t3] = 0.0;
                    for (t6 = 0; t6 <=16*t3-1; t6++) {
                        int index = t5 - 16*t3;
                        if (index < 0 || index >= nSlip) {
                            cerr << "Invalid index accessed at a[" << 16*t3 << "][" << index << "]." << endl;
                            exit(EXIT_FAILURE);
                        }
                        fdot[16*t3] += a[16*t3][t6] * a[t6][index];
                    }
                    a[16*t3][t5 - 16*t3] -= fdot[16*t3];
                    fdot[16*t3] = 0.0;
                    for (t6 = 16*t3; t6 <=32*t3-1 && t6 < nSlip; t6++) {
                        int row = t5 - 16*t3;
                        int col = t6 - 16*t3;
                        if (row < 0 || row >= nSlip || col < 0 || col >= nSlip) {
                            cerr << "Invalid index accessed at a[" << row << "][" << col << "]." << endl;
                            exit(EXIT_FAILURE);
                        }
                        fdot[16*t3] += a[row][col] * a[col][16*t3];
                    }
                    if (a[16*t3][16*t3] != 0.0) {
                        a[t5 - 16*t3][16*t3] = (a[t5 - 16*t3][16*t3] - fdot[16*t3]) / a[16*t3][16*t3];
                    } else {
                        cerr << "Division by zero encountered at a[" << 16*t3 << "][" << 16*t3 << "]." << endl;
                        exit(EXIT_FAILURE);
                    }
                }
            }
            for (t4 = max(32*t2,16*t3+1); t4 <= min(min(nSlip-2,32*t2+31),16*t3+15); t4++) {
                for (t5 = max(32*t3,t4); t5 <=2*t4-1 && t5 < nSlip; t5++) {
                    fdot[t4] += a[t4][t5 - t4] * a[t5 - t4][t4];
                }
                a[t4][t4] -= fdot[t4];
                for (t5 = 2*t4+1; t5 <= min(32*t3+31,t4+nSlip-1); t5++) {
                    fdot[t4] = 0.0;
                    for (t6 = 0; t6 <= t4-1; t6++) {
                        int index = t5 - t4;
                        if (index < 0 || index >= nSlip) {
                            cerr << "Invalid index accessed at a[" << t4 << "][" << index << "]." << endl;
                            exit(EXIT_FAILURE);
                        }
                        fdot[t4] += a[t4][t6] * a[t6][index];
                    }
                    a[t4][t5 - t4] -= fdot[t4];
                    fdot[t4] = 0.0;
                    for (t6 = t4; t6 <=2*t4-1 && t6 < nSlip; t6++) {
                        int row = t5 - t4;
                        int col = t6 - t4;
                        if (row < 0 || row >= nSlip || col < 0 || col >= nSlip) {
                            cerr << "Invalid index accessed at a[" << row << "][" << col << "]." << endl;
                            exit(EXIT_FAILURE);
                        }
                        fdot[t4] += a[row][col] * a[col][t4];
                    }
                    if (a[t4][t4] != 0.0) {
                        a[t5 - t4][t4] = (a[t5 - t4][t4] - fdot[t4]) / a[t4][t4];
                    } else {
                        cerr << "Division by zero encountered at a[" << t4 << "][" << t4 << "]." << endl;
                        exit(EXIT_FAILURE);
                    }
                }
            }
            if ((t2 >= ceild(nSlip-32,32)) && (t3 <= floord(nSlip-2,16)) && (t3 >= ceild(nSlip-16,16))) {
                for (t5 = max(32*t3,nSlip-1); t5 <=2*nSlip-3 && t5 < NSLIP; t5++) {
                    fdot[nSlip-1] += a[nSlip-1][t5-nSlip+1] * a[t5-nSlip+1][nSlip-1];
                }
                a[nSlip-1][nSlip-1] -= fdot[nSlip-1];
            }
            for (t4 = max(32*t2,16*t3+16); t4 <= min(nSlip-1,32*t2+31); t4++) {
                for (t5 = max(32*t3,t4); t5 <= min(32*t3+31,nSlip-1); t5++) {
                    fdot[t4] += a[t4][t5 - t4] * a[t5 - t4][t4];
                }
            }
        }
    }
}

// Function to measure performance of naive
double measure_performance_naive(const Matrix& initial_a, vector<double>& fdot_naive, int runs) {
    double total_duration = 0.0;
    for(int run = 0; run < runs; ++run) {
        Matrix a = initial_a;
        vector<double> fdot = fdot_naive;
        auto start = chrono::high_resolution_clock::now();
        cholesky_naive(a, fdot, NSLIP);
        auto end = chrono::high_resolution_clock::now();
        total_duration += chrono::duration_cast<chrono::milliseconds>(end - start).count();
    }
    return total_duration / runs;
}

// Function to measure performance of tiled
double measure_performance_tiled(const Matrix& initial_a, vector<double> fdot_tiled, int runs, int t2, int t3, int t4, int t5) {
    double total_duration = 0.0;
    for(int run = 0; run < runs; ++run) {
        Matrix a = initial_a;
        vector<double> fdot = fdot_tiled;
        auto start = chrono::high_resolution_clock::now();
        cholesky_tiled(a, fdot, NSLIP, t2, t3, t4, t5);
        auto end = chrono::high_resolution_clock::now();
        total_duration += chrono::duration_cast<chrono::milliseconds>(end - start).count();
    }
    return total_duration / runs;
}

// Function to find neighboring tile sizes
vector<tuple<int, int, int, int>> find_neighbors(int ts2, int ts3, int ts4, int ts5, const vector<int>& allowed_values, int distance) {
    vector<tuple<int, int, int, int>> neighbors;
    for(int i = 0; i < 4; ++i) {
        int current = (i == 0) ? ts2 : (i == 1) ? ts3 : (i == 2) ? ts4 : ts5;
        auto it = find(allowed_values.begin(), allowed_values.end(), current);
        if(it != allowed_values.end()) {
            int index = std::distance(allowed_values.begin(), it);
            for(int dist = 1; dist <= distance; ++dist) {
                if(index - dist >= 0) {
                    int prev = allowed_values[index - dist];
                    if(i == 0) neighbors.emplace_back(prev, ts3, ts4, ts5);
                    else if(i == 1) neighbors.emplace_back(ts2, prev, ts4, ts5);
                    else if(i == 2) neighbors.emplace_back(ts2, ts3, prev, ts5);
                    else neighbors.emplace_back(ts2, ts3, ts4, prev);
                }
                if(index + dist < allowed_values.size()) {
                    int next = allowed_values[index + dist];
                    if(i == 0) neighbors.emplace_back(next, ts3, ts4, ts5);
                    else if(i == 1) neighbors.emplace_back(ts2, next, ts4, ts5);
                    else if(i == 2) neighbors.emplace_back(ts2, ts3, next, ts5);
                    else neighbors.emplace_back(ts2, ts3, ts4, next);
                }
            }
        }
    }
    cout << "Neighbor size: " << neighbors.size() << "\n";
    return neighbors;
}

// Function to move to minimum performance configuration
tuple<int, int, int, int, double> move_to_minimum(int ts2, int ts3, int ts4, int ts5, int runs, const vector<int>& allowed_values, int distance, const Matrix& initial_a) {
    vector<tuple<int, int, int, int>> neighbors = find_neighbors(ts2, ts3, ts4, ts5, allowed_values, distance);
    double best_performance = measure_performance_tiled(initial_a, vector<double>(NSLIP, 0.0), runs, ts2, ts3, ts4, ts5);
    tuple<int, int, int, int> best_config = make_tuple(ts2, ts3, ts4, ts5);
    for(auto &neighbor : neighbors) {
        int n2 = get<0>(neighbor);
        int n3 = get<1>(neighbor);
        int n4 = get<2>(neighbor);
        int n5 = get<3>(neighbor);
        double perf = measure_performance_tiled(initial_a, vector<double>(NSLIP, 0.0), runs, n2, n3, n4, n5);
        if(perf < best_performance) {
            best_performance = perf;
            best_config = neighbor;
        }
    }
    return make_tuple(get<0>(best_config), get<1>(best_config), get<2>(best_config), get<3>(best_config), best_performance);
}

// Coordinate Descent Optimization
tuple<int, int, int, int, double> coordinate_descent(int ts2, int ts3, int ts4, int ts5, int runs, const vector<int>& allowed_values, int distance, const Matrix& initial_a) {
    double initial_perf = measure_performance_tiled(initial_a, vector<double>(NSLIP, 0.0), runs, ts2, ts3, ts4, ts5);
    cout << "Starting position: (" << ts2 << ", " << ts3 << ", " << ts4 << ", " << ts5 << ") with performance: " 
         << initial_perf << " ms\n";
    tuple<int, int, int, int, double> current_position = move_to_minimum(ts2, ts3, ts4, ts5, runs, allowed_values, distance, initial_a);
    double current_performance = get<4>(current_position);
    while(true) {
        tuple<int, int, int, int, double> next_position = move_to_minimum(get<0>(current_position), get<1>(current_position), get<2>(current_position), get<3>(current_position), runs, allowed_values, distance, initial_a);
        double next_performance = get<4>(next_position);
        if(next_performance >= current_performance) {
            return current_position;
        }
        current_position = next_position;
        current_performance = next_performance;
        cout << "Tile size (t2, t3, t4, t5): (" << get<0>(current_position) << ", " 
             << get<1>(current_position) << ", " << get<2>(current_position) << ", " 
             << get<3>(current_position) << ") with performance: " << current_performance << " ms\n";
    }
}

// Exhaustive Search
tuple<int, int, int, int, double> exhaustive_search(int runs, const vector<int>& allowed_values, const Matrix& initial_a) {
    int best_ts2 = 0, best_ts3 = 0, best_ts4 = 0, best_ts5 = 0;
    double best_performance = DBL_MAX;
    cout << "Starting Exhaustive Search...\n";
    for(int ts2 : allowed_values) {
        for(int ts3 : allowed_values) {
            for(int ts4 : allowed_values) {
                for(int ts5 : allowed_values) {
                    double performance = measure_performance_tiled(initial_a, vector<double>(NSLIP, 0.0), runs, ts2, ts3, ts4, ts5);
                    cout << "Performance at tile_size (t2, t3, t4, t5) = (" << ts2 << ", " << ts3 << ", " << ts4 << ", " << ts5 << "): " 
                         << performance << " ms\n";
                    if(performance < best_performance) {
                        best_performance = performance;
                        best_ts2 = ts2;
                        best_ts3 = ts3;
                        best_ts4 = ts4;
                        best_ts5 = ts5;
                    }
                }
            }
        }
    }
    return make_tuple(best_ts2, best_ts3, best_ts4, best_ts5, best_performance);
}

int main(int argc, char* argv[]) {
    vector<int> allowed_values = {1, 2, 4, 8, 16, 32, 64, 128, 256};
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
    srand(42); // Fixed seed for reproducibility

    // Initialize matrices to be symmetric positive-definite
    Matrix a_naive(NSLIP, vector<double>(NSLIP, 0.0));
    Matrix a_tiled(NSLIP, vector<double>(NSLIP, 0.0));
    Matrix a_pluto(NSLIP, vector<double>(NSLIP, 0.0));
    for(int i = 0; i < NSLIP; i++) {
        for(int j = 0; j <= i; j++) { // Ensure symmetry by iterating j <= i
            double value = static_cast<double>(rand()) / RAND_MAX;
            a_naive[i][j] = value;
            a_naive[j][i] = value; // Symmetric assignment
            
            a_tiled[i][j] = value;
            a_tiled[j][i] = value; // Symmetric assignment
            
            a_pluto[i][j] = value;
            a_pluto[j][i] = value; // Symmetric assignment
        }
        // Make the matrix diagonally dominant to ensure positive-definiteness
        a_naive[i][i] += NSLIP;
        a_tiled[i][i] += NSLIP;
        a_pluto[i][i] += NSLIP;
    }
    vector<double> fdot_naive(NSLIP, 0.0);
    vector<double> fdot_tiled(NSLIP, 0.0);
    vector<double> fdot_pluto(NSLIP, 0.0);

    // Run Naive Cholesky Decomposition
    auto start_naive = chrono::high_resolution_clock::now();
    cholesky_naive(a_naive, fdot_naive, NSLIP);
    auto end_naive = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duration_naive = end_naive - start_naive;
    cout << "Execution time of cholesky_naive: " << duration_naive.count() << " ms\n";

    // Run Pluto (Naive tiled with tile sizes 32,32,32,32)
    auto start_pluto = chrono::high_resolution_clock::now();
    double pluto_performance = measure_performance_tiled(a_pluto, vector<double>(NSLIP, 0.0), runs, 32, 32, 32, 32);
    auto end_pluto = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duration_pluto = end_pluto - start_pluto;
    cout << "Execution time of cholesky_pluto (32,32,32,32): " << pluto_performance << " ms\n";

    // Run Coordinate Descent
    // Initialize with tile sizes (32,32,32,32)
    int initial_ts2 = 1, initial_ts3 = 1, initial_ts4 = 1, initial_ts5 = 1;
    auto start_coordinate = chrono::high_resolution_clock::now();
    auto coordinate_result = coordinate_descent(initial_ts2, initial_ts3, initial_ts4, initial_ts5, runs, allowed_values, max_distance, a_tiled);
    auto end_coordinate = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duration_coordinate = end_coordinate - start_coordinate;
    cout << "Coordinate Descent completed in " << duration_coordinate.count() << " ms\n";
    int coord_ts2 = get<0>(coordinate_result);
    int coord_ts3 = get<1>(coordinate_result);
    int coord_ts4 = get<2>(coordinate_result);
    int coord_ts5 = get<3>(coordinate_result);
    double coord_performance = get<4>(coordinate_result);
    cout << "Best tile sizes from Coordinate Descent: t2=" << coord_ts2 << ", t3=" << coord_ts3 
         << ", t4=" << coord_ts4 << ", t5=" << coord_ts5 << "\n";
    cout << "Best performance from Coordinate Descent: " << coord_performance << " ms\n";

    // Run Exhaustive Search
    auto start_exhaustive = chrono::high_resolution_clock::now();
    auto exhaustive_result = exhaustive_search(runs, allowed_values, a_tiled);
    auto end_exhaustive = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duration_exhaustive = end_exhaustive - start_exhaustive;
    cout << "Exhaustive Search completed in " << duration_exhaustive.count() << " ms\n";
    int exhaustive_ts2 = get<0>(exhaustive_result);
    int exhaustive_ts3 = get<1>(exhaustive_result);
    int exhaustive_ts4 = get<2>(exhaustive_result);
    int exhaustive_ts5 = get<3>(exhaustive_result);
    double exhaustive_performance = get<4>(exhaustive_result);
    cout << "Best tile sizes from Exhaustive Search: t2=" << exhaustive_ts2 << ", t3=" << exhaustive_ts3 
         << ", t4=" << exhaustive_ts4 << ", t5=" << exhaustive_ts5 << "\n";
    cout << "Best performance from Exhaustive Search: " << exhaustive_performance << " ms\n";

    return 0;
}