#!/bin/bash

APP=jacobi
OUTLINE_FUNCTIONS=jacobi_block_wrapper,for_task_wrapper
CALLED_FUNCTIONS=jacobi_point,jacobi_block,for_task
INPUT="2048 64"
MIR_CONF="-w=1 -i -g -p"
OFILE_PREFIX="small"
PROCESS_PROFILE_DATA=1

. ${MIR_ROOT}/programs/common/stub-profile.sh
