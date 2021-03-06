#!/usr/bin/env python3

from statistics import mean, stdev
from itertools import groupby
from collections.abc import MutableMapping
import sys, csv, argparse, os.path, glob, toml, re


def flatten(d, parent_key='', sep='_'):
    """ Flatten nested dictionaries with configurable separator. Taken from
    StackExchange: https://stackoverflow.com/a/6027615 """
    items = []
    for k, v in d.items():
        new_key = parent_key + sep + k if parent_key else k
        if isinstance(v, MutableMapping):
            items.extend(flatten(v, new_key, sep=sep).items())
        else:
            items.append((new_key, v))
    return dict(items)

def parse_file(file):
    data = toml.load(file, _dict=dict)
    if not data["correct"]:
        print("!Incorrect Calculation in ", file)

    # AvgCPU needs special treatment...
    data["TimeStats"]["AvgCPU"] = int(data["TimeStats"]["AvgCPU"][:-1])
    fill_stats(data)
    data = flatten(data, sep='.')
    return data


def fill_stats(data):
    """Calculates Mean and StDev for all timings and saves them to the provided
    data object. Uses flat keys {phase}Time.{mean,stdev}"""
    for phase in ["setup", "online"]:
        series = [x[phase] for x in data["timings"]]
        data[phase + "Time.mean"] = mean(series)
        data[phase + "Time.stdev"] = stdev(series) if len(series) > 1 else 0
    data["count"] = len(data["timings"])


def group_runs(runs, gby):
    keyf = lambda x: "+".join((str(x[key]) for key in gby))
    try:
        sruns = sorted(runs, key=keyf) # groupby only works properly on sorted iterable
    except KeyError as e:
        raise KeyError((f"{gby} aren't parameters by which runs can be grouped."
            " Use parameters.dbSize, parameters.numRecords etc.")) from e

    yield from groupby(sruns, key=keyf)

def combine_runs(runs):
    """Combines several run dicitonaries to one, taking the average over
    timing outputs and max over memory and communication outputs
    Only works for flat dictionaries"""

    if len(runs) == 1: return runs[0]

    # Copy static data
    data = runs[0].copy()
    assert 'Parameters' not in data, \
            "Run combination only supported for flat dictionaries."

    def values(key):
        return (run[key] for run in runs)

    for key in data:
        if key in ['TimeStats.AvgCPU']:
            data[key] = mean(values(key))
        elif key in ['TimeStats.MaxMem'] or key.startswith("communication"):
            data[key] = max(values(key))

    # Merge timings
    data["timings"] = [ts for run in runs for ts in run["timings"]]
    # Recalculate mean and Stdev of timings
    fill_stats(data)

    return data

def parse_folder(path, role='0', parseparam=None):
    assert os.path.isdir(path), f"{path} is not a folder."

    # parse number from path name
    if parseparam:
        parsenum = int(re.match(r"[a-zA-Z0-9]+_(\d+)_", os.path.basename(path)).group(1))

    for fname in glob.iglob(f'{path}/*run.{role}*'):
        with open(fname) as file:
            data = parse_file(file)
            if parseparam: data[parseparam] = parsenum
            yield data

def parse_paths(paths, role='0', parseparam=None):
    for path in paths:
        if os.path.isfile(path) and ('run.'+role) in path:
            with open(path) as file:
                yield parse_file(file)
        elif os.path.isdir(path):
            yield from parse_folder(path, role, parseparam)

def write_csv(runs, fields, csvfile, delimiter):
    w = csv.DictWriter(csvfile, fieldnames=fields, extrasaction='ignore',
            delimiter=delimiter)
    w.writeheader()
    w.writerows(runs)

def parse_args():
    def split_comma(s): return s.split(',')

    parser = argparse.ArgumentParser(description="Converts single aby runs into a csv table for further analysis/pgfplots")
    parser.add_argument("paths", metavar="path", nargs="+",
            help="Input .run files or folders containing them")
    parser.add_argument("-o", "--output", default="-",
            help="Output CSV file basename (default: stdout)")
    parser.add_argument("-d", "--delimiter", default=",",
            help="Delimiter to use in CSV output (default: ',')")
    parser.add_argument("-a", "--append", action='store_true',
            help="Append to output file")
    parser.add_argument("-F", "--overwrite", action='store_true',
            help="Overwrite output file")
    parser.add_argument("-f", "--fields", type=split_comma,
            default=["parameters.dbSize", "parameters.numRecords",\
                "parameters.boolSharing", "parameters.arithConversion",\
                "TimeStats.AvgCPU", "TimeStats.MaxMem", \
                "circuit.total", "circuit.rounds", \
                "communication.setupCommSent", "communication.setupCommRecv",\
                "communication.onlineCommSent", "communication.onlineCommRecv",\
                "setupTime.mean", "setupTime.stdev",\
                "onlineTime.mean", "onlineTime.stdev",\
                "count"],
            help=("Comma separated list of fields to include in output. "
                  "Subfields are separated by a dot"))
    parser.add_argument("-r", "--role", default='0',
            help="0/1/'': Role to filter folders to. Default: 0 (Server)")
    parser.add_argument("-C", "--combine", metavar="parameters",\
            type=split_comma, default=None,\
            help="Combine runs for comma-separated list of parameters. Calculates Avgs and Maxs")
    parser.add_argument("-S", "--split", metavar="parameters",\
            type=split_comma, default=None,\
            help=("Split output by those parameters. Output will be named "
                  "{output}_{1}+{...}+{n}.csv"))
    parser.add_argument("-s", "--sort", help="Sort output table by this field")
    parser.add_argument("--parse-num-parameter", metavar="parameter",
            dest="parseparam", help=(
            "Parse a numerical parameter from the folder name into given "
            "parameter. The number is parsed from the pattern [a-zA-Z0-9]+_{number}_*"))

    args = parser.parse_args()

    if not args.output == '-' and glob.glob(args.output+'*') and not (args.overwrite or args.append):
        print("Output file exists. Either choose overwrite or append or change output path")
        sys.exit(1)

    # Interpret \t as tab char
    if args.delimiter == "\\t": args.delimiter = "\t"

    # prepend parse-parameter to fields list
    if args.parseparam and args.parseparam not in args.fields:
        args.fields.insert(0, args.parseparam)

    return args

def main():
    args = parse_args()

    # Read in runs
    runs = parse_paths(args.paths, role=args.role, parseparam=args.parseparam)

    # Group and combine if necessary
    if args.combine is not None:
        runs = (combine_runs(list(rung)) for _, rung in group_runs(runs, args.combine))

    mode = 'a' if args.append else 'w'

    def out_file(key: str):
        if args.output == '-': return sys.stdout
        out = f"{args.output}_{key}.csv" if key else args.output+'.csv'
        return open(out, mode)

    def sorted_run(run):
        if not args.sort: return run
        else: return sorted(run, key=lambda x: x[args.sort])

    if args.split is not None:
        for key, rung in group_runs(runs, args.split):
            with out_file(key) as f:
                write_csv(sorted_run(rung), args.fields, f, args.delimiter)
    else:
        with out_file(None) as f:
            write_csv(sorted_run(runs), args.fields, f, args.delimiter)

    sys.exit(0)

if __name__ == '__main__': main()
