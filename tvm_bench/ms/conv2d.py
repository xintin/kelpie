import os, sys, time, argparse, tvm
from tvm import te, topi

from tvm import meta_schedule as ms
from tvm.meta_schedule.runner.config import EvaluatorConfig

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.dirname(SCRIPT_DIR))

from tvm_bench.utils import *

## ------------------ Global ---------------------
input_shape = (1, 3, 224, 224)
filter_shape = (64, 3, 3, 3)
strides = (1, 1)
padding = (1, 1)
dilation = (1, 1)
layout = "NCHW"
dtype = "float32"


## ----------------- Benchmark -------------------
# @ms.register_workload
def conv2d_ansor(input_shape, filter_shape):
    A = te.placeholder(input_shape, name="A", dtype=dtype)
    W = te.placeholder(filter_shape, name="W", dtype=dtype)
    C = topi.nn.conv2d(
        A, W, strides, padding, dilation, data_layout=layout, out_dtype=dtype
    )
    return [A, W, C]


def generate_ansor_template(log_file, target, trials):
    # task = tvm.meta_schedule.TaskScheduler(
    #    func=conv2d_ansor, args=(input_shape, filter_shape), target=target
    # )

    start = time.time()
    with ms.Profiler() as profiler:
        database = ms.relay_integration.tune_relay(
            mod=conv2d_ansor,
            target=target,
            params=(input_shape, filter_shape),
            work_dir=logfile,
            max_trials_global=trials,
            num_trials_per_iter=64,
            runner=ms.runner.LocalRunner(
                evaluator_config=EvaluatorConfig(
                    number=10,
                    repeat=3,
                    min_repeat_ms=100,
                    enable_cpu_cache_flush=True if target == "llvm" else False,
                )
            ),
            cost_model=ms.cost_model.XGBModel(
                extractor=ms.feature_extractor.PerStoreFeature(),
                adaptive_training=False,
            ),
            strategy=ms.search_strategy.EvolutionarySearch(),
        )
        lib = ms.relay_integration.compile_relay(
            database=database,
            mod=mod,
            target=target,
            params=params,
        )
    end = time.time()
    print(f"Tuning Time (min): {(end-start)/60:.2f}")
    print(profiler.table())

    start = time.time()
    ## Run auto-tuning (search)
    # task.tune(tune_option)
    # end = time.time()

    time_avg, best_cfg = get_best_time(log_file)

    print("Time spent:", time_avg)
    print("Config:", best_cfg)
    print("Time spent to search:", end - start)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        "python print_record_info.py -m 'ansor' -a x86 -l 'results/cpu_matmul.json' -i 3"
    )
    parser.add_argument(
        "-m", "--method", type=str, required=True, help="Options: ansor, droplet"
    )
    parser.add_argument(
        "-a", "--arch", type=str, required=True, help="Options: x86, aarch64, cuda"
    )
    parser.add_argument("-l", "--logfile", type=str, required=True)
    parser.add_argument("-t", "--trials", type=int, default=100)
    args = parser.parse_args()

    method = args.method
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

    if method == "ansor":
        generate_ansor_template(logfile, target, trials)
    elif method == "droplet":
        build_template("conv2d", logfile, target, trials)
