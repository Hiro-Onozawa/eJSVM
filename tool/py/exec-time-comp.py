import yaml
import matplotlib.pyplot as plt
import numpy as np
import re
import itertools
import sys
import argparse

## argparser ##

parser = argparse.ArgumentParser(description='generate exec time comparison graph')

parser.add_argument('--format', action="store", dest="format", default="eps");
parser.add_argument('--vms', action="store", dest="vms", default="py/Vms_64.yaml");
parser.add_argument('--benches', action="store", dest="benchmarks", default="py/Benchmarks.yaml");

ARGS = parser.parse_args()

## main ##

VM_LIST=ARGS.vms
BENCHMARK_LIST=ARGS.benchmarks

def load_benchmarks():
    with open(BENCHMARK_LIST) as f:
        return yaml.load(f)

def load_vms():
    with open(VM_LIST) as f:
        return yaml.load(f)

def benchmark_list(benchmarks):
    lst = []
    for suit in benchmarks:
        dir = suit['dir']
        for p in suit['programs']:
            name = next(iter(p))
            opts = p[name]
            lst.append([dir, name, opts])
    return lst

def extract_result_file_single(filename):
    try:
        with open(filename) as f:
            lst = []
            for line in f.readlines():
                #               total CPU time = 38.403 msec, total GC time = 1.646 msec, max GC time = 0.919 msec (#GC = 2)
                m = re.match(".*total CPU time =\s+(\d+)\.(\d+) msec, total GC time =\s+(\d+)\.(\d+) msec.*", line)
                if m:
                    lst.append( [int(m.group(1)) + (int(m.group(2)) / 1000.0), int(m.group(3)) + (int(m.group(4)) / 1000.0)] )
        return lst
    except:
        pass
    return None

def extract_result_file(result_file, config):
    cputime = []
    gctime = []
    lst = extract_result_file_single(result_file)
    if lst:
        for item in lst:
            c, g = item
            cputime.append(c)
            gctime.append(g)
    if len(cputime) == 0:
        return (np.array([0.0, 0.0, 0.0]), np.array([0.0, 0.0, 0.0]))
    return (np.percentile(cputime, [50, 25, 75]),
        np.percentile(gctime, [50, 25, 75]))

def extract_result_vm(bench, vm, config):
    result = []
    for heap in config['heapsize']:
        result_file = ("%s/%d_%d/results/raw/%s_%d_t3_%s.txt" % (vm['dats'], vm['basebit'], vm['target'], vm['algorithm'], heap['size'], bench))
        r = extract_result_file(result_file, config)
        result.append((r, bench, vm, heap))
    return result

def extract_result_bench(bench, config):
    result = []
    for vm in config['vms']:
        r = extract_result_vm(bench, vm, config)
        result.append((r, bench, vm))
    return result

def extract_result(config, benchmark_list):
    result = []
    for _,benchname,_ in benchmark_list:
        r = extract_result_bench(benchname, config)
        result.append((r, benchname))
    return result

def normalise_datapoint(data, baseline):
    return [x/baseline[0][0] for x in data]

def normalise_vm(baseline, vmdata):
    return [normalise_datapoint(x, y) for x, y in
            itertools.zip_longest(vmdata, baseline)]

def normalise(result):
    return [normalise_vm(result[0], x) for x in result]

def plot_figure_bench(data, benchname, config):
    width = 0.8 / len(data)
    heaplist = []
    fig = plt.figure()
    ax = fig.add_subplot(1,1,1)
   # total time
    for vm_i,(result,_,vm) in enumerate(data):
        left = []
        height = []
        yerr_up = []
        yerr_low = []
        for prog_i,(prog,_,_,heap) in enumerate(result):
            if vm_i == 0:
                heaplist.append(heap['name'])
            left.append(prog_i + vm_i * width - 0.5 + width)
            height.append(prog[0][0])
            yerr_up.append(prog[0][2] - prog[0][0])
            yerr_low.append(prog[0][0] - prog[0][1])
        ax.bar(left, height, width = width,
                color = vm['color'], edgecolor = 'black',
                yerr = (yerr_up, yerr_low), label = '%s' % (vm['name']),
                error_kw = {"elinewidth": 0.5, "capsize": 1, "capthick": 0.5})
    # GC
    left = []
    height = []
    yerr_up = []
    yerr_low = []
    for vm_i,(result,_,vm) in enumerate(data):
        for prog_i,(prog,_,_,heap) in enumerate(result):
            left.append(prog_i + vm_i * width - 0.5 + width)
            height.append(prog[1][0])
    ax.bar(left, height, width = width, label = 'GC time',
            edgecolor = 'black', fill=None, hatch='xxx')

    plt.xticks([i for i,_ in enumerate(heaplist)],
               [n for n in heaplist], rotation=270)
    ax.tick_params(axis='x', length = 0)
    ax.yaxis.set_major_formatter(plt.FuncFormatter(lambda x, loc: "{:,}".format(int(x))))
#    ax.set_xlabel('ヒープサイズ [KiB]')
#    ax.set_ylabel('実行時間 [ms]')
    ax.set_xlabel('Heap size [KiB]')
    ax.set_ylabel('Execution time [ms]')
    anchor = (1, 1)
    loc = 'upper right'
    if benchname != "dht11":
        anchor = (1, 0.5)
        loc = 'center right'
    fig.legend(bbox_to_anchor=anchor, loc=loc, borderaxespad=3)
    fig.tight_layout()
    fig.savefig('exectime_%s_%s.%s' % (config['fname'], benchname, ARGS.format), format=ARGS.format, bbox_inches='tight', pad_inches=0)
    plt.clf()

def plot_figures(data, config):
    for d,benchname in data:
        plot_figure_bench(d, benchname, config)

def print_time(data, vmlist, blist):
    for prog_i, prog in enumerate(blist):
        dir, bname, _ = blist[prog_i]
        sys.stdout.write('%s' % bname)
        for vm_i, vmname in enumerate(vmlist):
            sys.stdout.write('\t%f' % data[vm_i][prog_i][0][0])
        sys.stdout.write('\n')
    for prog_i, prog in enumerate(blist):
        dir, bname, _ = blist[prog_i]
        sys.stdout.write('%s' % bname)
        for vm_i, vmname in enumerate(vmlist):
            sys.stdout.write('\t%f' % data[vm_i][prog_i][1][0])
        sys.stdout.write('\n')

def main():
    config = load_vms()
    benchmarks = load_benchmarks()
    blst = benchmark_list(benchmarks)
    data = extract_result(config, blst)
#    data = normalise(result)
    plot_figures(data, config)
#    print_time(data, vmlist, blst)


main()
