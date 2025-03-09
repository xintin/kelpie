#include <stdio.h>
#include <cuda.h>

#define N (1 << 28)  // Large array (268 million elements)
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

int main() {
    float *h_input, *h_output;
    float *d_input, *d_output;

    // Allocate memory
    h_input = (float*) malloc(N * sizeof(float));
    h_output = (float*) malloc((N / 2) * sizeof(float));  // Over-provisioning

    cudaMalloc(&d_input, N * sizeof(float));
    cudaMalloc(&d_output, (N / 2) * sizeof(float));

    // Initialize input array
    for (int i = 0; i < N; i++) h_input[i] = 1.0f;

    cudaMemcpy(d_input, h_input, N * sizeof(float), cudaMemcpyHostToDevice);

    printf("Threads per Block | Blocks      | Avg Time (ms)\n");
    printf("-------------------------------------------\n");

    // Loop over different thread configurations incrementing by 8
    for (int numThreads = 8; numThreads <= MAX_THREADS_PER_BLOCK; numThreads += 8) {
        int numBlocks = N / (2 * numThreads);

        float totalTime = 0.0f;

        // Run the kernel 3 times to take the average
        for (int run = 0; run < 3; ++run) {
            cudaEvent_t start, stop;
            cudaEventCreate(&start);
            cudaEventCreate(&stop);

            cudaEventRecord(start);
            reduceKernelUnrolled<<<numBlocks, numThreads, numThreads * sizeof(float)>>>(d_input, d_output, N);
        cudaEventRecord(stop);
        cudaEventSynchronize(stop);
        cudaDeviceSynchronize();

            float milliseconds = 0;
            cudaEventElapsedTime(&milliseconds, start, stop);

            totalTime += milliseconds;

            cudaEventDestroy(start);
            cudaEventDestroy(stop);
        }

        float avgTime = totalTime / 3.0f;
        printf("%16d | %10d | %8.4f ms\n", numThreads, numBlocks, avgTime);
    }

    // Free memory
    free(h_input);
    free(h_output);
    cudaFree(d_input);
    cudaFree(d_output);

    return 0;
}

