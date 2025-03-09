#include <cuda_runtime.h>
#include <iostream>
#include <vector>
#include <chrono>

#define MATRIX_SIZE 2048  // Fixed matrix size
#define TILE_WIDTH 4


__global__ void matMulOptimized(float* Md, float* Nd, float* Pd, int Width) {
    __shared__ float Mds[TILE_WIDTH][TILE_WIDTH];
    __shared__ float Nds[TILE_WIDTH][TILE_WIDTH];

    int tx = threadIdx.x;
    int ty = threadIdx.y;
    int Row = blockIdx.y * TILE_WIDTH + ty;
    int Col = blockIdx.x * TILE_WIDTH + tx;

    float Pvalue = 0.0;

    for (int m = 0; m < Width / TILE_WIDTH; ++m) {
        // Load tiles into shared memory
        if (Row < Width && (m * TILE_WIDTH + ty) < Width)
            Mds[tx][ty] = Md[Row * Width + (m * TILE_WIDTH + ty)];
        else
            Mds[tx][ty] = 0.0;  // Handle boundary case

        if (Col < Width && (m * TILE_WIDTH + tx) < Width)
            Nds[tx][ty] = Nd[Col + (m * TILE_WIDTH + tx) * Width];
        else
            Nds[tx][ty] = 0.0;

        __syncthreads();  // Ensure all threads have loaded data

        // Unrolled loop for better performance
        Pvalue += Mds[tx][0] * Nds[0][ty];
        Pvalue += Mds[tx][1] * Nds[1][ty];
        Pvalue += Mds[tx][2] * Nds[2][ty];
        Pvalue += Mds[tx][3] * Nds[3][ty];
        Pvalue += Mds[tx][4] * Nds[4][ty];
        Pvalue += Mds[tx][5] * Nds[5][ty];
        Pvalue += Mds[tx][6] * Nds[6][ty];
        Pvalue += Mds[tx][7] * Nds[7][ty];

        __syncthreads();  // Synchronize before next iteration
    }

    if (Row < Width && Col < Width)
        Pd[Row * Width + Col] = Pvalue;
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
    matMulOptimized<<<gridDim, blockDim>>>(d_B, d_C, d_A, matrixSize);  // Call matMul6
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
