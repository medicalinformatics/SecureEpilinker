#!/bin/bash
# run benchmarks for different combinations of sharing
# 1) GMW or Yao boolean sharing
# 2) whether to use arithmetic sharing for arithmetic circuit parts

server=phi0
client=phi1
export T=sharing N=100 D=100
cmd="./run_remote_ser.sh 10 $server $client"
C=0 S=0 $cmd
C=0 S=1 $cmd
C=1 S=0 $cmd
C=1 S=1 $cmd
