#!/usr/bin/env python3

# Prints stats for score tables genrated like:
# ./test_sel -L --bm-density-shift 2 -M 2 -N 10000 -T > ../../scores_bm_2.csv

import numpy as np
import pandas as pd
import sys

TYPES = ["16", "32", "64", "double"]

def main():
    print("Reading file", sys.argv[1])
    s = pd.read_csv(sys.argv[1], delimiter=';')

    for t in TYPES:
        s["sc_"+t] = s["num_"+t] / s["den_"+t]
        print("Type:", t, "\n", s["sc_"+t].describe())

    print("Bit-length\tmin\t\tmax\t\t\tmean (absolute deviations)")
    # last type is double - compare other types to it
    for t in TYPES[:-1]:
        scdev = s["sc_double"]- s["sc_"+t]
        print("{}\t{}\t{}\t{}".format(t, scdev.min(), scdev.max(), scdev.mean()))

if __name__ == "__main__":
    main()
