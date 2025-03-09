#include <stdio.h>
#include <cuda.h>

/*
   This kernel performs a vectorized reduction (sum of an array)

   1. Each thread loads multiple elements from global memory, stores them in
      registers, and performs a local sum.
   2. A warp-based reduction is performed using shared memory.

   If we use too many threads per block, the register allocation per thread
   exceeds the GPU's limit, causing register spilling into slow global memory,
   reducing performance.

   Reducing the number of threads per block allows the kernel to fit within
   available registers, avoiding spills and improving efficiency.
 */

#define N (1 << 24)  // Large array (16 million elements)
#define MAX_THREADS_PER_BLOCK 1024  // Hardware limit

__global__ void reduceKernel(float *input, float *output, int size) {
    extern __shared__ float sdata[];  // Shared memory for reduction

    int tid = threadIdx.x;
    int idx = blockIdx.x * blockDim.x * 2 + threadIdx.x;

    // Local sum using registers to avoid slow global memory accesses
    float sum = 0.0f;
    if (idx < size) sum += input[idx];
    if (idx + blockDim.x < size) sum += input[idx + blockDim.x];

    sdata[tid] = sum;  // Store into shared memory
    __syncthreads();

    // Warp reduction in shared memory (parallel sum)
    for (int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (tid < s) {
            sdata[tid] += sdata[tid + s];
        }
        __syncthreads();
    }

    // Write block result to global memory
    if (tid == 0) output[blockIdx.x] = sdata[0];
}

int main() {
    float *h_input, *h_output;
    float *d_input, *d_output;
    int numBlocks, numThreads = 512;  // Start with 512 threads per block

    h_input = (float*) malloc(N * sizeof(float));
    h_output = (float*) malloc((N / (2 * numThreads)) * sizeof(float));

    cudaMalloc(&d_input, N * sizeof(float));
    cudaMalloc(&d_output, (N / (2 * numThreads)) * sizeof(float));

    for (int i = 0; i < N; i++) h_input[i] = 1.0f;

    cudaMemcpy(d_input, h_input, N * sizeof(float), cudaMemcpyHostToDevice);

    numBlocks = N / (2 * numThreads);

    reduceKernel<<<numBlocks, numThreads, numThreads * sizeof(float)>>>(d_input, d_output, N);

    cudaMemcpy(h_output, d_output, (N / (2 * numThreads)) * sizeof(float), cudaMemcpyDeviceToHost);

    float sum = 0.0f;
    for (int i = 0; i < numBlocks; i++) {
        sum += h_output[i];
    }

    printf("Sum: %f\n", sum);

    free(h_input);
    free(h_output);
    cudaFree(d_input);
    cudaFree(d_output);

    return 0;
}

