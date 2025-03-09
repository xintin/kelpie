#include <iostream>
#include <stdio.h>
#include <cuda.h>
#include <vector>

#define max(x,y)  ((x) > (y)? (x) : (y))
#define min(x,y)  ((x) < (y)? (x) : (y))
#define ceil(a,b) ((a) % (b) == 0 ? (a) / (b) : ((a) / (b)) + 1)

void check_error(const char* message) {
    cudaError_t error = cudaGetLastError();
    if (error != cudaSuccess) {
        printf("CUDA error: %s, %s\n", message, cudaGetErrorString(error));
        exit(-1);
    }
}

__global__ void hypterm_1(double* __restrict__ flux_0, double* __restrict__ flux_1, double* __restrict__ flux_2, 
                          double* __restrict__ flux_3, double* __restrict__ flux_4, double* __restrict__ cons_1, 
                          double* __restrict__ cons_2, double* __restrict__ cons_3, double* __restrict__ cons_4, 
                          double* __restrict__ q_1, double* __restrict__ q_2, double* __restrict__ q_3, 
                          double* __restrict__ q_4, double dxinv0, double dxinv1, double dxinv2, int L, int M, int N) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    int j = blockIdx.y * blockDim.y + threadIdx.y;
    int k = blockIdx.z * blockDim.z + threadIdx.z;

    if (i >= 4 && j >= 4 && k >= 4 && i <= N - 5 && j <= M - 5 && k <= L - 5) {
        flux_0[k * M * N + j * N + i] = -((0.8 * (cons_1[k * M * N + j * N + i + 1] - cons_1[k * M * N + j * N + i - 1]) -
            0.2 * (cons_1[k * M * N + j * N + i + 2] - cons_1[k * M * N + j * N + i - 2]) +
            0.038 * (cons_1[k * M * N + j * N + i + 3] - cons_1[k * M * N + j * N + i - 3]) -
            0.0035 * (cons_1[k * M * N + j * N + i + 4] - cons_1[k * M * N + j * N + i - 4])) * dxinv0);
    }
}

void initialize_data(double* data, int size) {
    for (int i = 0; i < size; ++i) {
        data[i] = static_cast<double>(i % 5 + 1);
    }
}

float runKernel(double* flux_0, double* flux_1, double* flux_2, double* flux_3, double* flux_4,
                double* cons_1, double* cons_2, double* cons_3, double* cons_4,
                double* q_1, double* q_2, double* q_3, double* q_4,
                double dxinv0, double dxinv1, double dxinv2,
                int L, int M, int N, dim3 blockDim) {

    dim3 gridDim(ceil(N, blockDim.x), ceil(M, blockDim.y), ceil(L, blockDim.z));
    float total_time = 0.0f;

    for (int i = 0; i < 3; ++i) {
        cudaEvent_t start, stop;
        cudaEventCreate(&start);
        cudaEventCreate(&stop);

        cudaEventRecord(start);
        hypterm_1<<<gridDim, blockDim>>>(flux_0, flux_1, flux_2, flux_3, flux_4,
                                         cons_1, cons_2, cons_3, cons_4, q_1, q_2, q_3, q_4,
                                         dxinv0, dxinv1, dxinv2, L, M, N);

        
	cudaError_t err = cudaGetLastError();
	if (err != cudaSuccess) {
    	    std::cerr << "CUDA Kernel Launch Error: " << cudaGetErrorString(err) << std::endl;
	}

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
    int L = 512, M = 1024, N = 1024;
    size_t size = L * M * N * sizeof(double);

    double *h_flux_0 = new double[L * M * N];
    double *h_flux_1 = new double[L * M * N];
    double *h_flux_2 = new double[L * M * N];
    double *h_flux_3 = new double[L * M * N];
    double *h_flux_4 = new double[L * M * N];
    double *h_cons_1 = new double[L * M * N];
    double *h_cons_2 = new double[L * M * N];
    double *h_cons_3 = new double[L * M * N];
    double *h_cons_4 = new double[L * M * N];
    double *h_q_1 = new double[L * M * N];
    double *h_q_2 = new double[L * M * N];
    double *h_q_3 = new double[L * M * N];
    double *h_q_4 = new double[L * M * N];

    initialize_data(h_cons_1, L * M * N);
    initialize_data(h_cons_2, L * M * N);
    initialize_data(h_cons_3, L * M * N);
    initialize_data(h_cons_4, L * M * N);
    initialize_data(h_q_1, L * M * N);
    initialize_data(h_q_2, L * M * N);
    initialize_data(h_q_3, L * M * N);
    initialize_data(h_q_4, L * M * N);

    double *d_flux_0, *d_flux_1, *d_flux_2, *d_flux_3, *d_flux_4;
    double *d_cons_1, *d_cons_2, *d_cons_3, *d_cons_4;
    double *d_q_1, *d_q_2, *d_q_3, *d_q_4;

    cudaMalloc(&d_flux_0, size);
    cudaMalloc(&d_flux_1, size);
    cudaMalloc(&d_flux_2, size);
    cudaMalloc(&d_flux_3, size);
    cudaMalloc(&d_flux_4, size);
    cudaMalloc(&d_cons_1, size);
    cudaMalloc(&d_cons_2, size);
    cudaMalloc(&d_cons_3, size);
    cudaMalloc(&d_cons_4, size);
    cudaMalloc(&d_q_1, size);
    cudaMalloc(&d_q_2, size);
    cudaMalloc(&d_q_3, size);
    cudaMalloc(&d_q_4, size);

    cudaMemcpy(d_cons_1, h_cons_1, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_cons_2, h_cons_2, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_cons_3, h_cons_3, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_cons_4, h_cons_4, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_q_1, h_q_1, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_q_2, h_q_2, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_q_3, h_q_3, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_q_4, h_q_4, size, cudaMemcpyHostToDevice);

    std::vector<int> blockSizes = {1, 8, 16, 24, 32, 40, 48, 56, 64, 128, 256, 1024, 2048};
    printf("Matrix Size: %dx%dx%d\n", L, M, N);

for (int bx : blockSizes) {
    for (int by : blockSizes) {
        for (int bz : blockSizes) {
            // Ensure the total threads per block do not exceed the hardware limit
            if (bx <= 1024 && by <= 1024 && bz <= 64 && (bx * by * bz) <= 1024) {

		dim3 blockDim(bx, by, bz);
                
                float time = runKernel(d_flux_0, d_flux_1, d_flux_2, d_flux_3, d_flux_4,
                                       d_cons_1, d_cons_2, d_cons_3, d_cons_4,
                                       d_q_1, d_q_2, d_q_3, d_q_4,
                                       1.0, 1.0, 1.0, L, M, N, blockDim);

                printf("(%d, %d, %d):%.2f\n", bx, by, bz, time);
            }
        }
    }
}
    delete[] h_flux_0; delete[] h_flux_1; delete[] h_flux_2;
    delete[] h_flux_3; delete[] h_flux_4;
    delete[] h_cons_1; delete[] h_cons_2; delete[] h_cons_3; delete[] h_cons_4;
    delete[] h_q_1; delete[] h_q_2; delete[] h_q_3; delete[] h_q_4;

    cudaFree(d_flux_0); cudaFree(d_flux_1); cudaFree(d_flux_2);
    cudaFree(d_flux_3); cudaFree(d_flux_4);
    cudaFree(d_cons_1); cudaFree(d_cons_2); cudaFree(d_cons_3); cudaFree(d_cons_4);
    cudaFree(d_q_1); cudaFree(d_q_2); cudaFree(d_q_3); cudaFree(d_q_4);

    return 0;
}
