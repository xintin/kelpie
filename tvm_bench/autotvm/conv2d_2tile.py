import os, sys, time, argparse, tvm
from tvm import te, autotvm, topi

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.dirname(SCRIPT_DIR))

from utils import *

## ------------------ Global ---------------------
input_shape = (1, 3, 1000, 1000)
filter_shape = (64, 3, 3, 3)
strides = (1, 1)
padding = (1, 1)
dilation = (1, 1)
layout = "NCHW"
dtype = "float32"
search_space = [1] + [i for i in range(2,65,2)]

def conv2d(input_shape, filter_shape):
    A = te.placeholder(input_shape, name="A", dtype=dtype)
    W = te.placeholder(filter_shape, name="W", dtype=dtype)
    C = topi.nn.conv2d(
        A, W, strides, padding, dilation, data_layout=layout, out_dtype=dtype
    )
    return [A, W, C]


## ----------------- Benchmark -------------------
@autotvm.template("conv2d")
def conv2d_autotvm(input_shape, filter_shape):
    A, B, C = conv2d(input_shape, filter_shape)
    s = te.create_schedule(C.op)

    # schedule
    n, f, y, x  = s[C].op.axis
    k = s[C].op.reduce_axis[0]

    # get the config object
    cfg = autotvm.get_config()

    # define search space for only x and y
    cfg.define_knob("tile_x", search_space)
    cfg.define_knob("tile_y", search_space)

    # schedule according to config
    x0, x1 = s[C].split(x, cfg["tile_x"].val)
    y0, y1 = s[C].split(y, cfg["tile_y"].val)

    # reorder to tile x and y only
    s[C].reorder(y0, x0, k, f, y1, x1)

    return s, [A, B, C]


def autotvm_template(log_file, target, trials):
    task = autotvm.task.create("conv2d",args=(input_shape, filter_shape), target=target,)
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
        "python print_record_info.py -m 'ansor' -a x86 -l 'results/cpu_matmul.json' -i 3"
    )
    parser.add_argument(
        "-a", "--arch", type=str, required=True, help="Options: x86, aarch64, cuda"
    )
    parser.add_argument("-l", "--logfile", type=str, required=True)
    parser.add_argument("-t", "--trials", type=int, default=1000)
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
        print("Architecture doesn't support.")
        exit(0)

    autotvm_template(logfile, target, trials)

