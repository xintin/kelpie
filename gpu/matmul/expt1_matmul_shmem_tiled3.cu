#include <cuda_runtime.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>

#define MATRIX_SIZE 2048  // Fixed matrix size

__global__ void matrixMulTiledKernel(float* A, float* B, float* C, int n, int TILE_SIZE_X, int TILE_SIZE_Y) {
    extern __shared__ float sharedMemory[];
    float* tile_A = sharedMemory;
    float* tile_B = sharedMemory + TILE_SIZE_Y * TILE_SIZE_X;

    int row = blockIdx.y * TILE_SIZE_Y + threadIdx.y;
    int col = blockIdx.x * TILE_SIZE_X + threadIdx.x;
    float value = 0.0f;

    for (int t = 0; t < (n + TILE_SIZE_X - 1) / TILE_SIZE_X; ++t) {
        if (row < n && (t * TILE_SIZE_X + threadIdx.x) < n) {
            tile_A[threadIdx.y * TILE_SIZE_X + threadIdx.x] = A[row * n + t * TILE_SIZE_X + threadIdx.x];
        } else {
            tile_A[threadIdx.y * TILE_SIZE_X + threadIdx.x] = 0.0f;
        }

        if ((t * TILE_SIZE_X + threadIdx.y) < n && col < n) {
            tile_B[threadIdx.y * TILE_SIZE_X + threadIdx.x] = B[(t * TILE_SIZE_X + threadIdx.y) * n + col];
        } else {
            tile_B[threadIdx.y * TILE_SIZE_X + threadIdx.x] = 0.0f;
        }

        __syncthreads();

        for (int k = 0; k < TILE_SIZE_X; ++k) {
            value += tile_A[threadIdx.y * TILE_SIZE_X + k] * tile_B[k * TILE_SIZE_X + threadIdx.x];
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

float runKernel(float* d_A, float* d_B, float* d_C, int matrixSize, int TILE_SIZE_X, int TILE_SIZE_Y) {
    dim3 blockDim(TILE_SIZE_X, TILE_SIZE_Y);
    dim3 gridDim((matrixSize + TILE_SIZE_X - 1) / TILE_SIZE_X, (matrixSize + TILE_SIZE_Y - 1) / TILE_SIZE_Y);
    size_t sharedMemSize = 2 * TILE_SIZE_X * TILE_SIZE_Y * sizeof(float);

    float totalNanoseconds = 0;
    cudaError_t err;

    for (int i = 0; i < 3; ++i) {
        cudaDeviceSynchronize();

        cudaEvent_t start, stop;
        cudaEventCreate(&start);
        cudaEventCreate(&stop);

        cudaEventRecord(start);
        matrixMulTiledKernel<<<gridDim, blockDim, sharedMemSize>>>(d_A, d_B, d_C, matrixSize, TILE_SIZE_X, TILE_SIZE_Y);
        err = cudaGetLastError();
        if (err != cudaSuccess) {
            std::cerr << "CUDA Kernel Error: " << cudaGetErrorString(err) << std::endl;
            return -1;
        }
        cudaEventRecord(stop);
        cudaDeviceSynchronize();
        cudaEventSynchronize(stop);

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

    std::vector<int> TS = {1, 8, 16, 24, 32};
    for (int TILE_SIZE : TS) {
        if (TILE_SIZE * TILE_SIZE > 1024) continue;

        float avgTime = runKernel(d_A, d_B, d_C, matrixSize, TILE_SIZE, TILE_SIZE);

        if (avgTime >= 0) {
            std::cout << "Block Dim (" << TILE_SIZE << ", " << TILE_SIZE << ") Execution Time: " << avgTime << " ns" << std::endl;

            matrixMulCPU(h_A, h_B, h_C, matrixSize);
            cudaMemcpy(d_C_result, d_C, size, cudaMemcpyDeviceToHost);

            if (validateResults(h_C, d_C_result, matrixSize)) {
                std::cout << "\u2714 Validation Passed for Block Dim (" << TILE_SIZE << ", " << TILE_SIZE << ")" << std::endl;
            } else {
                std::cerr << "\u274C Validation Failed for Block Dim (" << TILE_SIZE << ", " << TILE_SIZE << ")" << std::endl;
            }
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
