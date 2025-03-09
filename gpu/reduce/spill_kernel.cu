#include <stdio.h>
#include <cuda.h>

#define N (1 << 24)  // Large array (16 million elements)
#define MAX_THREADS_PER_BLOCK 1024  // Hardware limit

struct LargeData {
    float data[1024];  // Large data structure to force register usage
};

__global__ void spillKernel(float *input, float *output, int size) {
    extern __shared__ float sdata[];

    int tid = threadIdx.x;
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    LargeData vars;

    for (int i = 0; i < 1024; ++i) {
        vars.data[i] = input[idx] * (i + 1.0f);
    }

    float sum = 0.0f;
    for (int i = 0; i < 1024; ++i) {
        sum += vars.data[i] * 1.1f;
    }

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

void runSpillKernel(float* d_input, float* d_output, int size) {
    int numThreads = 1024;
    int numBlocks = size / numThreads;

    printf("\nRunning kernel designed to cause register spilling:\n");
    
    spillKernel<<<numBlocks, numThreads, numThreads * sizeof(float)>>>(d_input, d_output, size);
    cudaDeviceSynchronize();
}

int main() {
    float *h_input, *h_output;
    float *d_input, *d_output;

    h_input = (float*) malloc(N * sizeof(float));
    h_output = (float*) malloc((N / MAX_THREADS_PER_BLOCK) * sizeof(float));

    cudaMalloc(&d_input, N * sizeof(float));
    cudaMalloc(&d_output, (N / MAX_THREADS_PER_BLOCK) * sizeof(float));

    for (int i = 0; i < N; i++) h_input[i] = 1.0f;

    cudaMemcpy(d_input, h_input, N * sizeof(float), cudaMemcpyHostToDevice);

    // Run the kernel that forces register spilling
    runSpillKernel(d_input, d_output, N);

    // Copy back results for verification
    cudaMemcpy(h_output, d_output, (N / MAX_THREADS_PER_BLOCK) * sizeof(float), cudaMemcpyDeviceToHost);

    float sum = 0.0f;
    for (int i = 0; i < (N / MAX_THREADS_PER_BLOCK); i++) {
        sum += h_output[i];
    }

    printf("\nFinal Sum: %f\n", sum);

    free(h_input);
    free(h_output);
    cudaFree(d_input);
    cudaFree(d_output);

    return 0;
}
