// utilities.h
#ifndef UTILITIES_H
#define UTILITIES_H

#include <vector>
#include <tuple>

// Declare the functions
void measure_performance();
std::vector<std::tuple<int, int>> find_neighbors(int x, int y);
std::tuple<int, int, int> move_to_minimum(int x, int y, int runs);
std::tuple<int, int, int> coordinate_descent(int x, int y, int runs);
std::tuple<int, int, int> exhaustive_search(int runs);
std::tuple<int, int, int> combined_exhaustive_search(int runs);

#endif // UTILITIES_H