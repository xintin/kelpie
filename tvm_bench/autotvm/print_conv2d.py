import tvm
import numpy as np
from tvm import autotvm, te

M = N = 1000  # Dimensions of the input
F = 3         # Filter size

# Define placeholders and computation with reduction axes
A = te.placeholder((M, N), name="A")
B = te.placeholder((F, F), name="B")
r1 = te.reduce_axis((0, F), name="r1")
r2 = te.reduce_axis((0, F), name="r2")
C = te.compute(
    (M - F + 1, N - F + 1),
    lambda i, j: te.sum(A[i + r1, j + r2] * B[r1, r2], axis=[r1, r2]),
    name="C"
)

# Create schedule
s = te.create_schedule(C.op)

# Lower the schedule and print IR
lowered_func = tvm.lower(s, [A, B, C], name="conv2d_tiled")
print("Lowered IR:")
print(lowered_func)

# Build the function for a target (e.g., LLVM for CPU)
target = "llvm"  # or "cuda" for GPU
func = tvm.build(s, [A, B, C], target=target, name="conv2d_tiled")

# Print the generated code for the specified target
print("\nGenerated code:")
print(func.get_source())
