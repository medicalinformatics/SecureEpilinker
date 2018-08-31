#!/bin/bash
source ./lib.sh

id=${sel_id:-tuda}
port=${sel_port:-8161}

local_init $id $port
remote_init $id $port
sel_test_conn $id $port
link_record $id $port

