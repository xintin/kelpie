#include <cuda_runtime.h>
#include <iostream>
#include <vector>
#include <chrono>


#define MATRIX_SIZE 2048  // Fixed matrix size

__global__ void matMul5(float* B, float* C, float* A, int Width) {
    int tx = blockIdx.x * blockDim.x + threadIdx.x;
    int ty = blockIdx.y * blockDim.y + threadIdx.y;

    if (tx < Width && ty < Width) {  // Bounds check
        float Pvalue = 0.0;
        for (int k = 0; k < Width; ++k) {
            Pvalue += B[ty * Width + k] * C[k * Width + tx];
        }
        A[ty * Width + tx] = Pvalue;  // Corrected indexing
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
    matMul5<<<gridDim, blockDim>>>(d_B, d_C, d_A, matrixSize);  // Call matMul5
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
            std::cout << "Block Dim (" << blockDim.x << ", " << blockDim.y << ") - Time: " << time << " ms" << std::endl;
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
