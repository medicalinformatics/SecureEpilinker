#!/usr/bin/env python3

from statistics import mean, stdev
from functools import reduce
from itertools import groupby
import sys, csv, argparse, os.path, glob, operator

def tonum(key, value):
    if key in ['BaseOTsTiming','Setup','Online']:
        return float(value)
    elif key == 'AvgCPU':
        return int(value[:-1])
    else:
        return int(value)

def fill_stats(data, header, timings):
    data['header'] = header
    data['timings'] = timings
    # Skip first column 'Time'
    for i, phase in enumerate(header[1:]):
        series = [x[i+1] for x in timings]
        data['First'+phase] = series[0]
        data['Avg'+phase] = mean(series)
        data['Stdev'+phase] = stdev(series) if len(timings) > 1 else 0


def parse_file(file, flat=True):
    section, header, timings, data = None, None, [], {}
    def data_sec(section):
        return data if flat else data[section]

    for line in file:
        if line.startswith('#'):
            section = line[1:-1] # remove trailing \n
            if not flat: data[section] = {}
            continue

        # Make sure we don't have a broken file that doesn't begin with a section
        assert section is not None, "File doesn't start with a section name"

        if section == 'QueryTimings':
            sline = line.split()
            # If header is not set, we're at the first line, specifying the header
            if not header:
                header = sline
                continue

            tline = tuple(tonum(*entry) for entry in zip(header,sline))

            # Append line to array otherwise
            timings.append(tline)

        else:
            key, value = line.split()
            datasec = data if flat else data[section]
            data_sec(section)[key] = tonum(key, value)

    # Check that number of queries match
    nqueries = data_sec('Parameters')['nqueries']
    assert len(timings) == nqueries, \
        "Expected %i query timings, got %i" % (nqueries, len(timings))

    # Post Processign
    fill_stats(data_sec('QueryTimings'), header, timings)

    return data

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
        elif key in ['BaseOTsTiming', 'AvgCPU']:
            data[key] = mean(values(key))
        elif key in ['MaxMem'] or 'Comm' in key:
            data[key] = max(values(key))

    # Recalculate mean and Stdev
    fill_stats(data, data['header'], reduce(operator.concat, values('timings')))

    return data

def parse_folder(path, flat=True, role='0'):
    assert os.path.isdir(path), "Path %s is not a folder."

    for fname in glob.iglob('%s/*run.%s*' %(path, role)):
        with open(fname) as file:
            yield parse_file(file, flat)

def parse_paths(paths, flat=True, role='0'):
    for path in paths:
        if os.path.isfile(path) and ('run.'+role) in path:
            with open(path) as file:
                yield parse_file(file)
        elif os.path.isdir(path):
            yield from parse_folder(path, flat, role)

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
            default=["runid","dbsize","qsize","NumANDGates","Depth",
                "TotalGates","BaseOTsTiming","AvgSetup","AvgOnline"],
            help="Comma separated list of fields to include in output")
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
    runs = parse_paths(args.paths, flat=True, role=args.role)

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
