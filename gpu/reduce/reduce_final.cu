#include <stdio.h>
#include <cuda.h>
#include <vector>

#define N (1 << 28)  // Large array (16 million elements)
#define MAX_THREADS_PER_BLOCK 1024  // Hardware limit

// Reduction Kernel
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

// Function to run kernel and measure time
float runKernel(float* d_input, float* d_output, dim3 blockDim) {
    int numBlocks = N / (2 * blockDim.x);

    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    cudaEventRecord(start);
    reduceKernel<<<numBlocks, blockDim>>>(d_input, d_output, N);
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

    // Initialize input data
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

    // Iterate through all combinations of bx and by
    for (int bx : blockSizes) {
        for (int by : blockSizes) {
            if (bx * by > 1024) continue;  // Ensure total threads per block does not exceed limit

            dim3 blockDim(bx, by);
            float totalTime = 0;

            // Run 3 times and average the results
            for (int i = 0; i < 3; i++) {
                totalTime += runKernel(d_input, d_output, blockDim);
            }

            float avgTime = totalTime / 3.0;
            printf("Block Dim (%d, %d) - Avg Time: %.2f ns\n", bx, by, avgTime);
        }
    }

    // Free memory
    cudaFree(d_input);
    cudaFree(d_output);
    free(h_input);
    free(h_output);

    return 0;
}
