#!/bin/bash

#SBATCH --job-name=rome_gcc_eval_ioopt
#SBATCH --output=./logs/rome_eval_ioopt_kernel1_gcc.log
#SBATCH --mail-type=ALL --mail-user=gaurav.verma@stonybrook.edu
#SBATCH -n 1 -c 1 -C rome,ib 

date
hostname
echo "Started..."

echo "Gathering CPU information..."
if lscpu; then
    lscpu >> ./logs/rome_cpu_info.txt 
    echo "Successfully displayed CPU information."
else
    echo "Failed to display CPU information." >&2
fi

# load modules
export NUMEXPR_MAX_THREADS=128
module load modules/2.1.1-20230405
module load  llvm/11.1.0   cuda/11.8.0   python/3.8.15  cmake/3.25.1   cudnn/8.4.0.27-11.6  gcc/12.2.0

#build_clang/matrix_exec 1000 700 800
build_gcc/matrix_exec 1000 700 800

date
echo -e "\nCompleted\n"

