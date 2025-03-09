#include <cuda_runtime.h>
#include <iostream>
#include <vector>
#include <cmath>

#define MATRIX_SIZE 138  // Adjust matrix size based on available memory
#define ceil(a,b) ((a) % (b) == 0 ? (a) / (b) : ((a) / (b)) + 1)

void checkCudaError(const char* message) {
    cudaError_t error = cudaGetLastError();
    if (error != cudaSuccess) {
        std::cerr << "CUDA error: " << message << " - " << cudaGetErrorString(error) << std::endl;
        exit(-1);
    }
}

__global__ void sw4(double* __restrict__ uacc_0, double* __restrict__ uacc_1, double* __restrict__ uacc_2,
                    double* __restrict__ u_0, double* __restrict__ u_1, double* __restrict__ u_2,
                    double* __restrict__ mu, double* __restrict__ la,
                    double* __restrict__ strx, double* __restrict__ stry, double* __restrict__ strz, int N) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    int j = blockIdx.y * blockDim.y + threadIdx.y;
    int k = blockIdx.z * blockDim.z + threadIdx.z;

    if (i >= 2 && j >= 2 && k >= 2 && i <= N - 3 && j <= N - 3 && k <= N - 3) {
        double r1 = mu[k * N * N + j * N + i] * strx[i] * stry[j] * strz[k];
        double r2 = la[k * N * N + j * N + i] * strx[i] * stry[j] * strz[k];
        double r3 = mu[k * N * N + j * N + i] * strx[i] * stry[j] * strz[k];

        uacc_0[k * N * N + j * N + i] += r1;
        uacc_1[k * N * N + j * N + i] += r2;
        uacc_2[k * N * N + j * N + i] += r3;
    }
}

void initializeMatrix(double* matrix, int size) {
    for (int i = 0; i < size; ++i) {
        matrix[i] = static_cast<double>(i % 5 + 1);
    }
}

float runKernel(double* d_uacc_0, double* d_uacc_1, double* d_uacc_2,
                double* d_u_0, double* d_u_1, double* d_u_2,
                double* d_mu, double* d_la,
                double* d_strx, double* d_stry, double* d_strz,
                int N, dim3 blockDim) {
    dim3 gridDim(ceil(N, blockDim.x), ceil(N, blockDim.y), ceil(N, blockDim.z));
    float total_time = 0.0f;

    for (int i = 0; i < 3; ++i) {
        cudaEvent_t start, stop;
        cudaEventCreate(&start);
        cudaEventCreate(&stop);

        cudaEventRecord(start);
        sw4<<<gridDim, blockDim>>>(d_uacc_0, d_uacc_1, d_uacc_2, d_u_0, d_u_1, d_u_2, d_mu, d_la, d_strx, d_stry, d_strz, N);
        cudaEventRecord(stop);
        cudaEventSynchronize(stop);
	cudaDeviceSynchronize();

        float milliseconds = 0;
        cudaEventElapsedTime(&milliseconds, start, stop);

        total_time += milliseconds * 1e6;

        cudaEventDestroy(start);
        cudaEventDestroy(stop);
    }

    return total_time / 3;
}

int main() {
    int N = MATRIX_SIZE;
    int size = N * N * N * sizeof(double);

    double* h_uacc_0 = new double[N * N * N];
    double* h_uacc_1 = new double[N * N * N];
    double* h_uacc_2 = new double[N * N * N];
    double* h_u_0 = new double[N * N * N];
    double* h_u_1 = new double[N * N * N];
    double* h_u_2 = new double[N * N * N];
    double* h_mu = new double[N * N * N];
    double* h_la = new double[N * N * N];
    double* h_strx = new double[N];
    double* h_stry = new double[N];
    double* h_strz = new double[N];

    initializeMatrix(h_uacc_0, N * N * N);
    initializeMatrix(h_uacc_1, N * N * N);
    initializeMatrix(h_uacc_2, N * N * N);
    initializeMatrix(h_u_0, N * N * N);
    initializeMatrix(h_u_1, N * N * N);
    initializeMatrix(h_u_2, N * N * N);
    initializeMatrix(h_mu, N * N * N);
    initializeMatrix(h_la, N * N * N);
    initializeMatrix(h_strx, N);
    initializeMatrix(h_stry, N);
    initializeMatrix(h_strz, N);

    double *d_uacc_0, *d_uacc_1, *d_uacc_2, *d_u_0, *d_u_1, *d_u_2, *d_mu, *d_la, *d_strx, *d_stry, *d_strz;

    cudaMalloc(&d_uacc_0, size);
    cudaMalloc(&d_uacc_1, size);
    cudaMalloc(&d_uacc_2, size);
    cudaMalloc(&d_u_0, size);
    cudaMalloc(&d_u_1, size);
    cudaMalloc(&d_u_2, size);
    cudaMalloc(&d_mu, size);
    cudaMalloc(&d_la, size);
    cudaMalloc(&d_strx, N * sizeof(double));
    cudaMalloc(&d_stry, N * sizeof(double));
    cudaMalloc(&d_strz, N * sizeof(double));

    cudaMemcpy(d_uacc_0, h_uacc_0, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_uacc_1, h_uacc_1, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_uacc_2, h_uacc_2, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_u_0, h_u_0, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_u_1, h_u_1, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_u_2, h_u_2, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_mu, h_mu, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_la, h_la, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_strx, h_strx, N * sizeof(double), cudaMemcpyHostToDevice);
    cudaMemcpy(d_stry, h_stry, N * sizeof(double), cudaMemcpyHostToDevice);
    cudaMemcpy(d_strz, h_strz, N * sizeof(double), cudaMemcpyHostToDevice);

    std::vector<int> blockSizes = {1, 8, 16, 24, 32, 40, 48, 56, 64, 128, 256, 512, 1024};
    std::cout << "Matrix Size: " << N << "x" << N << "x" << N << std::endl;

    for (int bx : blockSizes) {
        for (int by : blockSizes) {
            for (int bz : blockSizes) {
                if ((bx * by * bz > 1024) || (bz > 64)) continue;

                dim3 blockDim(bx, by, bz);
                float avgTime = runKernel(d_uacc_0, d_uacc_1, d_uacc_2, d_u_0, d_u_1, d_u_2, d_mu, d_la, d_strx, d_stry, d_strz, N, blockDim);

                if (avgTime >= 0) {
                    // std::cout << "Block Dim (" << bx << ", " << by << ", " << bz << ") - Avg Time: " << avgTime << " ns" << std::endl;
                    std::cout << "(" << bx << ", " << by << ", " << bz << "):" << avgTime << "\n";
		}
            }
        }
    }

    delete[] h_uacc_0;
    delete[] h_uacc_1;
    delete[] h_uacc_2;
    delete[] h_u_0;
    delete[] h_u_1;
    delete[] h_u_2;
    delete[] h_mu;
    delete[] h_la;
    delete[] h_strx;
    delete[] h_stry;
    delete[] h_strz;

    cudaFree(d_uacc_0);
    cudaFree(d_uacc_1);
    cudaFree(d_uacc_2);
    cudaFree(d_u_0);
    cudaFree(d_u_1);
    cudaFree(d_u_2);
    cudaFree(d_mu);
    cudaFree(d_la);
    cudaFree(d_strx);
    cudaFree(d_stry);
    cudaFree(d_strz);

    return 0;
}

