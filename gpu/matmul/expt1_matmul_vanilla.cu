#include <cuda_runtime.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>

#define MATRIX_SIZE 2048  // Fixed matrix size

__global__ void matrixMulKernel(float* A, float* B, float* C, int n) {
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;

    if (row < n && col < n) {
        float value = 0.0f;
        for (int k = 0; k < n; ++k) {
            value += A[row * n + k] * B[k * n + col];
        }
        C[row * n + col] = value;
    }
}

void initializeMatrix(float* matrix, int size) {
    for (int i = 0; i < size * size; ++i) {
        matrix[i] = static_cast<float>(i % 5 + 1);
    }
}

// Reference CPU Matrix Multiplication for correctness check
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


dim3 getOptimalGridSize(int matrixSize, dim3& blockDim) {
    int gridX = (matrixSize + blockDim.x - 1) / blockDim.x;
    int gridY = (matrixSize + blockDim.y - 1) / blockDim.y;

    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, 0);

    // Ensure we don’t exceed max grid size limits
    gridX = min(gridX, prop.maxGridSize[0]);
    gridY = min(gridY, prop.maxGridSize[1]);

    return dim3(gridX, gridY);
}

// Runs kernel 3 times and returns the average execution time
float runKernel(float* d_A, float* d_B, float* d_C, int matrixSize, dim3 blockDim) {
    dim3 gridDim((matrixSize + blockDim.x - 1) / blockDim.x,
                (matrixSize + blockDim.y - 1) / blockDim.y);

    // dim3 gridDim = getOptimalGridSize(matrixSize, blockDim);
    // std::cout << "Grid Dim (" << gridDim.x << " " << gridDim.y << ")" << "\n";

    float totalNanoseconds = 0;
    cudaError_t err;

    for (int i = 0; i < 3; ++i) {
        cudaDeviceSynchronize();
	
	cudaEvent_t start, stop;
        cudaEventCreate(&start);
        cudaEventCreate(&stop);

	cudaEventRecord(start);
        matrixMulKernel<<<gridDim, blockDim>>>(d_A, d_B, d_C, matrixSize);
        err = cudaGetLastError();  // Check for kernel errors
        if (err != cudaSuccess) {
            std::cerr << "CUDA Kernel Error: " << cudaGetErrorString(err) << std::endl;
            return -1;
        }
        cudaEventRecord(stop);
        cudaEventSynchronize(stop);
	cudaDeviceSynchronize();

	err = cudaGetLastError();  // Check for kernel errors
        if (err != cudaSuccess) {
            std::cerr << "CUDA Kernel Error: " << cudaGetErrorString(err) << std::endl;
            return -1;
        }

        float milliseconds = 0;
        cudaEventElapsedTime(&milliseconds, start, stop);
        cudaEventDestroy(start);
        cudaEventDestroy(stop);

        totalNanoseconds += milliseconds * 1e6;  // Convert ms to ns
    }

    return totalNanoseconds / 3.0f;  // Return the average execution time
}

// Validates GPU results against CPU results
bool validateResults(float* h_C, float* d_C, int matrixSize) {
    for (int i = 0; i < matrixSize * matrixSize; ++i) {
        if (fabs(h_C[i] - d_C[i]) > 1e-5) {  // Small floating-point tolerance
            std::cerr << "Mismatch at index " << i << ": CPU " << h_C[i] << " vs GPU " << d_C[i] << std::endl;
            return false;
        }
    }
    return true;
}

int main() {
    int matrixSize = MATRIX_SIZE;
    int size = matrixSize * matrixSize * sizeof(float);

    // Allocate host memory
    float* h_A = new float[matrixSize * matrixSize];
    float* h_B = new float[matrixSize * matrixSize];
    float* h_C = new float[matrixSize * matrixSize];  // CPU result for validation
    float* d_C_result = new float[matrixSize * matrixSize];  // Device result for validation

    std::vector<int> blockSizes = {1, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120, 128, 256, 512, 1024};

    std::cout << "Matrix Size: " << matrixSize << "x" << matrixSize << std::endl;
    
    for (int bx : blockSizes) {
        for (int by : blockSizes) {
            if (bx * by > 1024) continue;  // Skip invalid block sizes
            
            // Initialize matrices before each experiment
            initializeMatrix(h_A, matrixSize);
            initializeMatrix(h_B, matrixSize);

            // Allocate device memory
            float *d_A, *d_B, *d_C;
            cudaMalloc(&d_A, size);
            cudaMalloc(&d_B, size);
            cudaMalloc(&d_C, size);

            // Copy matrices to device
            cudaMemcpy(d_A, h_A, size, cudaMemcpyHostToDevice);
            cudaMemcpy(d_B, h_B, size, cudaMemcpyHostToDevice);

            // Run the kernel
            dim3 blockDim(bx, by);
            float avgTime = runKernel(d_A, d_B, d_C, matrixSize, blockDim);
            
            if (avgTime >= 0) {  // Valid execution
                std::cout << bx << " " << by << " " << avgTime << " ns" << std::endl;

                // Compute reference result on CPU
                matrixMulCPU(h_A, h_B, h_C, matrixSize);

                // Copy GPU result back to host
                cudaMemcpy(d_C_result, d_C, size, cudaMemcpyDeviceToHost);

		// Validate GPU results against CPU results
                if (validateResults(h_C, d_C_result, matrixSize)) {
                    continue;
		    // std::cout << "✔ Validation Passed for Block Dim (" << bx << ", " << by << ")" << std::endl;
                } else {
                    std::cerr << "❌ Validation Failed for Block Dim (" << bx << ", " << by << ")" << std::endl;
                }
            }

            // Cleanup device memory
            cudaFree(d_A);
            cudaFree(d_B);
            cudaFree(d_C);
        }
    }

    // Cleanup host memory
    delete[] h_A;
    delete[] h_B;
    delete[] h_C;
    delete[] d_C_result;

    return 0;
}
