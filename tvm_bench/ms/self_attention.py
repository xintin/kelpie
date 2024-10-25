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
N, L, M = 4096, 64, 4096
dtype = "float32"

def self_attention(A: te.Tensor, B: te.Tensor, C:te.Tensor) -> te.Tensor:
    k = te.reduce_axis((0, L), name="k")
    D = te.compute((N, M), lambda i, j: te.sum(A[i, k] * B[k, j], axis=k), name="D")
    E = te.compute((N, M), lambda i, j: D[i, j] * (1.0 / np.sqrt(N)))
    F = topi.nn.softmax(E)
    G = topi.nn.matmul(F, C)
    return G

## ----------------- Benchmark -------------------
def mm_print(N, L, M, dtype="float32"):
    A = te.placeholder((N, L), name="A", dtype=dtype)
    B = te.placeholder((L, M), name="B", dtype=dtype)
    C = te.placeholder((N, L), name="C", dtype=dtype)
    D = self_attention(A, B, C)
    te.create_prim_func([A, B, C, D]).show()

@tvm.script.ir_module
class Main:
    @T.prim_func
    def main(A: T.Buffer((4096, 64), "float32"), B: T.Buffer((64, 4096), "float32"), B_1: T.Buffer((4096, 64), "float32"), T_matmul_NN: T.Buffer((4096, 64), "float32")):
        T.func_attr({"layout_free_buffers": [2], "tir.noalias": T.bool(True)})
        # with T.block("root"):
        D = T.alloc_buffer((4096, 4096))
        compute = T.alloc_buffer((4096, 4096))
        T_softmax_maxelem = T.alloc_buffer((4096,))
        T_softmax_exp = T.alloc_buffer((4096, 4096))
        T_softmax_expsum = T.alloc_buffer((4096,))
        T_softmax_norm = T.alloc_buffer((4096, 4096))
        for i, j, k in T.grid(4096, 4096, 64):
            with T.block("D"):
                v_i, v_j, v_k = T.axis.remap("SSR", [i, j, k])
                T.reads(A[v_i, v_k], B[v_k, v_j])
                T.writes(D[v_i, v_j])
                with T.init():
                    D[v_i, v_j] = T.float32(0)
                D[v_i, v_j] = D[v_i, v_j] + A[v_i, v_k] * B[v_k, v_j]
        for i, j in T.grid(4096, 4096):
            with T.block("compute"):
                v_i, v_j = T.axis.remap("SS", [i, j])
                T.reads(D[v_i, v_j])
                T.writes(compute[v_i, v_j])
                compute[v_i, v_j] = D[v_i, v_j] * T.float32(0.015625)
        for i0, k in T.grid(4096, 4096):
            with T.block("T_softmax_maxelem"):
                v_i0, v_k = T.axis.remap("SR", [i0, k])
                T.reads(compute[v_i0, v_k])
                T.writes(T_softmax_maxelem[v_i0])
                with T.init():
                    T_softmax_maxelem[v_i0] = T.float32(-3.4028234663852886e+38)
                T_softmax_maxelem[v_i0] = T.max(T_softmax_maxelem[v_i0], compute[v_i0, v_k])
        for i0, i1 in T.grid(4096, 4096):
            with T.block("T_softmax_exp"):
                v_i0, v_i1 = T.axis.remap("SS", [i0, i1])
                T.reads(compute[v_i0, v_i1], T_softmax_maxelem[v_i0])
                T.writes(T_softmax_exp[v_i0, v_i1])
                T_softmax_exp[v_i0, v_i1] = T.exp(compute[v_i0, v_i1] - T_softmax_maxelem[v_i0])
        for i0, k in T.grid(4096, 4096):
            with T.block("T_softmax_expsum"):
                v_i0, v_k = T.axis.remap("SR", [i0, k])
                T.reads(T_softmax_exp[v_i0, v_k])
                T.writes(T_softmax_expsum[v_i0])
                with T.init():
                    T_softmax_expsum[v_i0] = T.float32(0)
                T_softmax_expsum[v_i0] = T_softmax_expsum[v_i0] + T_softmax_exp[v_i0, v_k]
        for i0, i1 in T.grid(4096, 4096):
            with T.block("T_softmax_norm"):
                v_i0, v_i1 = T.axis.remap("SS", [i0, i1])
                T.reads(T_softmax_exp[v_i0, v_i1], T_softmax_expsum[v_i0])
                T.writes(T_softmax_norm[v_i0, v_i1])
                T.block_attr({"axis": 1})
                T_softmax_norm[v_i0, v_i1] = T_softmax_exp[v_i0, v_i1] / T_softmax_expsum[v_i0]
        for i0, i1, k in T.grid(4096, 64, 4096):
            with T.block("T_matmul_NN"):
                v_i0, v_i1, v_k = T.axis.remap("SSR", [i0, i1, k])
                T.reads(T_softmax_norm[v_i0, v_k], B_1[v_k, v_i1])
                T.writes(T_matmul_NN[v_i0, v_i1])
                with T.init():
                    T_matmul_NN[v_i0, v_i1] = T.float32(0)
                T_matmul_NN[v_i0, v_i1] = T_matmul_NN[v_i0, v_i1] + T_softmax_norm[v_i0, v_k] * B_1[v_k, v_i1]


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
