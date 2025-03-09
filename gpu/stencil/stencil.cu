#include <stdio.h>
#include <cuda.h>

#define N (1 << 28)  // 16 million elements
#define MAX_THREADS_PER_BLOCK 1024

__global__ void stencilKernel(float *input, float *output, int size) {
   extern __shared__ float sdata[];
   int tid = threadIdx.x;
   int idx = blockIdx.x * blockDim.x + threadIdx.x;

   // Load data into shared memory
   if (idx < size) sdata[tid] = input[idx];
   __syncthreads();

   // Apply 5-point stencil if within bounds
   if (tid > 1 && tid < blockDim.x - 2 && idx < size - 2) {
       output[idx] = 0.2f * (sdata[tid - 2] + sdata[tid - 1] +
sdata[tid] + sdata[tid + 1] + sdata[tid + 2]);
   }

   // Handle boundary conditions
   if (idx < 2 || idx >= size - 2) output[idx] = input[idx];
}

int main() {
   float *h_input, *h_output;
   float *d_input, *d_output;

   h_input = (float*) malloc(N * sizeof(float));
   h_output = (float*) malloc(N * sizeof(float));

   cudaMalloc(&d_input, N * sizeof(float));
   cudaMalloc(&d_output, N * sizeof(float));

   for (int i = 0; i < N; i++) h_input[i] = 1.0f;

   cudaMemcpy(d_input, h_input, N * sizeof(float), cudaMemcpyHostToDevice);

   // int configs[][2] = {{64, 64}, {128, 128}, {256, 256}, {512, 512},
{1024, 256}};

   std::vector<int> blockSizes = {1, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120, 128, 256, 512, 1024};

   for (int i = 0; i < 5; i++) {
       int numThreads = configs[i][0];
       int numBlocks = (N + numThreads - 1) / numThreads;

       cudaEvent_t start, stop;
       cudaEventCreate(&start);
       cudaEventCreate(&stop);

       cudaEventRecord(start);
       stencilKernel<<<numBlocks, numThreads, numThreads *
sizeof(float)>>>(d_input, d_output, N);
       cudaEventRecord(stop);
       cudaEventSynchronize(stop);

       float milliseconds = 0;
       cudaEventElapsedTime(&milliseconds, start, stop);
       printf("Config: %d blocks, %d threads per block -> Time: %.3fms\n", numBlocks, numThreads, milliseconds);

       cudaEventDestroy(start);
       cudaEventDestroy(stop);
   }

   cudaMemcpy(h_output, d_output, N * sizeof(float), cudaMemcpyDeviceToHost);
   printf("Sample output: %f %f %f %f %f\n", h_output[0], h_output[1], h_output[2], h_output[3], h_output[4]);

   free(h_input);
   free(h_output);
   cudaFree(d_input);
   cudaFree(d_output);

   return 0;
}
