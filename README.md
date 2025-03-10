# Kelpie
to update the git repo fetch/push url:     
`git remote -v`  
`git remote set-url origin https://github.com/xintin/kelpie.git`  
confirm: `git remote -v`  

## Introduction:  
This project enhances polyhedral optimizations by integrating a dynamic post-optimization tuner into `Pluto`, addressing its static nature. By combining polyhedral-based static analysis with coordinate descent, it fine-tunes kernel parameters such as tiling sizes. This approach improves performance over both fully static optimizers (`Clang -O3`, `IOOpt`) and profile-guided techniques (`Apache TVM`), making polyhedral optimization more adaptable to modern architectures.

## Dependencies:  
| **Tool**       | **Version**      | **Installation Link**      |
|----------------|------------------|------------------|
| Pluto          | v0.11.4          | [Link](https://pluto-compiler.sourceforge.net)           |
| gcc            | v13.2.0          | [Link](https://gcc.gnu.org/install/)           |
| clang          | v16.0.0          | [Link](https://clang.llvm.org/get_started.html)           |
| Polly          | 20.0.0git        | [Link](https://polly.llvm.org/get_started.html)           |
| TVM            | v0.18.0          | [Link](https://tvm.apache.org/docs/install/index.html)           |
| IOOpt          | -                | [Link](https://iocomplexity.corse.inria.fr)           |



### To evaluate IOOPT:
`cd matrix_multiplication`   
`make clean`   
`make CXX=[clang++|g++] COMPILER=[clang|gcc]`   
`./build_{clang|gcc}/matrix_exec 1000 800 700`   

### To evaluate Pluto:   
`cd matrix_multiplication/src`     
`clang++ -std=c++17 -Wall -O3 simple_comparison.cpp -o sc_clang`  
`./sc_clang`     
or run,   
`g++ -std=c++17 -Wall -O3 simple_comparison.cpp -o sc_gcc`  
`./sc_gcc`     

### To generate the vectorization report:  
`clang++ -O3 -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -Rpass-analysis=loop-vectorize simple_comparison.cpp -o sc_clang 2> vectorization_report_llvm.txt`     

`g++ -O3 -fopt-info-vec -fopt-info-vec-missed simple_comparison.cpp -o sc_gcc 2> vectorization_report_gcc.txt`  
  
### To generate assembly:  
`clang++ -O3 -fverbose-asm -g -S simple_comparison.cpp -o assembly_llvm.s`    

`g++ -O3 -fverbose-asm -g -S simple_comparison.cpp -o assembly_gcc.s`  


### To evaluate GPU kernels:   
```
nvcc -arch=sm_80 -Xptxas=-v <kernel>.cu -o <kernel>
```  
