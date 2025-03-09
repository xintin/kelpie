#include <stdio.h>
#include <cuda.h>

#define N (1 << 28)  // Large array (16 million elements)
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

   // Allocate memory
   h_input = (float*) malloc(N * sizeof(float));
   h_output = (float*) malloc((N / 2) * sizeof(float));  // Maximum needed size

   cudaMalloc(&d_input, N * sizeof(float));
   cudaMalloc(&d_output, (N / 2) * sizeof(float));

   for (int i = 0; i < N; i++) h_input[i] = 1.0f;

   cudaMemcpy(d_input, h_input, N * sizeof(float), cudaMemcpyHostToDevice);

   printf("Threads per Block | Blocks  | Time (ms)\n");
   printf("--------------------------------------\n");

   // Loop over different thread configurations
   for (int numThreads = 8; numThreads <= MAX_THREADS_PER_BLOCK;
numThreads += 2) {
       int numBlocks = N / (2 * numThreads);

       // Timing events
       cudaEvent_t start, stop;
       cudaEventCreate(&start);
       cudaEventCreate(&stop);

       cudaEventRecord(start);
       reduceKernel<<<numBlocks, numThreads, numThreads *
sizeof(float)>>>(d_input, d_output, N);
       cudaEventRecord(stop);

       cudaEventSynchronize(stop);

       float milliseconds = 0;
       cudaEventElapsedTime(&milliseconds, start, stop);

       printf("%16d | %6d | %8.8f ms\n", numThreads, numBlocks, milliseconds);

       cudaEventDestroy(start);
       cudaEventDestroy(stop);
   }

   // Free memory
   free(h_input);
   free(h_output);
   cudaFree(d_input);
   cudaFree(d_output);

   return 0;
}
