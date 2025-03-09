#include <cuda_runtime.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>

#define MATRIX_SIZE 2048  // Fixed matrix size
#define TILE_SIZE 32      // Tile size for shared memory

__global__ void matrixMulTiledKernel(float* A, float* B, float* C, int n) {
    __shared__ float tile_A[TILE_SIZE][TILE_SIZE];
    __shared__ float tile_B[TILE_SIZE][TILE_SIZE];

    int row = blockIdx.y * TILE_SIZE + threadIdx.y;
    int col = blockIdx.x * TILE_SIZE + threadIdx.x;
    float value = 0.0f;

    for (int t = 0; t < (n + TILE_SIZE - 1) / TILE_SIZE; ++t) {
        if (row < n && t * TILE_SIZE + threadIdx.x < n)
            tile_A[threadIdx.y][threadIdx.x] = A[row * n + t * TILE_SIZE + threadIdx.x];
        else
            tile_A[threadIdx.y][threadIdx.x] = 0.0f;

        if (col < n && t * TILE_SIZE + threadIdx.y < n)
            tile_B[threadIdx.y][threadIdx.x] = B[(t * TILE_SIZE + threadIdx.y) * n + col];
        else
            tile_B[threadIdx.y][threadIdx.x] = 0.0f;

        __syncthreads();

        for (int k = 0; k < TILE_SIZE; ++k) {
            value += tile_A[threadIdx.y][k] * tile_B[k][threadIdx.x];
        }

        __syncthreads();
    }

    if (row < n && col < n) {
        C[row * n + col] = value;
    }
}

void initializeMatrix(float* matrix, int size) {
    for (int i = 0; i < size * size; ++i) {
        matrix[i] = static_cast<float>(i % 5 + 1);
    }
}

void matrixMulCPU(const float* A, const float* B, float* C, int n) {
    for (int row = 0; row < n; ++row) {
        for (int col = 0; col < n; ++col) {
            float sum = 0.0f;
            for (int k = 0; k < n; ++k) {
                sum += A[row * n + k] * B[k * n + col];
            }
            C[row * n + col] = sum;
        }
    }
}

float runKernel(float* d_A, float* d_B, float* d_C, int matrixSize) {
    dim3 blockDim(TILE_SIZE, TILE_SIZE);
    dim3 gridDim((matrixSize + TILE_SIZE - 1) / TILE_SIZE, (matrixSize + TILE_SIZE - 1) / TILE_SIZE);

    float totalNanoseconds = 0;
    cudaError_t err;

    for (int i = 0; i < 3; ++i) {
        cudaDeviceSynchronize();

        cudaEvent_t start, stop;
        cudaEventCreate(&start);
        cudaEventCreate(&stop);

        cudaEventRecord(start);
        matrixMulTiledKernel<<<gridDim, blockDim>>>(d_A, d_B, d_C, matrixSize);
        err = cudaGetLastError();
        if (err != cudaSuccess) {
            std::cerr << "CUDA Kernel Error: " << cudaGetErrorString(err) << std::endl;
            return -1;
        }
        cudaEventRecord(stop);
        cudaEventSynchronize(stop);
	cudaDeviceSynchronize();

        err = cudaGetLastError();
        if (err != cudaSuccess) {
            std::cerr << "CUDA Kernel Error: " << cudaGetErrorString(err) << std::endl;
            return -1;
        }

        float milliseconds = 0;
        cudaEventElapsedTime(&milliseconds, start, stop);
        cudaEventDestroy(start);
        cudaEventDestroy(stop);

        totalNanoseconds += milliseconds * 1e6;
    }

    return totalNanoseconds / 3.0f;
}

bool validateResults(float* h_C, float* d_C, int matrixSize) {
    for (int i = 0; i < matrixSize * matrixSize; ++i) {
        if (fabs(h_C[i] - d_C[i]) > 1e-5) {
            std::cerr << "Mismatch at index " << i << ": CPU " << h_C[i] << " vs GPU " << d_C[i] << std::endl;
            return false;
        }
    }
    return true;
}

int main() {
    int matrixSize = MATRIX_SIZE;
    int size = matrixSize * matrixSize * sizeof(float);

    float* h_A = new float[matrixSize * matrixSize];
    float* h_B = new float[matrixSize * matrixSize];
    float* h_C = new float[matrixSize * matrixSize];
    float* d_C_result = new float[matrixSize * matrixSize];

    initializeMatrix(h_A, matrixSize);
    initializeMatrix(h_B, matrixSize);

    float *d_A, *d_B, *d_C;
    cudaMalloc(&d_A, size);
    cudaMalloc(&d_B, size);
    cudaMalloc(&d_C, size);

    cudaMemcpy(d_A, h_A, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_B, h_B, size, cudaMemcpyHostToDevice);

    float avgTime = runKernel(d_A, d_B, d_C, matrixSize);

    if (avgTime >= 0) {
        std::cout << "Tiled Kernel Execution Time: " << avgTime << " ns" << std::endl;

        matrixMulCPU(h_A, h_B, h_C, matrixSize);
        cudaMemcpy(d_C_result, d_C, size, cudaMemcpyDeviceToHost);

        if (validateResults(h_C, d_C_result, matrixSize)) {
            std::cout << "\u2714 Validation Passed" << std::endl;
        } else {
            std::cerr << "\u274C Validation Failed" << std::endl;
        }
    }

    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);

    delete[] h_A;
    delete[] h_B;
    delete[] h_C;
    delete[] d_C_result;

    return 0;
}

