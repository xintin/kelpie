#ifndef MATRIX_H
#define MATRIX_H

#include <iostream>
#include <vector>

class Matrix {
private:
  size_t rows;
  size_t cols;
  std::vector<std::vector<double>> data;

public:
  Matrix(size_t rows, size_t cols, double initial = 0.0);
  Matrix(const Matrix &other);
  ~Matrix();

  size_t getRows() const;
  size_t getCols() const;

  Matrix &operator=(const Matrix &other);
  double &operator()(const size_t &row, const size_t &col);
  const double &operator()(const size_t &row, const size_t &col) const;

  void mm(const Matrix &A, const Matrix &B);
  void tiledMM_kernel_ioopt(const Matrix &A, const Matrix &B);
  void tiledMM_kernel_pluto(const Matrix &A, const Matrix &B);

  void fill(double value);
  void randomize();   // Fills the matrix with random numbers
  void print() const; // Prints the matrix
  static bool matricesAreEqual(const Matrix &C1, const Matrix &C2);
};

class MatrixTester {
public:
  static void testTiledMM(const Matrix &A, const Matrix &B, int maxTileSize);

private:
  static void tiledMM_kernel_ioopt(Matrix &C, const Matrix &A, const Matrix &B,
                              size_t tileK, size_t tileJ, size_t tileI);
};

#endif // MATRIX_H