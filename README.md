# eval_tiling_tools
to update the git repo fetch/push url:     
`git remote -v`  
`git remote set-url origin https://github.com/xintin/eval_tiling_tools.git`  
confirm: `git remote -v`  

### Results:  
[Eval Results](https://docs.google.com/spreadsheets/d/1_VqXzBx2B6Bm_sRaVA9pj-VefE9d2efk0Jc_0lSkFy0/edit?usp=sharing)

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
  
