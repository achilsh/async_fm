#! /bin/bash 
#
# install: Systemtap, url: http://openresty.org/en/build-systemtap.html
# install: kernel-devel 和 kernel-debuginfo,注意对应内核版本，用uname -r, eg: 2.6.32-696.20.1.el6.x86_64
# pl tool url:    https://github.com/openresty/openresty-systemtap-toolkit
#
if [ $# -ne 2 ];then 
    echo "Usage: $0 seconds pid" 
    exit 1 
fi 

PL_TOOL_DIR=/home/achilsh/tools/FlameGraph/
OFF_CPU_DIR=/home/achilsh/tools/openresty-systemtap-toolkit

${OFF_CPU_DIR}/sample-bt  -p $2 -t $1 -u -k > on_cpu_systab.bt
${PL_TOOL_DIR}/stackcollapse-stap.pl  on_cpu_systab.bt > on_cpu_systab.cbt
${PL_TOOL_DIR}/flamegraph.pl   on_cpu_systab.cbt > on_cpu_systab.svg
