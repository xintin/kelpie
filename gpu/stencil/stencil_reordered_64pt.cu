#include <stdio.h>
#include <cuda.h>
#include <cmath>
#include <vector>

#define N 8192  // Matrix size

__global__ void j2d64pt(double* __restrict__ l_in, double* __restrict__ l_out) {
    int i0 = blockIdx.x * blockDim.x + 4;
    int i = max(i0, 4) + threadIdx.x;
    int j0 = 4 * blockIdx.y * blockDim.y + 4;
    int j = max(j0, 4) + 4 * threadIdx.y;

    double (*in)[N] = (double (*)[N]) l_in;
    double (*out)[N] = (double (*)[N]) l_out;

    if (i >= 4 && j >= 4 && i < N - 4 && j < N - 4) {
        double _t_1_ = -in[j-4][i-3] + in[j-4][i+3] - in[j-3][i-4] + in[j-3][i+4]
                     + in[j+4][i-3] - in[j+4][i+3] + in[j+3][i-4] - in[j+3][i+4];
        out[j][i] = _t_1_ * 0.000136017;
    }
}

void j2d64pt_cpu(double* __restrict__ l_in, double* __restrict__ l_out) {
    double (*in)[N] = (double (*)[N]) l_in;
    double (*out)[N] = (double (*)[N]) l_out;

    for (int j = 4; j < N - 4; ++j) {
        for (int i = 4; i < N - 4; ++i) {
            double _t_1_ = -in[j-4][i-3] + in[j-4][i+3] - in[j-3][i-4] + in[j-3][i+4]
                         + in[j+4][i-3] - in[j+4][i+3] + in[j+3][i-4] - in[j+3][i+4];
            out[j][i] = _t_1_ * 0.000136017;
        }
    }
}

bool validate_results(double* cpu_out, double* gpu_out) {
    for (int i = 0; i < N * N; ++i) {
        if (fabs(cpu_out[i] - gpu_out[i]) > 1e-5) {
            printf("Mismatch at index %d: CPU=%.6f, GPU=%.6f\n", i, cpu_out[i], gpu_out[i]);
            return false;
        }
    }
    return true;
}

float runKernel(double* d_in, double* d_out, dim3 blockDim) {
    dim3 gridDim((N + blockDim.x - 1) / blockDim.x, (N + 4 * blockDim.y - 1) / (4 * blockDim.y));

    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    cudaEventRecord(start);
    j2d64pt<<<gridDim, blockDim>>>(d_in, d_out);
    cudaDeviceSynchronize();

    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
        printf("CUDA Kernel Launch Error: %s\n", cudaGetErrorString(err));
        return -1;
    }

    cudaEventRecord(stop);
    cudaEventSynchronize(stop);

    float elapsed;
    cudaEventElapsedTime(&elapsed, start, stop);

    cudaEventDestroy(start);
    cudaEventDestroy(stop);

    return elapsed * 1000000;  // Convert to nanoseconds
}

int main() {
    size_t size = N * N * sizeof(double);
    double *h_in = (double*)malloc(size);
    double *h_out_gpu = (double*)malloc(size);
    double *h_out_cpu = (double*)malloc(size);

    for (int i = 0; i < N * N; i++) {
        h_in[i] = 1.0;
    }

    double *d_in, *d_out;
    if (cudaMalloc(&d_in, size) != cudaSuccess || cudaMalloc(&d_out, size) != cudaSuccess) {
        printf("CUDA malloc failed\n");
        return -1;
    }
    cudaMemcpy(d_in, h_in, size, cudaMemcpyHostToDevice);

    // int blockSizes[] = {8, 16, 32, 64, 128};
    std::vector<int> blockSizes = {1, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120, 128, 256, 512, 1024};

    for (int bx : blockSizes) {
        for (int by : blockSizes) {
            if (bx * by > 1024) continue;

            dim3 blockDim(bx, by);
            float totalTime = 0;

            for (int i = 0; i < 3; i++) {
                totalTime += runKernel(d_in, d_out, blockDim);
            }

            float avgTime = totalTime / 3.0;
            printf("Block Dim (%d, %d) - Avg Time: %.2f ns\n", bx, by, avgTime);

            cudaMemcpy(h_out_gpu, d_out, size, cudaMemcpyDeviceToHost);
            j2d64pt_cpu(h_in, h_out_cpu);

            if (validate_results(h_out_cpu, h_out_gpu)) {
                printf("Validation successful for Block Dim (%d, %d)\n", bx, by);
            } else {
                printf("Validation failed for Block Dim (%d, %d)\n", bx, by);
            }
        }
    }

    cudaFree(d_in);
    cudaFree(d_out);
    free(h_in);
    free(h_out_gpu);
    free(h_out_cpu);

    return 0;
}

