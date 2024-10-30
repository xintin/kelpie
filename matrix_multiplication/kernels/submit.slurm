#!/usr/bin/env bash

#SBATCH --job-name=kelpie_clang_depthwise
#SBATCH --output=/gpfs/projects/ChapmanGroup/gverma/eval_tiling_tools/matrix_multiplication/kernels/logs/xm/depthwise_coord_decent_clang2.log
#SBATCH --ntasks=1  
#SBATCH --ntasks-per-node=1
#SBATCH -N 1
#SBATCH --cpus-per-task=1
#SBATCH --time=2-00:00:00
#SBATCH -p hbm-1tb-long-96core ##hbm-short-96core 

module load gcc/13.2.0 

export PATH=/gpfs/projects/ChapmanGroup/gverma/llvm-project/build:$PATH
export PATH=/gpfs/projects/ChapmanGroup/gverma/llvm-project/build/bin:$PATH
export LD_LIBRARY_PATH=/gpfs/projects/ChapmanGroup/gverma/llvm-project/build/lib:$LD_LIBRARY_PATH


cd /gpfs/projects/ChapmanGroup/gverma/eval_tiling_tools/matrix_multiplication/kernels/apps/xm

echo $(date)


# clang++ -mtune=sapphirerapids -fno-inline -O3 -o cholesky_clang cholesky.cpp
# ./cholesky_clang

# clang++ -mtune=sapphirerapids -fno-inline -O3 -o cholesky_dyn_coord_clang cholesky_dynamic_coord.cpp
# ./cholesky_dyn_coord_clang 1 1

./depthwise_coord_decent_clang 1

echo $(date)

echo -ne "completed"
