#!/bin/bash
# run benchmarks for different number of fields
# 1) Mode M: 0 dkfz, 1 integer fields, 2 bitmask fields, 3 integers & bitmasks
# 2) Number of fields F

server=phi0
client=phi1
tag="bitmask"
export T M=2 N=1 D=1000 F
cmd="./run_remote_ser.sh 5 $server $client"
for F in 1 2 4 8 16 32 64 128 256; do
  echo Running benchmark for ${tag}/F=${F}
  T="${tag}_${F}" C=1 S=0 $cmd
done
