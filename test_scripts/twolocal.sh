#!/bin/bash
source ./lib.sh

id1=${sel_id1:-tuda}
id2=${sel_id2:-dkfz}
port1=${sel_port1:-8161}
port2=${sel_port2:-8162}

local_init $id1 $port1
local_init $id2 $port2
remote_init $id2 $port1 localhost $port2
remote_init $id1 $port2 localhost $port1
sel_test_conn $id2 $port1
sel_test_ls $id2 $port1
sel_test_conn $id1 $port2
sel_test_ls $id1 $port2
match_record $id2 $port1
