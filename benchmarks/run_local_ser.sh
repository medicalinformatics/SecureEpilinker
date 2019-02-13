#!/bin/bash
#
# Run me like
# $ D=5 N=1000 ./run_local_par.sh 3
# to run three (pairs of) local instances serially with the optional parameters
# D, N, S, C prepended as environment variables
# assumes example directory=benchmarks name to be 'test_sel'
# Writes output to a file with all parameters included in their file names
# Average CPU load and max. memory (RSS - resident set size) is written at the
# end of the file
#
# Processes are started in local shell and can be stopped with Ctrl+C

exe="../build/test_sel"
D=${D:-1}      # Number of records in database
N=${N:-1}      # Number of records to link
C=${C:-""}     # Use Arithmetic circuit
S=${S:-1}      # Boolean Sharing to use, 0: GMW 1: YAO
R=${1:-1}      # Number of seriell runs

if [[ ${C} ]]
then
  C="-c"
fi

ts=$(date +%s)
arith=${C+1}
out_base="$(hostname)_${ts}_D${D}_N${N}_S${S}_C${arith-0}_sel"
dir="runs/${out_base}"
session="sel-local-${ts}"
mkdir -p $dir

cmd="$exe ${C} -s ${S} -n ${D} -N ${N}"
cmd_time="/usr/bin/time -f '[TimeStats]\nAvgCPU=\"%P\"\nMaxMem=%M' -a -o"
echo "cmd=$cmd"

# Arg 1: role (0 or 1)
run-role() {
  out="${dir}/run.${1}"
  echo "Starting local queries for role $1 -> ${out}_r*"
  for r in $(seq $R); do
    echo "********** Run ${r} of ${R} **********"
    out1="${out}_r${r}"
    eval "${cmd_time} ${out1} $cmd -r $1 -B ${out1}| tee ${out1}"
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
# TK: Signal 13 means "broken pipe", i.e. something is written to a pipe 
# where nothing is read from anymore (e.g. see 
# http://people.cs.pitt.edu/~alanjawi/cs449/code/shell/UnixSignals.htm ).
