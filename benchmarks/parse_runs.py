#!/usr/bin/env python3

from statistics import mean, stdev
from functools import reduce
from itertools import groupby
import sys, csv, argparse, os.path, glob, operator, toml

def fill_stats(data):
    """Calculates Mean and StDev for all timings"""
    # data['header'] = header
    # data['timings'] = timings
    # # Skip first column 'Time'
    # for i, phase in enumerate(header[1:]):
        # series = [x[i+1] for x in timings]
        # data['First'+phase] = series[0]
        # data['Avg'+phase] = mean(series)
        # data['Stdev'+phase] = stdev(series) if len(timings) > 1 else 0
    setup_series = [x["setup"] for x in data["timings"]]
    online_series = [x["online"] for x in data["timings"]]
    data["setupTime"] = {"mean": mean(setup_series),\
            "stdev": stdev(setup_series) if len(setup_series) > 1 else 0}
    data["onlineTime"] = {"mean": mean(online_series),\
            "stdev": stdev(online_series) if len(online_series) > 1 else 0}


def parse_file(file):
    data = toml.load(file, _dict=dict)
    if not data["correct"]:
        print("!Incorrect Calculation in ", file)
    # section, header, timings, data = None, None, [], {}
    # def data_sec(section):
        # return data if flat else data[section]

    # for line in file:
        # if line.startswith('#'):
            # section = line[1:-1] # remove trailing \n
            # if not flat: data[section] = {}
            # continue

        # # Make sure we don't have a broken file that doesn't begin with a section
        # assert section is not None, "File doesn't start with a section name"

        # if section == 'QueryTimings':
            # sline = line.split()
            # # If header is not set, we're at the first line, specifying the header
            # if not header:
                # header = sline
                # continue

            # tline = tuple(tonum(*entry) for entry in zip(header,sline))

            # # Append line to array otherwise
            # timings.append(tline)

        # else:
            # key, value = line.split()
            # datasec = data if flat else data[section]
            # data_sec(section)[key] = tonum(key, value)

    # # Check that number of queries match
    # nqueries = data_sec('Parameters')['nqueries']
    # assert len(timings) == nqueries, \
        # "Expected %i query timings, got %i" % (nqueries, len(timings))

    # # Post Processign
    # fill_stats(data_sec('QueryTimings'), header, timings)
    fill_stats(data)
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
    fill_stats(data)

    return data

def parse_folder(path, role='0'):
    assert os.path.isdir(path), "Path %s is not a folder."

    for fname in glob.iglob('%s/*run.%s*' %(path, role)):
        print("DEBUG::",fname)
        with open(fname) as file:
            yield parse_file(file)

def parse_paths(paths, role='0'):
    for path in paths:
        if os.path.isfile(path) and ('run.'+role) in path:
            with open(path) as file:
                yield parse_file(file)
        elif os.path.isdir(path):
            yield from parse_folder(path, role)

def filter_data(data_gen, fields):
    """Filter Data to print according to input arg. This allows the printing of
    nested information, formated as "circuit.Arithmetic.B2A". Max level of
    nesting allowed is 2 (i.e. as in previous example)"""
    row = []
    data = dict(next(data_gen))
    for datum in sorted(fields):
        heading = datum.split(".")
        if len(heading) > 3 or len(heading) == 0:
            raise KeyError("Invalid number of fields")
        if len(heading) == 1:
            row.append(data[datum])
        elif len(heading) == 2:
            row.append(data[heading[0]][heading[1]])
        elif len(heading) == 3:
            row.append(data[heading[0]][heading[1]][heading[2]])
    yield row


def write_csv(runs, fields, csvfile, delimiter):
    filtered_data = filter_data(runs, fields)
    writer = csv.writer(csvfile, quotechar='"', delimiter=delimiter)
    writer.writerow(sorted(fields))
    writer.writerows(filtered_data)

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
    runs = parse_paths(args.paths, role=args.role)

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
