import tvm
from tvm import te, autotvm
import numpy as np

# Global shapes and constants
M, N, F = 1000, 1000, 3  # Example values for M, N, and F
tile_size_t2 = 4       # Placeholder for tile size, will be defined by AutoTVM
tile_size_t3 = 4       # Placeholder for tile size, will be defined by AutoTVM

@autotvm.template("conv2d_tiled")
def conv2d_tiled():
    # Define placeholders for input matrices A and B
    A = te.placeholder((M, N), name="A")
    B = te.placeholder((F, F), name="B")
    
    # Define reduction axes for convolution
    r1 = te.reduce_axis((0, F), name="r1")
    r2 = te.reduce_axis((0, F), name="r2")
    
    # Define the convolution operation
    C = te.compute(
        (M - F + 1, N - F + 1),
        lambda i, j: te.sum(A[i + r1, j + r2] * B[r1, r2], axis=[r1, r2]),
        name="C"
    )
    
    # Create a schedule
    s = te.create_schedule(C.op)
    
    # Tile the computation axes
    i, j = s[C].op.axis
    cfg = autotvm.get_config()

    # Define the search space for tile sizes
    cfg.define_knob("tile_size_t2", [i for i in range(1, 65, 2)])
    cfg.define_knob("tile_size_t3", [i for i in range(1, 65, 2)])

    # Apply tiling
    i_outer, i_inner = s[C].split(i, cfg["tile_size_t2"].val)
    j_outer, j_inner = s[C].split(j, cfg["tile_size_t3"].val)

    # Reorder to improve cache performance
    s[C].reorder(i_outer, j_outer, i_inner, j_inner, r1, r2)

    return s, [A, B, C]


# Tuning process
if __name__ == "__main__":
    target = "llvm"
    task = autotvm.task.create("conv2d_tiled", args=(), target=target)
    print(task.config_space)

    log_file = "conv2d_tiled.log"
    tuner = autotvm.tuner.XGBTuner(task)
    tuner.tune(
        n_trial=100,
        measure_option=autotvm.measure_option(
            builder="local", 
            runner=autotvm.LocalRunner(number=10, repeat=3, min_repeat_ms=100, enable_cpu_cache_flush=True)
        ),
        callbacks=[autotvm.callback.log_to_file(log_file)]
    )

    # Retrieve the best config
    dispatch_context = autotvm.apply_history_best(log_file)
    best_config = dispatch_context.query(task.target, task.workload)
    print("Best config:", best_config)

