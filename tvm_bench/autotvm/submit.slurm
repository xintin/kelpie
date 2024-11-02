#!/usr/bin/env bash

#SBATCH --job-name=autotvm_conv2d_updated
#SBATCH --output=/gpfs/projects/ChapmanGroup/gverma/eval_tiling_tools/tvm_bench/autotvm/logs/conv2d_updated.log
#SBATCH --ntasks=1  
#SBATCH --ntasks-per-node=1
#SBATCH -N 1
#SBATCH --cpus-per-task=1
#SBATCH --time=4:00:00
#SBATCH -p hbm-short-96core ##hbm-1tb-long-96core ##hbm-large-96core ##hbm-short-96core 

module load anaconda/3 gcc/13.2.0 

export PATH=/gpfs/projects/ChapmanGroup/gverma/llvm-project/build:$PATH
export PATH=/gpfs/projects/ChapmanGroup/gverma/llvm-project/build/bin:$PATH
export LD_LIBRARY_PATH=/gpfs/projects/ChapmanGroup/gverma/llvm-project/build/lib:$LD_LIBRARY_PATH
export PATH=/gpfs/projects/ChapmanGroup/gverma/tvm/build:/gpfs/projects/ChapmanGroup/gverma/tvm/build/bin:$PATH
export LD_LIBRARY_PATH=/gpfs/projects/ChapmanGroup/gverma/tvm/build/lib:$LD_LIBRARY_PATH
export PATH=/gpfs/projects/ChapmanGroup/gverma/tvm:$PATH
export TVM_HOME=/gpfs/projects/ChapmanGroup/gverma/tvm
export PYTHONPATH=$TVM_HOME/python:$PYTHONPATH

cd /gpfs/projects/ChapmanGroup/gverma/eval_tiling_tools/tvm_bench/autotvm

echo $(date)

python conv2d_updated.py -a x86 -l 'results/conv2d_updated.json' -t 100

echo $(date)

echo -ne "completed"
