import os, sys, time, argparse, tvm
from tvm import te, autotvm, topi

num_threads = os.cpu_count()
os.environ["TVM_NUM_THREADS"] = str(num_threads)
os.environ["MKL_NUM_THREADS"] = str(num_threads * 2 // 3)
os.environ["NUMEXPR_NUM_THREADS"] = str(num_threads * 2 // 3)
os.environ["OMP_NUM_THREADS"] = str(num_threads * 2 // 3)

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.dirname(SCRIPT_DIR))

from utils import *

## ------------------ Global ---------------------
N = 1000
dtype = "float32"
search_space = [1] + [i for i in range(2,65,2)]

def cholesky(N, dtype) -> te.Tensor:
    A = te.placeholder((N, N), name="A", dtype=dtype)
    k1 = te.reduce_axis((0, N), name="k1")
    k2 = te.reduce_axis((0, N), name="k2")
    k3 = te.reduce_axis((0, N), name="k3")
    
    B = te.compute([N,N], lambda i, j: te.if_then_else(
                    j < i,
                    A[i, j] * A[j, i], 
                    0
                ), name="B"
    )
    fdot = te.compute([N,], lambda i: te.sum(B[i][k1], axis=k1), name="fdot")

    A_diag = te.compute(
        (N,N),
        lambda i, j: te.if_then_else(
            i != j,
            A[i, i] - fdot[i],
            A[i, j]
        ),
        name="A_diag"
    )

    A_temp = te.compute((N,N), lambda i, j: te.sum(A[i, k2] * A[k2, j], axis=k2), name="A_temp")
    fdot3 = te.compute([N,], lambda i: te.sum(A_temp[i, k3], axis=k3), name="fdot3")

    A_offdiag = te.compute(
        (N, N),
        lambda i, j: te.if_then_else(
            i < j,
            A[i, j] - A_temp[i, j],
            A[i, j]
        ),
        name="A_offdiag"
    )

    res = te.compute(
        (N, N),
        lambda i, j: te.if_then_else(
            i < j,
            (A_offdiag[j, i] - fdot3[i]) / (A_diag[i][i]),
            0.0
        ),
        name="res"
    )
    return A, res

@autotvm.template("cholesky")
def cholesky_autotvm(N, dtype="float32"):
    A, res = cholesky(N, dtype)

    s = te.create_schedule(res.op)

    #print(s[B].op.axis)
    # schedule
    y, x  = s[res].op.axis

    # get the config object
    cfg = autotvm.get_config()

    # define search space
    cfg.define_knob("tile_x", search_space)
    cfg.define_knob("tile_y", search_space)

    # schedule according to config
    x0, x1 = s[res].split(x, cfg["tile_x"].val)
    y0, y1 = s[res].split(y, cfg["tile_y"].val)

    s[res].reorder(y0, x0, y1, x1)

    #te.create_prim_func([A, res]).show()

    return s, [A, res]


## ---------------------------------------------


def autotvm_template(log_file, target, trials):
    task = autotvm.task.create("cholesky",args=(N, "float32"), target=target)
    #print(task.config_space)
    tuner = autotvm.tuner.XGBTuner(task, loss_type="rank-binary")

    start = time.time()
    tuner.tune(
        n_trial=min(trials, len(task.config_space)),
        measure_option=autotvm.measure_option(
            builder="local", 
            runner=autotvm.LocalRunner(
                number=2, 
                repeat=5, 
                timeout=100, 
                enable_cpu_cache_flush=True if target == "llvm" else False,
            )
        ),
        callbacks=[autotvm.callback.log_to_file(log_file)],
    )
    end = time.time()

    best_time, _ = get_best_time(log_file)

    print(f"Best time (s): {np.mean(best_time):.10f}")
    print(f"Best std  (s): {np.std(best_time):.10f}")
    print(f"Tuning Time (min): {(end-start)/60:.2f}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        "python gemm_layernorm.py -a x86 -l 'results/cpu_matmul.json' -t 1000"
    )
    parser.add_argument(
        "-a", "--arch", type=str, required=True, help="Options: x86, aarch64, cuda"
    )
    parser.add_argument("-l", "--logfile", type=str, required=True)
    parser.add_argument("-t", "--trials", type=int, default=100)
    args = parser.parse_args()

    arch = args.arch
    logfile = args.logfile
    trials = args.trials

    # clean the files
    if os.path.isfile(logfile):
        os.remove(logfile)

    if arch == "x86":
        target = tvm.target.Target("llvm")
        dev = tvm.cpu()
    elif arch == "cuda":
        target = tvm.target.Target("cuda")
        dev = tvm.cuda()
    elif arch == "aarch64":
        target = tvm.target.Target("llvm -mcpu=a64fx")
        dev = tvm.cpu()
    else:
        print("Archtecture doesn't support.")
        exit(0)

    autotvm_template(logfile, target, trials)
