#!/bin/bash

APP=fib
TASKS=ol_fib_0,ol_fib_1
CALLED_FUNCS=fib,fib_seq
INPUT="10 5"
MIR_CONF="-w=1 -i -g"
OPF="small"
PROCESS_TASK_GRAPH=1

# Profile and generate data
. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh
