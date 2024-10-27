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
alpha = 1.0000001
beta = 0.4
N, L, M = 1000, 1000, 1000
dtype = "float32"


def gemm_bias_relu(A: te.Tensor, B: te.Tensor, bias: te.tensor) -> te.Tensor:
    C = topi.matmul(A, B) * beta
    D = topi.add(C, bias) * alpha
    E = topi.nn.relu(D)
    return E

## ----------------- Benchmark -------------------
def mm_print(N, L, M, dtype="float32"):
    A = te.placeholder((N, L), name="A", dtype=dtype)
    B = te.placeholder((L, M), name="B", dtype=dtype)
    bias = te.placeholder((N, M), name="bias", dtype=dtype)
    C = gemm_bias_relu(A, B, bias)
    te.create_prim_func([A, B, bias, C]).show()

@tvm.script.ir_module
class Main:
    @T.prim_func
    def main(A: T.Buffer((1000, 1000), "float32"), B: T.Buffer((1000, 1000), "float32"), bias: T.Buffer((1000, 1000), "float32"), compute: T.Buffer((1000, 1000), "float32")):
        T.func_attr({"tir.noalias": T.bool(True)})
        # with T.block("root"):
        T_matmul = T.alloc_buffer((1000, 1000))
        T_multiply = T.alloc_buffer((1000, 1000))
        T_add = T.alloc_buffer((1000, 1000))
        T_multiply_1 = T.alloc_buffer((1000, 1000))
        for ax0, ax1, k in T.grid(1000, 1000, 1000):
            with T.block("T_matmul"):
                v_ax0, v_ax1, v_k = T.axis.remap("SSR", [ax0, ax1, k])
                T.reads(A[v_ax0, v_k], B[v_k, v_ax1])
                T.writes(T_matmul[v_ax0, v_ax1])
                with T.init():
                    T_matmul[v_ax0, v_ax1] = T.float32(0)
                T_matmul[v_ax0, v_ax1] = T_matmul[v_ax0, v_ax1] + A[v_ax0, v_k] * B[v_k, v_ax1]
        for ax0, ax1 in T.grid(1000, 1000):
            with T.block("T_multiply"):
                v_ax0, v_ax1 = T.axis.remap("SS", [ax0, ax1])
                T.reads(T_matmul[v_ax0, v_ax1])
                T.writes(T_multiply[v_ax0, v_ax1])
                T_multiply[v_ax0, v_ax1] = T_matmul[v_ax0, v_ax1] * T.float32(0.40000000000000002)
        for ax0, ax1 in T.grid(1000, 1000):
            with T.block("T_add"):
                v_ax0, v_ax1 = T.axis.remap("SS", [ax0, ax1])
                T.reads(T_multiply[v_ax0, v_ax1], bias[v_ax0, v_ax1])
                T.writes(T_add[v_ax0, v_ax1])
                T_add[v_ax0, v_ax1] = T_multiply[v_ax0, v_ax1] + bias[v_ax0, v_ax1]
        for ax0, ax1 in T.grid(1000, 1000):
            with T.block("T_multiply_1"):
                v_ax0, v_ax1 = T.axis.remap("SS", [ax0, ax1])
                T.reads(T_add[v_ax0, v_ax1])
                T.writes(T_multiply_1[v_ax0, v_ax1])
                T_multiply_1[v_ax0, v_ax1] = T_add[v_ax0, v_ax1] * T.float32(1.0000001000000001)
        for i0, i1 in T.grid(1000, 1000):
            with T.block("compute"):
                v_i0, v_i1 = T.axis.remap("SS", [i0, i1])
                T.reads(T_multiply_1[v_i0, v_i1])
                T.writes(compute[v_i0, v_i1])
                compute[v_i0, v_i1] = T.max(T_multiply_1[v_i0, v_i1], T.float32(0))
## ---------------------------------------------


def ms_execute(logfile, target, target_name, trials):
    # only print
    # mm_print(N, L, M, dtype)
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

    print(f"Best time (s): {np.mean(best_time):.10f}")
    print(f"Best std  (s): {np.std(best_time):.10f}")
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
