#include <cuda_runtime.h>
#include <iostream>
#include <vector>
#include <cmath>

#define MATRIX_SIZE 2048  // Fixed matrix size

__global__ void matMul5(float* B, float* C, float* A, int Width) {
    int tx = blockIdx.x * blockDim.x + threadIdx.x;
    int ty = blockIdx.y * blockDim.y + threadIdx.y;

    if (tx < Width && ty < Width) {
        float Pvalue = 0.0;
        for (int k = 0; k < Width; ++k) {
            Pvalue += B[ty * Width + k] * C[k * Width + tx];
        }
        A[ty * Width + tx] = Pvalue;
    }
}

void initializeMatrix(float* matrix, int size) {
    for (int i = 0; i < size * size; ++i) {
        matrix[i] = static_cast<float>(i % 5 + 1);
    }
}

void matrixMulCPU(const float* B, const float* C, float* A, int n) {
    for (int row = 0; row < n; ++row) {
        for (int col = 0; col < n; ++col) {
            float sum = 0.0f;
            for (int k = 0; k < n; ++k) {
                sum += B[row * n + k] * C[k * n + col];
            }
            A[row * n + col] = sum;
        }
    }
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

float runKernel(float* d_A, float* d_B, float* d_C, int matrixSize, dim3 blockDim) {
    dim3 gridDim((matrixSize + blockDim.x - 1) / blockDim.x, 
                 (matrixSize + blockDim.y - 1) / blockDim.y);

    float totalNanoseconds = 0;
    cudaError_t err;

    for (int i = 0; i < 3; ++i) {
        cudaDeviceSynchronize();
        cudaEvent_t start, stop;
        cudaEventCreate(&start);
        cudaEventCreate(&stop);

        cudaEventRecord(start);
        matMul5<<<gridDim, blockDim>>>(d_B, d_C, d_A, matrixSize);
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

int main() {
    int matrixSize = MATRIX_SIZE;
    int size = matrixSize * matrixSize * sizeof(float);

    float* h_A = new float[matrixSize * matrixSize];
    float* h_B = new float[matrixSize * matrixSize];
    float* h_C = new float[matrixSize * matrixSize];
    float* d_C_result = new float[matrixSize * matrixSize];

    std::vector<int> blockSizes = {1, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120, 128, 256, 512, 1024};

    std::cout << "Matrix Size: " << matrixSize << "x" << matrixSize << std::endl;
    
    for (int bx : blockSizes) {
        for (int by : blockSizes) {
            if (bx * by > 1024) continue;

            initializeMatrix(h_B, matrixSize);
            initializeMatrix(h_C, matrixSize);

            float *d_A, *d_B, *d_C;
            cudaMalloc(&d_A, size);
            cudaMalloc(&d_B, size);
            cudaMalloc(&d_C, size);

            cudaMemcpy(d_B, h_B, size, cudaMemcpyHostToDevice);
            cudaMemcpy(d_C, h_C, size, cudaMemcpyHostToDevice);

            dim3 blockDim(bx, by);
            float avgTime = runKernel(d_A, d_B, d_C, matrixSize, blockDim);

            if (avgTime >= 0) {
                std::cout << bx << " " << by << " " << avgTime << " ns" << std::endl;

                matrixMulCPU(h_B, h_C, h_A, matrixSize);
                cudaMemcpy(d_C_result, d_A, size, cudaMemcpyDeviceToHost);

                if (validateResults(h_A, d_C_result, matrixSize)) {
                    continue;
                } else {
                    std::cerr << "\u274C Validation Failed for Block Dim (" << bx << ", " << by << ")" << std::endl;
                }
            }

            cudaFree(d_A);
            cudaFree(d_B);
            cudaFree(d_C);
        }
    }

    delete[] h_A;
    delete[] h_B;
    delete[] h_C;
    delete[] d_C_result;

    return 0;
}
