#!/bin/bash
#
# Run me like
# $ N=100 D=100 ./run_remote_ser.sh 5 phi0 phi1
# to run three (pairs of) remote instances serially with the optional parameters
# D, N, S, C prepended as environment variables. Second argument is remote host
# that becomes the server, third argument becomes the client.
# Assumes binary path to be '~/secure_epilink/build/test_sel' on remotes
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

exepath="secure_epilink/build"
exe="./test_sel"
ssh_user="kussel"
D=${D:-1}      # Number of records in database
N=${N:-1}      # Number of records to link
C=${C:-0}      # Use Arithmetic circuit
S=${S:-1}      # Boolean Sharing to use, 0: GMW 1: YAO
R=${1:-1}      # Number of seriell runs
declare -a remote
remote[0]=${2:-localhost}
remote[1]=${3:-localhost}

serverip=$(getent ahostsv4 ${remote[0]} | awk '{ print $1}' | head -n1)
echo "Remote server: ${remote[0]} client: ${remote[1]}"
echo "Server IP: ${serverip}"

ts=$(date +%s)
out_base="${T}${T:+_}${remote[0]}_${remote[1]}_${ts}_D${D}_N${N}_S${S}_C${C}_sel"
dir="runs/${out_base}"
rdir="~/tmp/${dir}"
session="sel-remote-${ts}"
mkdir -p $dir
for role in 0 1; do ssh -l ${ssh_user} ${remote[$role]} "mkdir -p $rdir"; done

[[ $C -eq 1 ]] && c_arg="-c"
cmd="$exe ${c_arg} -s ${S} -n ${D} -N ${N}"
cmd_time="/usr/bin/time -f '[TimeStats]\nAvgCPU=\"%P\"\nMaxMem=%M' -a -o"
echo "cmd=$cmd"

# Arg 1: role (0 or 1)
run-role() {
  out="${dir}/run.${1}"
  rout="${rdir}/run.${1}"
  echo "Starting remote queries for role $1 -> ${out}_r*"
  for r in $(seq $R); do
    echo "********** Run ${r} of ${R} **********"
    out1="${out}_r${r}"
    rout1="${rout}_r${r}"
    ssh -l ${ssh_user} ${remote[$1]} \
      "cd ${exepath}; ${cmd_time} ${rout1} $cmd -r $1 -S ${serverip} -B ${rout1}; cat ${rout1}" \
      | tee ${out1}
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

echo "********** Done. You may cleanup ~/tmp/runs/ on remotes **********"

# Weirdly, sometimes time prints the string 'Command terminated by signal 13',
# so we have to manually delete it afterward :( E.g. with
# $ sed -i -e '/Command terminated by signal 13/d' *
# TK: Signal 13 means "broken pipe", i.e. something is written to a pipe
# where nothing is read from anymore (e.g. see
# http://people.cs.pitt.edu/~alanjawi/cs449/code/shell/UnixSignals.htm).
