#!/bin/bash

APP=fib
OUTLINE_FUNCTIONS=ol_fib_0,ol_fib_1
CALLED_FUNCTIONS=fib,fib_seq
INPUT="42 12"
MIR_CONF="-w=1 -i -g -l=32"
OFILE_PREFIX="small-grain"
PROCESS_PROFILE_DATA=1

# Profile and generate data
. ${MIR_ROOT}/programs/common/stub-profile.sh
