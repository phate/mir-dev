#!/bin/bash

APP=bintree
OUTLINE_FUNCTIONS=ol_node_0,ol_node_1
CALLED_FUNCTIONS=fib,node
INPUT="4 20"
MIR_CONF="-w=1 -i -g -p"
OFILE_PREFIX="small"
PROCESS_PROFILE_DATA=1

# Profile and process data
. ${MIR_ROOT}/programs/common/stub-profile.sh
