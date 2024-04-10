// matrix.cpp
#include "matrix.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>

Matrix::Matrix(size_t rows, size_t cols, double initial)
    : rows(rows), cols(cols), data(rows, std::vector<double>(cols, initial)) {}

Matrix::Matrix(const Matrix &other)
    : rows(other.rows), cols(other.cols), data(other.data) {}

Matrix::~Matrix() {}

size_t Matrix::getRows() const { return rows; }
size_t Matrix::getCols() const { return cols; }

Matrix &Matrix::operator=(const Matrix &other) {
  if (this != &other) {
    rows = other.rows;
    cols = other.cols;
    data = other.data;
  }
  return *this;
}

double &Matrix::operator()(const size_t &row, const size_t &col) {
  return data[row][col];
}

const double &Matrix::operator()(const size_t &row, const size_t &col) const {
  return data[row][col];
}

void Matrix::fill(double value) {
  for (auto &row : data) {
    std::fill(row.begin(), row.end(), value);
  }
}

void Matrix::randomize() {
  for (auto &row : data) {
    for (auto &elem : row) {
      elem = (double)rand() / RAND_MAX;
    }
  }
}

void Matrix::print() const {
  for (const auto &row : data) {
    for (const auto &elem : row) {
      std::cout << elem << " ";
    }
    std::cout << "\n";
  }
}

bool Matrix::matricesAreEqual(const Matrix &C1, const Matrix &C2) {
  if (C1.getRows() != C2.getRows() || C1.getCols() != C2.getCols())
    return false;
  for (size_t i = 0; i < C1.getRows(); ++i) {
    for (size_t j = 0; j < C1.getCols(); ++j) {
      if (std::fabs(C1(i, j) - C2(i, j)) > 1e-9) {
        return false;
      }
    }
  }
  return true;
}

void Matrix::mm(const Matrix &A, const Matrix &B) {
  if (A.cols != B.rows)
    throw std::invalid_argument("Matrix dimensions must agree.");

  size_t M = A.rows;
  size_t K = A.cols;
  size_t N = B.cols;

  this->data.resize(M, std::vector<double>(N, 0.0));
  this->rows = M;
  this->cols = N;

  // auto start = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < M; ++i) {
    for (size_t j = 0; j < N; ++j) {
      // this->data[i][j] = 0;
      for (size_t k = 0; k < K; ++k) {
        this->data[i][j] += A(i, k) * B(k, j);
      }
    }
  }

  // auto stop = std::chrono::high_resolution_clock::now();
  // auto duration =
  // std::chrono::duration_cast<std::chrono::duration<double>>(stop -
  // start).count();

  // return duration;
}

// tiled matrix multiplication
void Matrix::tiledMM_kernel1(const Matrix &A, const Matrix &B) {
  if (A.cols != B.rows) {
    throw std::invalid_argument(
        "Matrices A and B dimensions do not allow multiplication.");
  }

  size_t M = A.rows;
  size_t K = A.cols;
  size_t N = B.cols;

  this->data.resize(M, std::vector<double>(N, 0.0));
  this->rows = M;
  this->cols = N;

  // Tile sizes
  size_t tileK = 619;
  size_t tileJ = 230;
  size_t tileI = 140;

  // auto start = std::chrono::high_resolution_clock::now();

  for (size_t k2 = 0; k2 < K; k2 += tileK) {
    size_t Sk2 =
        std::min(tileK, K - k2); // Compute the actual size of the tile in K
    for (size_t j2 = 0; j2 < N; j2 += tileJ) {
      size_t Sj2 =
          std::min(tileJ, N - j2); // Compute the actual size of the tile in J
      for (size_t i2 = 0; i2 < M; i2 += tileI) {
        size_t Si2 =
            std::min(tileI, M - i2); // Compute the actual size of the tile in I
        for (size_t k1 = k2; k1 < k2 + Sk2; k1++) {
          for (size_t j = j2; j < j2 + Sj2; j++) {
            for (size_t i = i2; i < i2 + Si2; i++) {
              (*this)(i, j) += A(i, k1) * B(k1, j);
            }
          }
        }
      }
    }
  }

  // auto stop = std::chrono::high_resolution_clock::now();
  // auto duration =
  // std::chrono::duration_cast<std::chrono::duration<double>>(stop -
  // start).count();

  // return duration;
}