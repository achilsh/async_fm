#!/bin/bash
if [ $# -ne 2 ];then 
    echo "Usage: $0 seconds pid" 
    exit 1 
fi 
##
## url: https://github.com/brendangregg/FlameGraph
##
##
perf record -g -p $2 -o perf.data & 

PID=`ps aux | grep "perf record" | grep -v grep | awk '{print $2}' ` 

if [ -n "$PID" ]; then 
    sleep $1 
    kill -s INT $PID 
fi 

# wait until perf exite 
sleep 1 

PL_TOOL_DIR=/home/achilsh/tools/FlameGraph/

perf script -i perf.data &> perf.unfold 
${PL_TOOL_DIR}/stackcollapse-perf.pl   perf.unfold &> perf.folded 
${PL_TOOL_DIR}/flamegraph.pl   perf.folded &> child_perf.svg
