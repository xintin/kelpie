#include <stdio.h>
#include <cuda.h>
#include <vector>

#define N 8192  // Matrix size

__global__ void j2d64pt(double* __restrict__ l_in, double* __restrict__ l_out) {
    int i0 = blockIdx.x * blockDim.x + 4;
    int i = max(i0, 4) + threadIdx.x;
    int j0 = 4 * blockIdx.y * blockDim.y + 4;
    int j = max(j0, 4) + 4 * threadIdx.y;

    double (*in)[8192] = (double (*)[8192]) l_in;
    double (*out)[8192] = (double (*)[8192]) l_out;

    if (i >= 4 && j >= 4 && i <= N - 5 && j <= N - 5 &&
        i - 4 >= 0 && j - 4 >= 0 && i + 4 < N && j + 4 < N) {
        out[j][i] =
            (in[j-4][i-4] - in[j-4][i+4] - in[j+4][i-4] + in[j+4][i+4]) * 1.274495 +
            (-in[j-4][i-3] + in[j-4][i+3] + in[j-3][i+4] - in[j-3][i-4] + in[j+3][i-4] - in[j+3][i+4] + in[j+4][i-3] - in[j+4][i+3]) * 0.000136017 +
            (in[j-4][i-2] - in[j-4][i+2] + in[j-2][i-4] - in[j-2][i+4] - in[j+2][i-4] + in[j+2][i+4] - in[j+4][i-2] + in[j+4][i+2]) * 0.000714000 +
            (-in[j-4][i-1] + in[j-4][i+1] - in[j-1][i-4] + in[j-1][i+4] + in[j+1][i-4] - in[j+1][i+4] + in[j+4][i-1] - in[j+4][i+1]) * 0.00285600 +
            (in[j-3][i-3] - in[j-3][i+3] - in[j+3][i-3] + in[j+3][i+3]) * 0.00145161 +
            (-in[j-3][i-2] + in[j-3][i+2] - in[j-2][i-3] + in[j-2][i+3] + in[j+2][i-3] - in[j+2][i+3] + in[j+3][i-2] - in[j+3][i+2]) * 0.00762000 +
            (in[j-3][i-1] - in[j-3][i+1] + in[j-1][i-3] - in[j-1][i+3] - in[j+1][i-3] + in[j+1][i+3] - in[j+3][i-1] + in[j+3][i+1]) * 0.0304800 +
            (in[j-2][i-2] - in[j-2][i+2] - in[j+2][i-2] + in[j+2][i+2]) * 0.0400000 +
            (-in[j-2][i-1] + in[j-2][i+1] - in[j-1][i-2] + in[j-1][i+2] + in[j+1][i-2] - in[j+1][i+2] + in[j+2][i-1] - in[j+2][i+1]) * 0.160000 +
            (in[j-1][i-1] - in[j-1][i+1] - in[j+1][i-1] + in[j+1][i+1]) * 0.640000;
    }
}

float runKernel(double* d_in, double* d_out, dim3 blockDim) {
    dim3 gridDim((N + blockDim.x - 1) / blockDim.x, (N + 4 * blockDim.y - 1) / (4 * blockDim.y));

    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    cudaEventRecord(start);
    j2d64pt<<<gridDim, blockDim>>>(d_in, d_out);
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    cudaDeviceSynchronize();

    float elapsed;
    cudaEventElapsedTime(&elapsed, start, stop);

    cudaEventDestroy(start);
    cudaEventDestroy(stop);

    return elapsed * 1000000;  // Convert to nanoseconds
}

int main() {
    size_t size = N * N * sizeof(double);
    double *h_in = (double*)malloc(size);
    double *h_out = (double*)malloc(size);

    for (int i = 0; i < N * N; i++) {
        h_in[i] = 1.0;
    }

    double *d_in, *d_out;
    cudaMalloc(&d_in, size);
    cudaMalloc(&d_out, size);
    cudaMemcpy(d_in, h_in, size, cudaMemcpyHostToDevice);

    std::vector<int> blockSizes = {1, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120, 128, 256, 512, 1024};
    // int blockSizes[] = {8, 16, 32, 64, 128};

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
        }
    }

    cudaFree(d_in);
    cudaFree(d_out);
    free(h_in);
    free(h_out);

    return 0;
}
