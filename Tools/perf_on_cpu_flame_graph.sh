#!/bin/bash
if [ $# -ne 2 ];then 
    echo "Usage: $0 seconds pid" 
    exit 1 
fi 
##
## url: https://github.com/brendangregg/FlameGraph
## 使用参考：http://www.udpwork.com/item/15962.html
##
perf record -g -p $2 -o on_perf.data & 

PID=`ps aux | grep "perf record" | grep -v grep | awk '{print $2}' ` 

if [ -n "$PID" ]; then 
    sleep $1 
    kill -s INT $PID 
fi 

# wait until perf exite 
sleep 1 

PL_TOOL_DIR=/home/achilsh/tools/FlameGraph/

perf script -i on_perf.data &> on_perf.unfold 
${PL_TOOL_DIR}/stackcollapse-perf.pl   on_perf.unfold &> on_perf.folded 
${PL_TOOL_DIR}/flamegraph.pl   on_perf.folded &> on_child_perf.svg
