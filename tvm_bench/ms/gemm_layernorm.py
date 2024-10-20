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
N, L, M = 1000, 1000, 1000
alpha = 1.00000000001 
beta = 0.4
epsilon = 0.00001
dtype = "float32"

def gemm_layernorm(A: te.Tensor, B: te.Tensor) -> te.Tensor:
    k = te.reduce_axis((0, L), name="k")
    C = te.compute((N, M), lambda i, j: te.sum(alpha * A[i, k] * B[k, j], axis=k), name="C")
    D = te.compute((N, M), lambda i, j: te.add(C[i, j], C[i, j] * beta))

    sum_mean = te.compute((N,), lambda i: te.sum(D[i, k], axis=k), name="sum_mean")
    mean = te.compute((N,), lambda i: te.div(sum_mean[i], L), name="mean")
    var = te.compute((N,), lambda i: te.sum((D[i, k] - mean[i]) * (D[i, k] - mean[i]), axis=k), name='variance')
    norm = te.compute((N, M), lambda i, j: (D[i, j] - mean[i]) / te.sqrt(var[i] + epsilon), name='normalized')

    return norm

## ----------------- Benchmark -------------------
def mm_print(N, L, M, dtype="float32"):
    A = te.placeholder((N, L), name="A", dtype=dtype)
    B = te.placeholder((L, M), name="B", dtype=dtype)
    C = gemm_layernorm(A, B)
    te.create_prim_func([A, B, C]).show()

@tvm.script.ir_module
class Main:
    @T.prim_func
    def main(A: T.Buffer((1000, 1000), "float32"), B: T.Buffer((1000, 1000), "float32"), normalized: T.Buffer((1000, 1000), "float32")):
        T.func_attr({"tir.noalias": T.bool(True)})
        # with T.block("root"):
        C = T.alloc_buffer((1000, 1000))
        compute = T.alloc_buffer((1000, 1000))
        sum_mean = T.alloc_buffer((1000,))
        mean = T.alloc_buffer((1000,))
        variance = T.alloc_buffer((1000,))
        for i, j, k in T.grid(1000, 1000, 1000):
            with T.block("C"):
                v_i, v_j, v_k = T.axis.remap("SSR", [i, j, k])
                T.reads(A[v_i, v_k], B[v_k, v_j])
                T.writes(C[v_i, v_j])
                with T.init():
                    C[v_i, v_j] = T.float32(0)
                C[v_i, v_j] = C[v_i, v_j] + T.float32(1.00000000001) * A[v_i, v_k] * B[v_k, v_j]
        for i, j in T.grid(1000, 1000):
            with T.block("compute"):
                v_i, v_j = T.axis.remap("SS", [i, j])
                T.reads(C[v_i, v_j])
                T.writes(compute[v_i, v_j])
                compute[v_i, v_j] = C[v_i, v_j] + C[v_i, v_j] * T.float32(0.40000000000000002)
        for i, k in T.grid(1000, 1000):
            with T.block("sum_mean"):
                v_i, v_k = T.axis.remap("SR", [i, k])
                T.reads(compute[v_i, v_k])
                T.writes(sum_mean[v_i])
                with T.init():
                    sum_mean[v_i] = T.float32(0)
                sum_mean[v_i] = sum_mean[v_i] + compute[v_i, v_k]
        for i in range(1000):
            with T.block("mean"):
                v_i = T.axis.spatial(1000, i)
                T.reads(sum_mean[v_i])
                T.writes(mean[v_i])
                mean[v_i] = sum_mean[v_i] * T.float32(0.001)
        for i, k in T.grid(1000, 1000):
            with T.block("variance"):
                v_i, v_k = T.axis.remap("SR", [i, k])
                T.reads(compute[v_i, v_k], mean[v_i])
                T.writes(variance[v_i])
                with T.init():
                    variance[v_i] = T.float32(0)
                variance[v_i] = variance[v_i] + (compute[v_i, v_k] - mean[v_i]) * (compute[v_i, v_k] - mean[v_i])
        for i, j in T.grid(1000, 1000):
            with T.block("normalized"):
                v_i, v_j = T.axis.remap("SS", [i, j])
                T.reads(compute[v_i, v_j], mean[v_i], variance[v_i])
                T.writes(normalized[v_i, v_j])
                normalized[v_i, v_j] = (compute[v_i, v_j] - mean[v_i]) / T.sqrt(variance[v_i] + T.float32(1.0000000000000001e-05))


## ---------------------------------------------


def ms_execute(logfile, target, target_name, trials):
    # only print
    #mm_print(N, L, M, dtype)
    #return

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
