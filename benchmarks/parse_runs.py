#!/usr/bin/env python3

from statistics import mean, stdev
from functools import reduce
from itertools import groupby
import sys, csv, argparse, os.path, glob, operator, toml

from collections.abc import MutableMapping

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

def parse_file(file,fields):
    data = toml.load(file, _dict=dict)
    if not data["correct"]:
        print("!Incorrect Calculation in ", file)
    fill_stats(data)
    yield flatten(data,sep='.')

def fill_stats(data):
    """Calculates Mean and StDev for all timings"""
    setup_series = [x["setup"] for x in data["timings"]]
    online_series = [x["online"] for x in data["timings"]]
    data["setupTime"] = {"mean": mean(setup_series),\
            "stdev": stdev(setup_series) if len(setup_series) > 1 else 0}
    data["onlineTime"] = {"mean": mean(online_series),\
            "stdev": stdev(online_series) if len(online_series) > 1 else 0}

def group_runs(runs, gby):
    keyf = lambda x: x[gby]
    try:
        sruns = sorted(runs, key=keyf)
    except KeyError as e:
        raise KeyError("%s is not a parameter by which runs can be grouped. \
                Use dbsize, nqueries etc." %gby) from e

    for _, groups in groupby(sruns, key=keyf):
        yield groups

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
        if key == 'runid': # save list of runids
            runids = list(values(key))
            data[key] = "%i..%i" %(runids[0], runids[-1])
        elif key in ['AvgCPU']:
            data[key] = mean(values(key))
        elif key in ['MaxMem'] or 'Comm' in key:
            data[key] = max(values(key))

    # Recalculate mean and Stdev
    fill_stats(data)

    return data

def parse_folder(path, fields, role='0'):
    assert os.path.isdir(path), "Path %s is not a folder."

    for fname in glob.iglob('%s/*run.%s*' %(path, role)):
        with open(fname) as file:
            yield from parse_file(file, fields)

def parse_paths(paths, fields, role='0'):
    for path in paths:
        if os.path.isfile(path) and ('run.'+role) in path:
            with open(path) as file:
                yield parse_file(file, fields)
        elif os.path.isdir(path):
            yield from parse_folder(path, fields, role)

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
            help="Output CSV file (default: stdout)")
    parser.add_argument("-d", "--delimiter", default=",",
            help="Delimiter to use in CSV output (default: ',')")
    parser.add_argument("-a", "--append", action='store_true',
            help="Append to output file")
    parser.add_argument("-F", "--overwrite", action='store_true',
            help="Overwrite output file")
    parser.add_argument("-f", "--fields", type=split_comma,
            default=["parameters.dbSize",
                "parameters.numRecords","circuit.total","baseOTs.time",\
                "communication.setupCommSent","communication.onlineCommSent",\
                "setupTime.mean","setupTime.stdev","onlineTime.mean",\
                "onlineTime.stdev"],
            help="Comma separated list of fields to include in output.\
                    Subfields are separated by a dot")
    parser.add_argument("-r", "--role", default='0',
            help="0/1/'': Role to filter folders to. Default: 0 (Server)")
    parser.add_argument("-C", "--combine", metavar="parameter", default=None,
            help="Combine runs for given parameter. Calculates Avgs and Maxs")

    args = parser.parse_args()

    if not args.output == '-' and os.path.isfile(args.output) and not (args.overwrite or args.append):
        print("Output file exists. Either choose overwrite or append or change output path")
        exit(1)

    # Interpret \t as tab char
    if args.delimiter == "\\t": args.delimiter = "\t"

    return args

def main():
    args = parse_args()

    # Read in runs
    runs = parse_paths(args.paths, fields=args.fields, role=args.role)

    # Group and combine if necessary
    if args.combine is not None:
        runs = (combine_runs(list(rung)) for rung in group_runs(runs, args.combine))

    mode = 'a' if args.append else 'w'
    csvfile = sys.stdout if args.output == '-' else open(args.output, mode)
    try:
        write_csv(runs, args.fields, csvfile, args.delimiter)
    finally:
        if csvfile is not sys.stdout:
            csvfile.close()

    exit(0)

if __name__ == '__main__': main()
