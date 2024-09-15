import os, sys, time, argparse, tvm
from tvm import te, auto_scheduler, topi

num_threads = os.cpu_count()
os.environ["TVM_NUM_THREADS"] = str(num_threads)
os.environ["MKL_NUM_THREADS"] = str(num_threads * 2 // 3)
os.environ["NUMEXPR_NUM_THREADS"] = str(num_threads * 2 // 3)
os.environ["OMP_NUM_THREADS"] = str(num_threads * 2 // 3)

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.dirname(SCRIPT_DIR))

from utils import *

## ------------------ Global ---------------------
N, L, M = 1024, 1024, 1024
alpha = 1.00000000001 
beta = 0.4
dtype = "float32"

## ----------------- Benchmark -------------------
def gemm_bilinear(A: te.Tensor, B: te.Tensor) -> te.Tensor:
    k = te.reduce_axis((0, L), name="k")
    C = te.compute((N, M), lambda i, j: te.sum(alpha * A[i, k] * B[k, j], axis=k), name="C")
    D = te.compute((N,M), lambda i, j: te.add(C[i, j], C[i, j] * beta))
    return D

@auto_scheduler.register_workload
def gemm(N, L, M, dtype="float32"):
    A = te.placeholder((N, L), name="A", dtype=dtype)
    B = te.placeholder((L, M), name="B", dtype=dtype)
    C = gemm_bilinear(A, B)
    return [A, B, C]


## ---------------------------------------------


def generate_ansor_template(log_file, target, trials):
    task = tvm.auto_scheduler.SearchTask(
        func=gemm, args=(N, L, M, "float32"), target=target
    )

    ## Set Parameters for Auto-Scheduler
    trial = trials
    tune_option = auto_scheduler.TuningOptions(
        num_measure_trials=trial,  # change this to 20000 to achieve the best performance
        runner=auto_scheduler.LocalRunner(
            number=10,
            repeat=3,
            timeout=100,
            enable_cpu_cache_flush=True if target == "llvm" else False,
        ),
        measure_callbacks=[auto_scheduler.RecordToFile(log_file)],
        verbose=0,
    )

    start = time.time()
    # Run auto-tuning (search)
    task.tune(tune_option)
    end = time.time()

    best_time, _ = get_best_time(log_file)

    print(f"Best time (ms): {np.mean(best_time):.10f}")
    print(f"Best std  (ms): {np.std(best_time):.10f}")
    print(f"Tuning Time (min): {(end-start)/60:.2f}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        "python gemm_bias_relu.py -a x86 -l 'results/cpu_matmul.json' -t 1000"
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

    generate_ansor_template(logfile, target, trials)
