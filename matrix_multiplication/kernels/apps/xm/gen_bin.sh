#!/bin/sh


# cd ../../depthwise

# /gpfs/projects/ChapmanGroup/schheda/llvm-host/bin/clang++ -std=c++17 -Wall -march=native -mavx -O3 -fno-inline-functions -mtune=native -mavx2 -mpopcnt -mbmi2 depthwise_compare.cpp -o ../apps/xm/depthwise_compare_clang
# g++ -std=c++17 -Wall -march=native -mavx -O3 -fno-inline-functions -mtune=native -mavx2 -mpopcnt -mbmi2 depthwise_compare.cpp -o ../apps/xm/depthwise_compare_gcc
# /gpfs/projects/ChapmanGroup/schheda/llvm-host/bin/clang++ -std=c++17 -Wall -march=native -mavx -O3 -fno-inline-functions -mtune=native -mavx2 -mpopcnt -mbmi2 depthwise_online_coord_decent.cpp -o ../apps/xm/depthwise_coord_decent_clang
# g++ -std=c++17 -Wall -march=native -mavx -O3 -fno-inline-functions -mtune=native -mavx2 -mpopcnt -mbmi2 depthwise_online_coord_decent.cpp -o ../apps/xm/depthwise_coord_decent_gcc

# cd ../apps/xm

# ./depthwise_compare_gcc > ../../logs/xm/depthwise_compare_gcc.log
# ./depthwise_compare_clang > ../../logs/xm/depthwise_compare_clang.log
# ./depthwise_coord_decent_gcc 1 > ../../logs/xm/depthwise_coord_decent_gcc.log
# ./depthwise_coord_decent_clang 1 > ../../logs/xm/depthwise_coord_decent_clang.log

echo -ne "gemm_bias_relu\n"

cd ../../gemm_bias_relu

/gpfs/projects/ChapmanGroup/schheda/llvm-host/bin/clang++ -std=c++17 -Wall -march=native -mavx -O3 -fno-inline-functions -mtune=native -mavx2 -mpopcnt -mbmi2 gemm_bias_relu_compare.cpp -o ../apps/xm/gemm_bias_relu_compare_clang
g++ -std=c++17 -Wall -march=native -mavx -O3 -fno-inline-functions -mtune=native -mavx2 -mpopcnt -mbmi2 gemm_bias_relu_compare.cpp -o ../apps/xm/gemm_bias_relu_compare_gcc
/gpfs/projects/ChapmanGroup/schheda/llvm-host/bin/clang++ -std=c++17 -Wall -march=native -mavx -O3 -fno-inline-functions -mtune=native -mavx2 -mpopcnt -mbmi2 gemm_bias_relu_online_coord.cpp -o ../apps/xm/gemm_bias_relu_online_coord_clang
g++ -std=c++17 -Wall -march=native -mavx -O3 -fno-inline-functions -mtune=native -mavx2 -mpopcnt -mbmi2 gemm_bias_relu_online_coord.cpp -o ../apps/xm/gemm_bias_relu_online_coord_gcc

cd ../apps/xm

./gemm_bias_relu_compare_clang > ../../logs/xm/gemm_bias_relu_compare_clang.log
./gemm_bias_relu_compare_gcc > ../../logs/xm/gemm_bias_relu_compare_gcc.log
./gemm_bias_relu_online_coord_clang > ../../logs/xm/gemm_bias_relu_online_coord_clang.log
./gemm_bias_relu_online_coord_gcc > ../../logs/xm/gemm_bias_relu_online_coord_gcc.log


echo -ne "gemm_bilinear\n" 

cd ../../gemm_bilinear

/gpfs/projects/ChapmanGroup/schheda/llvm-host/bin/clang++ -std=c++17 -Wall -march=native -mavx -O3 -fno-inline-functions -mtune=native -mavx2 -mpopcnt -mbmi2 gemm_bilinear_compare.cpp -o ../apps/xm/gemm_bilinear_compare_clang
g++ -std=c++17 -Wall -march=native -mavx -O3 -fno-inline-functions -mtune=native -mavx2 -mpopcnt -mbmi2 gemm_bilinear_compare.cpp -o ../apps/xm/gemm_bilinear_compare_gcc
/gpfs/projects/ChapmanGroup/schheda/llvm-host/bin/clang++ -std=c++17 -Wall -march=native -mavx -O3 -fno-inline-functions -mtune=native -mavx2 -mpopcnt -mbmi2 gemm_bilinear_online_coord.cpp -o ../apps/xm/gemm_bilinear_online_coord_clang
g++ -std=c++17 -Wall -march=native -mavx -O3 -fno-inline-functions -mtune=native -mavx2 -mpopcnt -mbmi2 gemm_bilinear_online_coord.cpp -o ../apps/xm/gemm_bilinear_online_coord_gcc

cd ../apps/xm

./gemm_bilinear_compare_clang > ../../logs/xm/gemm_bilinear_compare_clang.log
./gemm_bilinear_compare_gcc > ../../logs/xm/gemm_bilinear_compare_gcc.log
./gemm_bilinear_online_coord_clang > ../../logs/xm/gemm_bilinear_online_coord_clang.log
./gemm_bilinear_online_coord_gcc > ../../logs/xm/gemm_bilinear_online_coord_gcc.log


echo -ne "gemm_layernorm\n" 

cd ../../gemm_layernorm

/gpfs/projects/ChapmanGroup/schheda/llvm-host/bin/clang++ -std=c++17 -Wall -march=native -mavx -O3 -fno-inline-functions -mtune=native -mavx2 -mpopcnt -mbmi2 gemm_layernorm_compare.cpp -o ../apps/xm/gemm_layernorm_compare_clang
g++ -std=c++17 -Wall -march=native -mavx -O3 -fno-inline-functions -mtune=native -mavx2 -mpopcnt -mbmi2 gemm_layernorm_compare.cpp -o ../apps/xm/gemm_layernorm_compare_gcc
/gpfs/projects/ChapmanGroup/schheda/llvm-host/bin/clang++ -std=c++17 -Wall -march=native -mavx -O3 -fno-inline-functions -mtune=native -mavx2 -mpopcnt -mbmi2 gemm_layernorm_online_coord.cpp -o ../apps/xm/gemm_layernorm_online_coord_clang
g++ -std=c++17 -Wall -march=native -mavx -O3 -fno-inline-functions -mtune=native -mavx2 -mpopcnt -mbmi2 gemm_layernorm_online_coord.cpp -o ../apps/xm/gemm_layernorm_online_coord_gcc

cd ../apps/xm

./gemm_layernorm_compare_clang > ../../logs/xm/gemm_layernorm_compare_clang.log
./gemm_layernorm_compare_gcc > ../../logs/xm/gemm_layernorm_compare_gcc.log
./gemm_layernorm_online_coord_clang > ../../logs/xm/gemm_layernorm_online_coord_clang.log
./gemm_layernorm_online_coord_gcc > ../../logs/xm/gemm_layernorm_online_coord_gcc.log
