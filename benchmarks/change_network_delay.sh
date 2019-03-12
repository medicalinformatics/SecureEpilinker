#!/bin/bash
#
# Run me like
# $ ./change_network_delay.sh phi1 eno1 100 
# to change the network delay of interface eno1 on host phi1 to 100 ms.
# If the given delay is 0 the network delay rule is removed.
# Unfortuantely this script asks for the remote user password twice, due to the
# usage of sude. Maybe this can be improved.

remote=${1:-"phi1"}
dev=${2:-"eno1"}
D=${3:-100}
ssh_user="kussel"
ssh -l ${ssh_user} -t ${remote} "sudo tc qdisc show dev eno1 | grep delay"
rc=$?

if [[ $rc -ne 0 ]]; then
  ssh -l ${ssh_user} -t ${remote} "sudo tc qdisc add dev ${dev} root handle 1:0 netem delay ${D}msec"
elif [[ $D -ne 0 ]]; then
  ssh -l ${ssh_user} -t ${remote} "sudo tc qdisc change dev ${dev} root handle 1:0 netem delay ${D}msec"
else
  ssh -l ${ssh_user} -t ${remote} "sudo tc qdisc del dev ${dev} root"
fi


