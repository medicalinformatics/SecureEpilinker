#!/bin/bash
# parse folders in run/ with fields relevant for publication, which is assumed
# to reside next to the secure_epilink directory

parseparam=numFields
groupby=$parseparam
sortby=$parseparam
pubdir=../../2018-secure-record-linkage/data/
pattern=combined

./parse_runs.py -C $groupby -s $sortby --parse-num-parameter $parseparam \
  -F -o "${pubdir}${pattern}" "runs/${pattern}"*
