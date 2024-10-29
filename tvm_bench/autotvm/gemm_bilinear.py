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
alpha = 1.0000001
beta = 0.4
N, L, M = 1000, 1000, 1000
dtype = "float32"
search_space = [1] + [i for i in range(2,129,2)]

## ----------------- Benchmark -------------------
def gemm_bilinear(N, L, M, dtype="float32") -> te.Tensor:
    A = te.placeholder((N, L), name="A", dtype=dtype)
    B = te.placeholder((L, M), name="B", dtype=dtype)
    k = te.reduce_axis((0, L), name="k")
    C = te.compute((N, M), lambda i, j: te.sum(alpha * A[i, k] * B[k, j], axis=k), name="C")
    D = te.compute((N,M), lambda i, j: te.add(C[i, j], C[i, j] * beta))
    return [A, B, C, D]

@autotvm.template("gemm_bilinear")
def gemm_bilinear_autotvm(N, L, M, dtype="float32"):
    A, B, C, D = gemm_bilinear(N, L, M)
    s = te.create_schedule(D.op)
    
    #print(s[D].op.axis)
    # schedule
    y, x  = s[D].op.axis

    # get the config object
    cfg = autotvm.get_config()

    # define search space
    cfg.define_knob("tile_x", search_space)
    cfg.define_knob("tile_y", search_space)

    # schedule according to config
    x0, x1 = s[D].split(x, cfg["tile_x"].val)
    y0, y1 = s[D].split(y, cfg["tile_y"].val)

    s[D].reorder(y0, x0, y1, x1)

    return s, [A, B, C, D]


def generate_autotvm_template(log_file, target, trials):
    task = autotvm.task.create("gemm_bilinear", args=(N, L, M, "float32"), target=target)
    #print(task.config_space)
    tuner = autotvm.tuner.XGBTuner(task, loss_type="rank")

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
        "python gemm.py -a x86 -l 'results/cpu_matmul.json' -t 1000"
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

    generate_autotvm_template(logfile, target, trials)
