import time
import tvm
import numpy as np
from tvm import autotvm, te

M = N = 1000
F = 3

# Load the best config
with autotvm.apply_history_best("conv2d_tiled.log"):
    # Recreate the convolution function with the best config applied
    A = te.placeholder((M, N), name="A")
    B = te.placeholder((F, F), name="B")
    r1 = te.reduce_axis((0, F), name="r1")
    r2 = te.reduce_axis((0, F), name="r2")
    C = te.compute(
        (M - F + 1, N - F + 1),
        lambda i, j: te.sum(A[i + r1, j + r2] * B[r1, r2], axis=[r1, r2]),
        name="C"
    )

    s = te.create_schedule(C.op)
    # Compile the function with the best config applied
    target = tvm.target.Target("llvm")
    func = tvm.build(s, [A, B, C], target=target, name="conv2d_tiled_best")

    # Create random input data
    a_np = np.random.uniform(-1, 1, size=(M, N)).astype("float32")
    b_np = np.random.uniform(-1, 1, size=(F, F)).astype("float32")
    c_np = np.zeros((M - F + 1, N - F + 1), dtype="float32")

    # Transfer data to the TVM runtime and measure execution time
    dev = tvm.cpu()
    a_tvm = tvm.nd.array(a_np, dev)
    b_tvm = tvm.nd.array(b_np, dev)
    c_tvm = tvm.nd.array(c_np, dev)

    # Measure execution time
    evaluator = func.time_evaluator(func.entry_name, dev, number=10)
    execution_time = evaluator(a_tvm, b_tvm, c_tvm).mean

    print(f"Best execution time: {execution_time * 1e3:.6f} ms")
