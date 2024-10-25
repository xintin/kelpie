import os, sys, time, argparse, tvm
from tvm import te, topi
from tvm import meta_schedule as ms
from tvm.meta_schedule.runner.config import EvaluatorConfig
from tvm.script import tir as T

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
alpha = 1.00000000001 
beta = 0.4
epsilon = 0.00001
dtype = "float32"

def cholesky(A: te.Tensor) -> te.Tensor:
    k = te.reduce_axis((0, N), name="k")
    fdot = te.compute([N,N], lambda i, k: te.if_then_else(
                    k < i,
                    A[i, k] * A[k, i], 
                    0
                ), name="fdot_cond"
    )

    A = te.compute([N, N], lambda i, j: te.if_then_else( 
        i == j, A[i][j] - fdot[i][j], A[i][j]
    ), name="A")

    fdot = te.compute([N,], lambda i: te.sum(A[i, k] * A[k, i], axis=k), name="fdot")

    res = te.compute(
        (N, N),
        lambda i, j: te.if_then_else(
            te.all(j > i),
            (A[i, j] - fdot[i]) / A[i, i],
            0.0
        ),
        name="res"
    )

    return res

## ----------------- Benchmark -------------------
def mm_print(N, dtype="float32"):
    A = te.placeholder((N, N), name="A", dtype=dtype)
    B = cholesky(A)
    te.create_prim_func([A, B]).show()

@tvm.script.ir_module
class Main:
    @T.prim_func
    def main(A: T.Buffer((1000, 1000), "float32"), res: T.Buffer((1000, 1000), "float32")):
        T.func_attr({"tir.noalias": T.bool(True)})
        # with T.block("root"):
        fdot_cond = T.alloc_buffer((1000, 1000))
        A_1 = T.alloc_buffer((1000, 1000))
        fdot = T.alloc_buffer((1000,))
        for i, k in T.grid(1000, 1000):
            with T.block("fdot_cond"):
                v_i, v_k = T.axis.remap("SS", [i, k])
                T.reads(A[T.min(v_i, v_k):T.min(v_i, v_k) + (T.max(v_i, v_k) + 1 - T.min(v_i, v_k)), T.min(v_k, v_i):T.min(v_k, v_i) + (T.max(v_k, v_i) + 1 - T.min(v_k, v_i))])
                T.writes(fdot_cond[v_i, v_k])
                fdot_cond[v_i, v_k] = T.if_then_else(v_k < v_i, A[v_i, v_k] * A[v_k, v_i], T.float32(0))
        for i, j in T.grid(1000, 1000):
            with T.block("A"):
                v_i, v_j = T.axis.remap("SS", [i, j])
                T.reads(A[v_i, v_j], fdot_cond[v_i, v_j])
                T.writes(A_1[v_i, v_j])
                A_1[v_i, v_j] = T.if_then_else(v_i == v_j, A[v_i, v_j] - fdot_cond[v_i, v_j], A[v_i, v_j])
        for i, k in T.grid(1000, 1000):
            with T.block("fdot"):
                v_i, v_k = T.axis.remap("SR", [i, k])
                T.reads(A_1[T.min(v_i, v_k):T.min(v_i, v_k) + (T.max(v_i, v_k) + 1 - T.min(v_i, v_k)), T.min(v_k, v_i):T.min(v_k, v_i) + (T.max(v_k, v_i) + 1 - T.min(v_k, v_i))])
                T.writes(fdot[v_i])
                with T.init():
                    fdot[v_i] = T.float32(0)
                fdot[v_i] = fdot[v_i] + A_1[v_i, v_k] * A_1[v_k, v_i]
        for i, j in T.grid(1000, 1000):
            with T.block("res"):
                v_i, v_j = T.axis.remap("SS", [i, j])
                T.reads(A_1[v_i, T.min(v_j, v_i):T.min(v_j, v_i) + (T.max(v_j, v_i) + 1 - T.min(v_j, v_i))], fdot[v_i])
                T.writes(res[v_i, v_j])
                res[v_i, v_j] = T.if_then_else(v_i < v_j, (A_1[v_i, v_j] - fdot[v_i]) / A_1[v_i, v_i], T.float32(0))


## ---------------------------------------------


def ms_execute(logfile, target, target_name, trials):
    # only print
    # mm_print(N, dtype)
    # return

    start = time.time()
    database = ms.tune_tir(
        mod=Main,
        target=target,
        max_trials_global=trials,
        num_trials_per_iter=64,
        work_dir=logfile,
        runner=ms.runner.LocalRunner(
            evaluator_config=EvaluatorConfig(
                number=10,
                repeat=3,
                min_repeat_ms=100,
                enable_cpu_cache_flush=True if target_name == "llvm" else False,
            )
        ),
        cost_model=ms.cost_model.XGBModel(
            extractor=ms.feature_extractor.PerStoreFeature(),
            adaptive_training=False,
        ),
        strategy=ms.search_strategy.EvolutionarySearch(),
    )
    end = time.time()

    best_time = get_ms_time(logfile + "/database_tuning_record.json")

    print(f"Best time (ms): {np.mean(best_time):.10f}")
    print(f"Best std  (ms): {np.std(best_time):.10f}")
    print(f"Tuning Time (min): {(end-start)/60:.2f}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser("python mm.py -a x86 -l 'results/ms/cpu_matmul'")
    parser.add_argument(
        "-a", "--arch", type=str, required=True, help="Options: x86, aarch64, cuda"
    )
    parser.add_argument("-l", "--logfile", type=str, required=True)
    parser.add_argument("-t", "--trials", type=int, default=1000)
    args = parser.parse_args()

    arch = args.arch
    logfile = args.logfile
    trials = args.trials

    # clean the files
    if os.path.isfile(logfile):
        os.remove(logfile)

    if arch == "x86":
        target_name = "llvm"
        target = tvm.target.Target(f"llvm -num-cores {num_threads // 2}")
        dev = tvm.cpu()
    elif arch == "cuda":
        target_name = "cuda"
        target = tvm.target.Target(
            "cuda -max_threads_per_block 1024 -max_shared_memory_per_block 49152"
        )
        dev = tvm.cuda()
    elif arch == "arm":
        target = tvm.target.Target("llvm -mcpu=a64fx -num-cores 48")
        dev = tvm.cpu()
    else:
        print("Archtecture doesn't support.")
        exit(0)

    ms_execute(logfile, target, target_name, trials)
