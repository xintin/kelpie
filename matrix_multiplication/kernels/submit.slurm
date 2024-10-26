#!/usr/bin/env bash

#SBATCH --job-name=kelpie_clang_self_cholesky
#SBATCH --output=/gpfs/projects/ChapmanGroup/gverma/eval_tiling_tools/matrix_multiplication/kernels/logs/xm/self_cholesky_clang.log
#SBATCH --ntasks-per-node=96
#SBATCH -N 1
#SBATCH --time=1:00:00
#SBATCH -p hbm-1tb-long-96core ##hbm-short-96core 

module load gcc/13.2.0 

export PATH=/gpfs/projects/ChapmanGroup/gverma/llvm-project/build:$PATH
export PATH=/gpfs/projects/ChapmanGroup/gverma/llvm-project/build/bin:$PATH
export LD_LIBRARY_PATH=/gpfs/projects/ChapmanGroup/gverma/llvm-project/build/lib:$LD_LIBRARY_PATH


cd /gpfs/projects/ChapmanGroup/gverma/eval_tiling_tools/matrix_multiplication/kernels/cholesky

echo $(date)


clang++ -mtune=sapphirerapids -fno-inline -O3 -o cholesky_clang cholesky.cpp
./cholesky_clang

clang++ -mtune=sapphirerapids -fno-inline -O3 -o cholesky_dyn_coord_clang cholesky_dynamic_coord.cpp
./cholesky_dyn_coord_clang 1 1


echo $(date)

echo -ne "completed"
