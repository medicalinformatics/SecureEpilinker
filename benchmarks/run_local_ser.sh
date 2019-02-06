#!/bin/bash
#
# Run me like
# $ Q=5 N=1000 ./run_local_par.sh 3
# to run three (pairs of) local instances serially with the optional parameters
# Q, N, S prepended as environment variables
# assumes example directory=binary name to be 'variant_query'
# Writes output to a file with all parameters included in their file names
# Average CPU load and max. memory (RSS - resident set size) is written at the
# end of the file
#
# Processes are started in local shell and can be stopped with Ctrl+C

exe="../../../bin/variant_query.exe"
Q=${Q:-1}      # Number of queries
N=${N:-100000} # Size of a single entry
S=${S:-5}      # Size of a query
#P=${P:-6650}   # Port base - for parallel runs
R=${1:-1}      # Number of seriell runs
K=${K:-32}     # dictionary key bit length
V=${V:-16}     # dictionary value bit length

ts=$(date +%s)
out_base="$(hostname)_${ts}_N${N}_Q${Q}_S${S}_R${R}_ser"
dir="runs/${out_base}"
session="vq-local-${ts}"
mkdir -p $dir

cmd="$exe -q $Q -n $N -s $S -v $V -k $K"
cmd_time="/usr/bin/time -f '#TimeStats\nAvgCPU\t%P\nMaxMem\t%M' -a -o"
echo "cmd=$cmd"

# Arg 1: role (0 or 1)
run-role() {
  out="${dir}/run.${1}"
  echo "Starting local queries for role $1 -> ${out}_r*"
  for r in $(seq $R); do
    echo "********** Run ${r} of ${R} **********"
    out1="${out}_r${r}"
    eval "${cmd_time} ${out1} $cmd -r ${1} | tee ${out1}"
  done
}

run-role 0 &
pid0="$!"
run-role 1 &>/dev/null & # Only print Server output
pid1="$!"
echo "Server PID: ${pid0} Client PID: ${pid1}"

cleanup () {
  echo "Interrupt received. Killing server and client loops..."
  kill "${pid0}"
  kill "${pid1}"
  exit 1
}
trap cleanup SIGINT SIGTERM

wait $pid0
wait $pid1

# Weirdly, sometimes time prints the string 'Command terminated by signal 13',
# so we have to manually delete it afterward :( E.g. with
# $ sed -i -e '/Command terminated by signal 13/d' *
