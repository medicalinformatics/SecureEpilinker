#!/bin/bash
# parse folders in run/ with fields relevant for publication, which is assumed
# to reside next to the secure_epilink directory

groupby=parameters.boolSharing,parameters.arithConversion,parameters.dbSize,parameters.numRecords
pubdir=../../2018-secure-record-linkage/benchmarks/
pattern=sizes

./parse_runs.py -C $groupby -o "${pubdir}${pattern}.csv" "runs/${pattern}"*
