#!/bin/bash
# run benchmarks for different number of fields
# 1) Config: 0 dkfz, 1 integer fields, 2 bitmask fields, 3 integers & bitmasks
# 2) number of fields

server=phi0
client=phi1
export T="combined_1" M=3 N=1 D=1000 F=1
cmd="./run_remote_ser.sh 5 $server $client"
C=1 S=0 $cmd
