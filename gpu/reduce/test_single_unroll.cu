#include <stdio.h>
#include <cuda.h>
#include <vector>

#define N (1 << 28)  // Large array (16 million elements)
#define MAX_THREADS_PER_BLOCK 1024  // Hardware limit


__global__ void reduceKernelUnrolled(float *input, float *output, int size) {
    extern __shared__ float sdata[];

    int tid = threadIdx.x;
    int idx = blockIdx.x * blockDim.x * 2 + threadIdx.x;

    // Load elements into registers
    float sum = 0.0f;
    if (idx < size) sum += input[idx];
    if (idx + blockDim.x < size) sum += input[idx + blockDim.x];

    sdata[tid] = sum;
    __syncthreads();

    // Unrolled reduction in shared memory
    if (blockDim.x >= 1024 && tid < 512) sdata[tid] += sdata[tid + 512];
    __syncthreads();

    if (blockDim.x >= 512 && tid < 256) sdata[tid] += sdata[tid + 256];
    __syncthreads();

    if (blockDim.x >= 256 && tid < 128) sdata[tid] += sdata[tid + 128];
    __syncthreads();

    if (blockDim.x >= 128 && tid < 64) sdata[tid] += sdata[tid + 64];
    __syncthreads();

    // Warp-synchronous reduction (no need for __syncthreads())
    if (tid < 32) {
        volatile float* vsmem = sdata;  // Volatile to prevent compiler reordering
        vsmem[tid] += vsmem[tid + 32];
        vsmem[tid] += vsmem[tid + 16];
        vsmem[tid] += vsmem[tid + 8];
        vsmem[tid] += vsmem[tid + 4];
        vsmem[tid] += vsmem[tid + 2];
        vsmem[tid] += vsmem[tid + 1];
    }

    // Write the result from each block to global memory
    if (tid == 0) output[blockIdx.x] = sdata[0];
}


float runKernel(float* d_input, float* d_output, int numThreads) {
    int numBlocks = N / (2 * numThreads);

    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    cudaEventRecord(start);
    reduceKernelUnrolled<<<numBlocks, numThreads, numThreads * sizeof(float)>>>(d_input, d_output, N);
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
    size_t size = N * sizeof(float);
    float *h_input = (float*)malloc(size);
    float *h_output = (float*)malloc((N / 2) * sizeof(float));

    for (int i = 0; i < N; i++) {
        h_input[i] = 1.0f;
    }

    float *d_input, *d_output;
    cudaMalloc(&d_input, size);
    cudaMalloc(&d_output, (N / 2) * sizeof(float));
    cudaMemcpy(d_input, h_input, size, cudaMemcpyHostToDevice);
    cudaError_t mem_err = cudaGetLastError();
    if (mem_err != cudaSuccess) {
        printf("CUDA Memcpy Error: %s\n", cudaGetErrorString(mem_err));
        return -1;
    }

    std::vector<int> blockSizes = {1, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120, 128, 256, 512, 1024};

    for (int numThreads : blockSizes) {
        if (numThreads > MAX_THREADS_PER_BLOCK) continue;

        float totalTime = 0;

        for (int i = 0; i < 3; i++) {
            totalTime += runKernel(d_input, d_output, numThreads);
        }

        float avgTime = totalTime / 3.0;
        printf("Block Dim (%d) - Avg Time: %.2f ns\n", numThreads, avgTime);
    }

    cudaFree(d_input);
    cudaFree(d_output);
    free(h_input);
    free(h_output);

    return 0;
}
