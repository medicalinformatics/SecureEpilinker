#!/bin/bash
source ./lib.sh

id1=${sel_id1:-tuda}
id2=${sel_id2:-dkfz}
id3=${sel_id3:-charite}
port1=${sel_port1:-8161}
port2=${sel_port2:-8162}
port3=${sel_port3:-8163}

local_init $id1 $port1
local_init $id2 $port2
local_init $id3 $port3
remote_init $id2 $port1 127.0.0.1 $port2 # tuda/dkfz
remote_init $id1 $port2 127.0.0.1 $port1 # dkfz/tuda
remote_init $id3 $port1 127.0.0.1 $port3 # tuda/charite
remote_init $id1 $port3 127.0.0.1 $port1 # charite/tuda
sel_test_conn $id2 $port1
sel_test_conn $id1 $port2
sel_test_conn $id3 $port1
sel_test_conn $id1 $port3
match_record $id2 $port1 # tuda->dkfz
match_record $id1 $port3 # charite->tuda
