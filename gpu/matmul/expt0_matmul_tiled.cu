#include <cuda_runtime.h>
#include <iostream>
#include <vector>
#include <chrono>

#define MATRIX_SIZE 2048  // Fixed matrix size
#define TILE_WIDTH 4      // Tile size

__global__ void matMulTiledGlobal(float* B, float* C, float* A, int Width) {
    int tx = threadIdx.x;
    int ty = threadIdx.y;
    int Row = blockIdx.y * blockDim.y + ty;
    int Col = blockIdx.x * blockDim.x + tx;

    if (Row < Width && Col < Width) {
        float Pvalue = 0.0;
        for (int m = 0; m < Width; m += TILE_WIDTH) {  // Fixed tiling logic
            if (Row < Width && (m + tx) < Width && Col < Width && (m + ty) < Width) {
                Pvalue += B[Row * Width + (m + tx)] * C[(m + ty) * Width + Col];
            }
        }
        A[Row * Width + Col] = Pvalue;  // Ensure correct indexing
    }
}

void initializeMatrix(float* matrix, int size) {
    for (int i = 0; i < size * size; ++i) {
        matrix[i] = static_cast<float>(i % 5 + 1);
    }
}

float runKernel(float* d_A, float* d_B, float* d_C, int matrixSize, dim3 blockDim) {
    dim3 gridDim((matrixSize + blockDim.x - 1) / blockDim.x,
                 (matrixSize + blockDim.y - 1) / blockDim.y);

    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    cudaEventRecord(start);
    matMulTiledGlobal<<<gridDim, blockDim>>>(d_B, d_C, d_A, matrixSize);

    cudaError_t err = cudaGetLastError();  // Check for kernel launch error
    if (err != cudaSuccess) {
        std::cerr << "CUDA Kernel Error: " << cudaGetErrorString(err) << std::endl;
        return -1;  // Return error code
    }

    cudaEventRecord(stop);
    cudaEventSynchronize(stop);

    float milliseconds = 0;
    cudaEventElapsedTime(&milliseconds, start, stop);
    float nanoseconds = milliseconds * 1e6;

    cudaEventDestroy(start);
    cudaEventDestroy(stop);

    return nanoseconds;
}

int main() {
    int matrixSize = MATRIX_SIZE;
    int size = matrixSize * matrixSize * sizeof(float);

    // Allocate host memory
    float* h_A = new float[matrixSize * matrixSize];
    float* h_B = new float[matrixSize * matrixSize];
    float* h_C = new float[matrixSize * matrixSize];

    initializeMatrix(h_A, matrixSize);
    initializeMatrix(h_B, matrixSize);

    // Allocate device memory
    float *d_A, *d_B, *d_C;
    cudaMalloc(&d_A, size);
    cudaMalloc(&d_B, size);
    cudaMalloc(&d_C, size);

    // Copy host memory to device
    cudaMemcpy(d_A, h_A, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_B, h_B, size, cudaMemcpyHostToDevice);

    std::vector<int> blockSizes = {8, 16, 32, 64};

    std::cout << "Matrix Size: " << matrixSize << "x" << matrixSize << std::endl;
    for (int bx : blockSizes) {
        for (int by : blockSizes) {
            if (bx * by > 1024) continue;
            dim3 blockDim(bx, by);
            float time = runKernel(d_A, d_B, d_C, matrixSize, blockDim);
            if (time >= 0) {  // Only print valid results
                std::cout << "Block Dim (" << blockDim.x << ", " << blockDim.y << ") - Time: " << time << " ns" << std::endl;
            }
        }
    }

    // Cleanup
    delete[] h_A;
    delete[] h_B;
    delete[] h_C;
    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);

    return 0;
}
