#!/bin/bash

APP=fib
TASKS=fib._omp_fn.0,fib._omp_fn.1
CALLED_FUNCS=fib,fib_seq
INPUT="8 3"
MIR_CONF="-w=1 -i -g -p"
OPF="test"
PROCESS_TASK_GRAPH=1

# Profile and generate data
. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh
