#!/bin/bash
# run benchmarks for different request and DB sizes

server=phi0
client=phi1
export T="sizes" C D N S
cmd_gmw="./run_remote_ser.sh 5 $server $client"
# only one yao run to check that it is really that much worse
cmd_yao="./run_remote_ser.sh 1 $server $client"

for C in 0 1; do
  N=1
  for D in 1 10 25 100 250 1000 2500 10000; do
    S=0 $cmd_gmw
    S=1 $cmd_yao
  done
  # GMW RAM usage can take it
  S=0
  D=25000 $cmd_gmw
  # for D=100000 there's an overflow in CBitVector...
done
