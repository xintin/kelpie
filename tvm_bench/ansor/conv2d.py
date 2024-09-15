import os, sys, time, argparse, tvm
from tvm import te, auto_scheduler, topi
import tvm.auto_scheduler
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
input_shape = (1, 3, 224, 224)
filter_shape = (64, 3, 3, 3)
strides = (1, 1)
padding = (1, 1)
dilation = (1, 1)
layout = "NCHW"
dtype = "float32"


## ----------------- Benchmark -------------------
@auto_scheduler.register_workload
def conv2d_ansor(input_shape, filter_shape):
    A = te.placeholder(input_shape, name="A", dtype=dtype)
    W = te.placeholder(filter_shape, name="W", dtype=dtype)
    C = topi.nn.conv2d(
        A, W, strides, padding, dilation, data_layout=layout, out_dtype=dtype
    )
    return [A, W, C]


@tvm.script.ir_module
class Main:
    @T.prim_func
    def main(
        A: T.Buffer((1, 3, 224, 224), "float32"),
        W: T.Buffer((64, 3, 3, 3), "float32"),
        conv2d_nchw: T.Buffer((1, 64, 224, 224), "float32"),
    ):
        T.func_attr({"tir.noalias": T.bool(True)})
        # with T.block("root"):
        pad_temp = T.alloc_buffer((1, 3, 226, 226))
        for i0, i1, i2, i3 in T.grid(1, 3, 226, 226):
            with T.block("pad_temp"):
                v_i0, v_i1, v_i2, v_i3 = T.axis.remap("SSSS", [i0, i1, i2, i3])
                T.reads(A[v_i0, v_i1, v_i2 - 1, v_i3 - 1])
                T.writes(pad_temp[v_i0, v_i1, v_i2, v_i3])
                pad_temp[v_i0, v_i1, v_i2, v_i3] = T.if_then_else(
                    1 <= v_i2 and v_i2 < 225 and 1 <= v_i3 and v_i3 < 225,
                    A[v_i0, v_i1, v_i2 - 1, v_i3 - 1],
                    T.float32(0),
                )
        for nn, ff, yy, xx, rc, ry, rx in T.grid(1, 64, 224, 224, 3, 3, 3):
            with T.block("conv2d_nchw"):
                v_nn, v_ff, v_yy, v_xx, v_rc, v_ry, v_rx = T.axis.remap(
                    "SSSSRRR", [nn, ff, yy, xx, rc, ry, rx]
                )
                T.reads(
                    pad_temp[v_nn, v_rc, v_yy + v_ry, v_xx + v_rx],
                    W[v_ff, v_rc, v_ry, v_rx],
                )
                T.writes(conv2d_nchw[v_nn, v_ff, v_yy, v_xx])
                with T.init():
                    conv2d_nchw[v_nn, v_ff, v_yy, v_xx] = T.float32(0)
                conv2d_nchw[v_nn, v_ff, v_yy, v_xx] = (
                    conv2d_nchw[v_nn, v_ff, v_yy, v_xx]
                    + pad_temp[v_nn, v_rc, v_yy + v_ry, v_xx + v_rx]
                    * W[v_ff, v_rc, v_ry, v_rx]
                )

def generate_ansor_template(log_file, target, trials):
    task = tvm.auto_scheduler.SearchTask(
        func=conv2d_ansor, args=(input_shape, filter_shape), target=target
    )

    ## Set Parameters for Auto-Scheduler
    tune_option = auto_scheduler.TuningOptions(
        num_measure_trials=trials,  # change this to 20000 to achieve the best performance
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
        "python conv2d.py -a x86 -l 'results/cpu_matmul.json' -i 3"
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

