#!/bin/bash
# parse folders in run/ with fields relevant for publication, which is assumed
# to reside next to the secure_epilink directory

groupby=parameters.boolSharing,parameters.arithConversion,parameters.dbSize,parameters.numRecords
splitby=parameters.boolSharing,parameters.arithConversion
sortby=parameters.dbSize
pubdir=../../2018-secure-record-linkage/data/
pattern=sizes

./parse_runs.py -C $groupby -S $splitby -s $sortby -F -o "${pubdir}${pattern}" "runs/${pattern}"*
