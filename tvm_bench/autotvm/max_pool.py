import os, sys, time, argparse, tvm
from tvm import te, autotvm, topi
from tvm.topi.nn.utils import get_pad_tuple

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.dirname(SCRIPT_DIR))

from utils import *

## ------------------ Global ---------------------
input_shape = (128, 168, 83, 83)
dtype = "float32"
search_space = [1] + [i for i in range(2,129,2)]

# avg      128 168 83 83 1  2       VALID
# pooltype N,  CI, H, W, K, strides padding


## ----------------- Benchmark -------------------
def max_pool(input_shape, dtype="float32"):
    A = te.placeholder(shape=input_shape, name="A", dtype=dtype)
    B = topi.nn.pool2d(
        A, (1, 1), (2, 2), (1, 1), get_pad_tuple("VALID", (1, 1)), pool_type="avg"
    )
    return [A, B]

@autotvm.template("max_pool")
def max_pool_autotvm(input_shape, dtype="float"):
    A, B = max_pool(input_shape, dtype)
    s = te.create_schedule(B.op)

    #print(s[B].op.axis)
    # schedule
    n, y, x, j = s[B].op.axis

    # get the config object
    cfg = autotvm.get_config()

    # define search space
    cfg.define_knob("tile_n", search_space)
    cfg.define_knob("tile_x", search_space)
    cfg.define_knob("tile_y", search_space)
    cfg.define_knob("tile_j", search_space)

    # schedule according to config
    x0, x1 = s[B].split(x, cfg["tile_x"].val)
    y0, y1 = s[B].split(y, cfg["tile_y"].val)
    n0, n1 = s[B].split(n, cfg["tile_x"].val)
    j0, j1 = s[B].split(j, cfg["tile_j"].val)

    s[B].reorder(n0, y0, x0, j0, n1, y1, x1, j1)

    return s, [A, B]

## ---------------------------------------------


def generate_autotvm_template(log_file, target, trials):
    task = autotvm.task.create("max_pool", args=(input_shape, "float32"), target=target)
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
