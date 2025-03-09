#include <cuda_runtime.h>
#include <cuda_profiler_api.h>

#include <iostream>
#include <iomanip>
#include <cstdlib>

// CUDA kernel for matrix multiplication
__global__ void matrixMulKernel(float* A, float* B, float* C, int n) {
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;

    if (row < n && col < n) {
        float value = 0.0;
        for (int k = 0; k < n; ++k) {
            value += A[row * n + k] * B[k * n + col];
        }
        C[row * n + col] = value;
    }
}

// Function to initialize matrices
void initializeMatrix(float* matrix, int size) {
    for (int i = 0; i < size * size; ++i) {
        matrix[i] = static_cast<float>(i % 5 + 1);
    }
}

// Function to calculate average kernel execution time
float runKernel(float* d_A, float* d_B, float* d_C, int matrixSize, dim3 blockDim, int numRuns) {
    dim3 gridDim((matrixSize + blockDim.x - 1) / blockDim.x,
                 (matrixSize + blockDim.y - 1) / blockDim.y);

    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    float totalMilliseconds = 0;

    // Start profiling for this configuration
    cudaProfilerStart();

    for (int i = 0; i < numRuns; ++i) {
        cudaEventRecord(start);
        matrixMulKernel<<<gridDim, blockDim>>>(d_A, d_B, d_C, matrixSize);
        cudaEventRecord(stop);
        cudaEventSynchronize(stop);

        float milliseconds = 0;
        cudaEventElapsedTime(&milliseconds, start, stop);
        totalMilliseconds += milliseconds;
    }

    // Stop profiling after iterations
    cudaProfilerStop();

    cudaEventDestroy(start);
    cudaEventDestroy(stop);

    return totalMilliseconds / numRuns; // Average execution time
}

// Function to calculate warp occupancy
void calculateOccupancy(dim3 blockDim) {
    cudaFuncAttributes attr;
    cudaFuncGetAttributes(&attr, (const void*)matrixMulKernel);

    int maxBlocksPerMultiprocessor;
    cudaOccupancyMaxActiveBlocksPerMultiprocessor(
        &maxBlocksPerMultiprocessor,
        (const void*)matrixMulKernel,
        blockDim.x * blockDim.y,
        0 // Shared memory (assume none for now)
    );

    int device;
    cudaGetDevice(&device);
    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, device);

    float activeWarpsPerBlock = (float)(blockDim.x * blockDim.y) / prop.warpSize;
    float maxWarpsPerSM = (float)prop.maxThreadsPerMultiProcessor / prop.warpSize;

    float occupancy = (float)(maxBlocksPerMultiprocessor * activeWarpsPerBlock) / maxWarpsPerSM;
    std::cout << "Block Dim (" << blockDim.x << ", " << blockDim.y << ") "
              << "Occupancy: " << std::fixed << std::setprecision(2) << occupancy * 100 << "%" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <Matrix Size> <Block Size> <Num Iterations>" << std::endl;
        return 1;
    }

    // Parse command-line arguments
    int matrixSize = std::atoi(argv[1]);
    int blockSize = std::atoi(argv[2]);
    int numRuns = std::atoi(argv[3]);

    if (matrixSize <= 0 || blockSize <= 0 || numRuns <= 0) {
        std::cerr << "Error: Matrix size, block size, and number of iterations must be positive integers." << std::endl;
        return 1;
    }

    // Define block and grid dimensions
    dim3 blockDim(blockSize, blockSize);
    dim3 gridDim((matrixSize + blockDim.x - 1) / blockDim.x,
                 (matrixSize + blockDim.y - 1) / blockDim.y);

    // Allocate host memory
    float* h_A = new float[matrixSize * matrixSize];
    float* h_B = new float[matrixSize * matrixSize];
    float* h_C = new float[matrixSize * matrixSize];

    initializeMatrix(h_A, matrixSize);
    initializeMatrix(h_B, matrixSize);

    // Allocate device memory
    float *d_A, *d_B, *d_C;
    cudaMalloc(&d_A, matrixSize * matrixSize * sizeof(float));
    cudaMalloc(&d_B, matrixSize * matrixSize * sizeof(float));
    cudaMalloc(&d_C, matrixSize * matrixSize * sizeof(float));

    // Copy host memory to device
    cudaMemcpy(d_A, h_A, matrixSize * matrixSize * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_B, h_B, matrixSize * matrixSize * sizeof(float), cudaMemcpyHostToDevice);

    // Analyze occupancy
    calculateOccupancy(blockDim);

    // Run kernel and measure average execution time
    float avgTime = runKernel(d_A, d_B, d_C, matrixSize, blockDim, numRuns);
    std::cout << "Average Time: " << avgTime << " ms" << std::endl;

    // Clean up
    delete[] h_A;
    delete[] h_B;
    delete[] h_C;
    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);

    return 0;
}
