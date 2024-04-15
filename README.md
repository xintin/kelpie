# eval_ioopt

### To evaluate IOOPT:
`cd matrix_multiplication`   
`make clean`   
`make CXX=[clang++|g++] COMPILER=[clang|gcc]`   
`./build_{clang|gcc}/matrix_exec 1000 800 700`   

### To evaluate Pluto:   
`cd matrix_multiplication/src`    
`clang++ -std=c++17 -Wall -o3 simple_comparison.cpp -o sc_clang`  
`./sc_clang`
or run,   
`g++ -std=c++17 -Wall -o3 simple_comparison.cpp -o sc_gcc`   
`./sc_gcc`  
