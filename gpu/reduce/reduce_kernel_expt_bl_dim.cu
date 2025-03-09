#include <stdio.h>
#include <cuda.h>

#define N (1 << 24)  // Large array (16 million elements)
#define MAX_THREADS_PER_BLOCK 1024  // Hardware limit

__global__ void reduceKernel(float *input, float *output, int size) {
    extern __shared__ float sdata[];

    int tid = threadIdx.x;
    int idx = blockIdx.x * blockDim.x * 2 + threadIdx.x;

    // Introduce additional variables and more complex operations
    float sum = 0.0f, temp1 = 0.0f, temp2 = 0.0f, temp3 = 0.0f, temp4 = 0.0f, temp5 = 0.0f;
    if (idx < size) {
        sum += input[idx];
        temp1 = sum * 2.0f;
        temp2 = temp1 + sum;
        temp3 = temp2 * 0.5f;
        temp4 = temp3 * temp2;
        temp5 = temp4 + temp1 - sum;
        sum += temp5;
    }

    if (idx + blockDim.x < size) sum += input[idx + blockDim.x];

    sdata[tid] = sum;
    __syncthreads();

    for (int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (tid < s) {
            sdata[tid] += sdata[tid + s];
        }
        __syncthreads();
    }

    if (tid == 0) output[blockIdx.x] = sdata[0];
}

void experimentWithBlockSizes(float* d_input, float* d_output, int size) {
    int blockSizes[] = {1024};
    int numExperiments = sizeof(blockSizes) / sizeof(blockSizes[0]);

    for (int i = 0; i < numExperiments; i++) {
        int numThreads = blockSizes[i];
        int numBlocks = size / (2 * numThreads);

        printf("\nRunning with %d threads per block:\n", numThreads);
        
        reduceKernel<<<numBlocks, numThreads, numThreads * sizeof(float)>>>(d_input, d_output, size);
        cudaDeviceSynchronize();
    }
}

int main() {
    float *h_input, *h_output;
    float *d_input, *d_output;

    h_input = (float*) malloc(N * sizeof(float));
    h_output = (float*) malloc((N / 64) * sizeof(float));  // Maximum possible output size

    cudaMalloc(&d_input, N * sizeof(float));
    cudaMalloc(&d_output, (N / 64) * sizeof(float));

    for (int i = 0; i < N; i++) h_input[i] = 1.0f;

    cudaMemcpy(d_input, h_input, N * sizeof(float), cudaMemcpyHostToDevice);

    // Run experiments with different block sizes
    experimentWithBlockSizes(d_input, d_output, N);

    // Copy back results for final sum
    int numBlocks = N / (2 * 512);  // Default to 512 threads per block for final sum
    cudaMemcpy(h_output, d_output, numBlocks * sizeof(float), cudaMemcpyDeviceToHost);

    float sum = 0.0f;
    for (int i = 0; i < numBlocks; i++) {
        sum += h_output[i];
    }

    printf("\nFinal Sum (with 512 threads/block): %f\n", sum);

    free(h_input);
    free(h_output);
    cudaFree(d_input);
    cudaFree(d_output);

    return 0;
}

