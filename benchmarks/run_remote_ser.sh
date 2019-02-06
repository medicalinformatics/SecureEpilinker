#!/bin/bash
#
# Run me like
# $ Q=5 N=1000 ./run_local_par.sh 3 bioclust3 bioclust7
# to run three (pairs of) remote instances serially with the optional parameters
# Q, N, S prepended as environment variables. Second argument is remote host
# that becomes the server, third argument becomes the client.
# Assumes binary path to be '~/aby-vq/bin/variant_query.exe' on remotes
# Outputs are written to one file per run. The path name includes all parameters.
# Average CPU load and max. memory (RSS - resident set size) is written at the
# end of each file
#
# Processes are started in local shell and can be stopped with Ctrl+C
# Output is written to local runs/ folder. The resulting subfolder will be
# prepended with a tag if given in variable T
#
# You should use ssh's ControlMaster capabilities to speedup remote execution,
# see https://puppet.com/blog/speed-up-ssh-by-reusing-connections

exe="aby-vq/bin/variant_query.exe"
Q=${Q:-1}      # Number of queries
N=${N:-100000} # Size of a single entry
S=${S:-5}      # Size of a query
P=${P:-6677}   # Port
R=${1:-1}      # Number of seriell runs
K=${K:-32}     # dictionary key bit length
V=${V:-16}     # dictionary value bit length
declare -a remote
remote[0]=${2:-localhost}
remote[1]=${3:-localhost}
echo "Remote server: ${remote[0]} client: ${remote[1]}"

ts=$(date +%s)
out_base="${T}${T:+_}${remote[0]}_${remote[1]}_${ts}_N${N}_Q${Q}_S${S}_R${R}_ser"
dir="runs/${out_base}"
rdir="/tmp/${dir}"
session="vq-remote-${ts}"
mkdir -p $dir
for role in 0 1; do ssh ${remote[$role]} "mkdir -p $rdir"; done

cmd="$exe -q $Q -n $N -s $S -v $V -k $K -a ${remote[0]} -p $P"
cmd_time="/usr/bin/time -f '#TimeStats\nAvgCPU\t%P\nMaxMem\t%M' -a -o"
echo "cmd=$cmd"

# Arg 1: role (0 or 1)
run-role() {
  out="${dir}/run.${1}"
  rout="${rdir}/run.${1}"
  echo "Starting local queries for role $1 -> ${out}_r*"
  for r in $(seq $R); do
    echo "********** Run ${r} of ${R} **********"
    out1="${out}_r${r}"
    rout1="${rout}_r${r}"
    ssh ${remote[$1]} "${cmd_time} ${rout1} $cmd -r ${1} > ${rout1}; cat ${rout1}" | tee ${out1}
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

echo "********** Done. You may cleanup /tmp/runs/ on remotes **********"

# Weirdly, sometimes time prints the string 'Command terminated by signal 13',
# so we have to manually delete it afterward :( E.g. with
# $ sed -i -e '/Command terminated by signal 13/d' *
