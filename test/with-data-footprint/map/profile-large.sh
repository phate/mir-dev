#!/bin/bash

APP=map
TASKS=map_wrapper,for_task_wrapper
CALLED_FUNCS=map,for_task
INPUT="384 128"
MIR_CONF="-w=1 -i -g -p"
OPF="large"
PROCESS_TASK_GRAPH=1

. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh
