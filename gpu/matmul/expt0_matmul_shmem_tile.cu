#include <cuda_runtime.h>
#include <iostream>
#include <vector>
#include <chrono>

#define MATRIX_SIZE 2048  // Fixed matrix size
#define TILE_WIDTH 4

__global__ void matMul6(float* B, float* C, float* A, int Width) {
    __shared__ float Bs[TILE_WIDTH][TILE_WIDTH];
    __shared__ float Cs[TILE_WIDTH][TILE_WIDTH];

    int tx = threadIdx.x;
    int ty = threadIdx.y;
    int Row = blockIdx.y * TILE_WIDTH + ty;  // Fix Row indexing
    int Col = blockIdx.x * TILE_WIDTH + tx;  // Fix Col indexing

    float Pvalue = 0.0;

    for (int m = 0; m < Width / TILE_WIDTH; ++m) {
        // Load data into shared memory correctly
        if (Row < Width && (m * TILE_WIDTH + tx) < Width)
            Bs[ty][tx] = B[Row * Width + (m * TILE_WIDTH + tx)];
        else
            Bs[ty][tx] = 0.0;

        if (Col < Width && (m * TILE_WIDTH + ty) < Width)
            Cs[ty][tx] = C[(m * TILE_WIDTH + ty) * Width + Col];
        else
            Cs[ty][tx] = 0.0;

        __syncthreads();  // Synchronize threads in the block

        // Perform multiplication
        for (int k = 0; k < TILE_WIDTH; ++k) {
            Pvalue += Bs[ty][k] * Cs[k][tx];
        }
        __syncthreads();  // Ensure all computations complete
    }

    if (Row < Width && Col < Width)
        A[Row * Width + Col] = Pvalue;  // Fix final write order
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
    matMul6<<<gridDim, blockDim>>>(d_B, d_C, d_A, matrixSize);  // Call matMul6
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
            dim3 blockDim(bx, by);
            float time = runKernel(d_A, d_B, d_C, matrixSize, blockDim);
            std::cout << "Block Dim (" << blockDim.x << ", " << blockDim.y << ") - Time: " << time << " ns" << std::endl;
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
