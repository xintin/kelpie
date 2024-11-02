import os, sys, time, argparse, tvm
from tvm import te, autotvm, topi

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.dirname(SCRIPT_DIR))

from utils import *

## ------------------ Global ---------------------
# 128 84 83 83 5  5  2       SAME
# N   CI H  W  KH KW Strides Padding
input_shape = (128, 84, 83, 83)
filter_shape = (84, 1, 5, 5)
strides = (1, 1)
padding = (1, 1)
dilation = (1, 1)
layout = "NCHW"
dtype = "float32"
search_space = [1] + [i for i in range(2,65,2)]

## ----------------- Benchmark -------------------

def depthwise(input_shape, filter_shape):
    A = te.placeholder(input_shape, name="A", dtype=dtype)
    B = te.placeholder(filter_shape, name="B", dtype=dtype)
    C = topi.nn.depthwise_conv2d_nhwc(
        A, B, stride=strides, padding=padding, dilation=dilation, out_dtype=dtype
    )
    return [A, B, C]


## ----------------- Benchmark -------------------
@autotvm.template("depthwise")
def conv2d_autotvm(input_shape, filter_shape):
    A, B, C = depthwise(input_shape, filter_shape)
    s = te.create_schedule(C.op)

    #print(s[C].op.axis)

    # schedule
    b, i, j, c  = s[C].op.axis
    
    k = s[C].op.reduce_axis[0]

    # get the config object
    cfg = autotvm.get_config()

    # define search space
    cfg.define_knob("tile_b", search_space)
    cfg.define_knob("tile_i", search_space)
    cfg.define_knob("tile_j", search_space)
    cfg.define_knob("tile_c", search_space)

    # schedule according to config
    b0, b1 = s[C].split(b, cfg["tile_b"].val)
    i0, i1 = s[C].split(i, cfg["tile_i"].val)
    j0, j1 = s[C].split(j, cfg["tile_j"].val)
    c0, c1 = s[C].split(c, cfg["tile_c"].val)

    s[C].reorder(b0, j0, i0, c0, k, b1, j1, i1, c1)

    return s, [A, B, C]
## ---------------------------------------------

def autotvm_template(log_file, target, trials):
    task = autotvm.task.create("depthwise",args=(input_shape, filter_shape), target=target,)
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
        print("Archtecture doesn't support.")
        exit(0)

    autotvm_template(logfile, target, trials)
