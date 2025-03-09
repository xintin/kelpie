#include <cuda_runtime.h>
#include <iostream>
#include <vector>
#include <cmath>

#define MATRIX_SIZE 1000  // Fixed matrix size
#define TILE_WIDTH 4      // Tile size


__global__ void matMulTiledGlobal(const float* B, const float* C, float* A, int Width) {
    extern __shared__ float sharedMem[];

    float* tileB = sharedMem;
    float* tileC = &sharedMem[blockDim.x * blockDim.y];

    int tx = threadIdx.x;
    int ty = threadIdx.y;
    int Row = blockIdx.y * blockDim.y + ty;
    int Col = blockIdx.x * blockDim.x + tx;

    float Pvalue = 0.0f;

    int tiles = (Width + blockDim.x - 1) / blockDim.x;

    for (int m = 0; m < tiles; ++m) {
        // Load B's tile
        if (Row < Width && (m * blockDim.x + tx) < Width) {
            tileB[ty * blockDim.x + tx] = B[Row * Width + (m * blockDim.x + tx)];
        } else {
            tileB[ty * blockDim.x + tx] = 0.0f;
        }

        // Load C's tile
        if ((m * blockDim.y + ty) < Width && Col < Width) {
            tileC[ty * blockDim.x + tx] = C[(m * blockDim.y + ty) * Width + Col];
        } else {
            tileC[ty * blockDim.x + tx] = 0.0f;
        }

        __syncthreads();

        // Perform multiplication within the tile
        for (int k = 0; k < blockDim.x; ++k) {
            Pvalue += tileB[ty * blockDim.x + k] * tileC[k * blockDim.x + tx];
        }

        __syncthreads();
    }

    // Write the result back to global memory
    if (Row < Width && Col < Width) {
        A[Row * Width + Col] = Pvalue;
    }
}

bool validateResults(const float* A, const float* B, const float* C, int size) {
    for (int row = 0; row < size; ++row) {
        for (int col = 0; col < size; ++col) {
            float expected = 0.0f;
            for (int k = 0; k < size; ++k) {
                expected += B[row * size + k] * C[k * size + col];
            }
            if (fabs(A[row * size + col] - expected) > 1e-4) {
                std::cerr << "Mismatch at (" << row << ", " << col << ")! GPU: " << A[row * size + col] << ", CPU: " << expected << std::endl;
                return false;
            }
        }
    }
    return true;
}

float runKernel(float* d_A, const float* d_B, const float* d_C, int matrixSize, dim3 blockDim) {
    dim3 gridDim((matrixSize + blockDim.x - 1) / blockDim.x,
                 (matrixSize + blockDim.y - 1) / blockDim.y);

    // Allocate shared memory for both tileB and tileC
    size_t sharedMemSize = 2 * blockDim.x * blockDim.y * sizeof(float);

    float total_time = 0.0f;
    for (int i = 0; i < 3; ++i) {
        cudaEvent_t start, stop;
        cudaEventCreate(&start);
        cudaEventCreate(&stop);

        cudaEventRecord(start);
        matMulTiledGlobal<<<gridDim, blockDim, sharedMemSize>>>(d_B, d_C, d_A, matrixSize);

        cudaError_t err = cudaGetLastError();
        if (err != cudaSuccess) {
            std::cerr << "CUDA Kernel Error: " << cudaGetErrorString(err) << std::endl;
            return -1.0f;
        }

        cudaEventRecord(stop);
        cudaEventSynchronize(stop);

        float milliseconds = 0;
        cudaEventElapsedTime(&milliseconds, start, stop);
        total_time += milliseconds * 1e6;

        cudaEventDestroy(start);
        cudaEventDestroy(stop);
    }

    return total_time / 3;
}

void initializeMatrix(float* matrix, int size) {
    for (int i = 0; i < size * size; ++i) {
        matrix[i] = static_cast<float>(i % 5 + 1);
    }
}

int main() {
    int matrixSize = MATRIX_SIZE;
    int size = matrixSize * matrixSize * sizeof(float);

    float* h_A = new float[matrixSize * matrixSize];
    float* h_B = new float[matrixSize * matrixSize];
    float* h_C = new float[matrixSize * matrixSize];

    initializeMatrix(h_B, matrixSize);
    initializeMatrix(h_C, matrixSize);

    float *d_A, *d_B, *d_C;
    cudaMalloc(&d_A, size);
    cudaMalloc(&d_B, size);
    cudaMalloc(&d_C, size);

    cudaMemcpy(d_B, h_B, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_C, h_C, size, cudaMemcpyHostToDevice);

    std::vector<int> blockSizes = {1, 8, 16, 32, 64, 128, 256, 512, 1024};
    /*
    for (int i = 1; i <= 1024; i += 8) {
        blockSizes.push_back(i);
    }
    */

    std::cout << "Matrix Size: " << matrixSize << "x" << matrixSize << "\n";
    for (int bx : blockSizes) {
        for (int by : blockSizes) {
            if (bx * by > 1024) continue;
            dim3 blockDim(bx, by);
            float time = runKernel(d_A, d_B, d_C, matrixSize, blockDim);
            if (time >= 0) {
                cudaMemcpy(h_A, d_A, size, cudaMemcpyDeviceToHost);
                bool valid = validateResults(h_A, h_B, h_C, matrixSize);
                std::cout << "Block Dim (" << blockDim.x << ", " << blockDim.y << ") - Avg Time: " << time << " ns";
                if (valid) {
                    std::cout << " - Validation: PASSED\n";
                } else {
                    std::cout << " - Validation: FAILED\n";
                }
            }
        }
    }

    delete[] h_A;
    delete[] h_B;
    delete[] h_C;
    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);

    return 0;
}

