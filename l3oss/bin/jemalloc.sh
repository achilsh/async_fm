#!/bin/sh

prefix=/work/imdev/IM3.0/l3oss
exec_prefix=/work/imdev/IM3.0/l3oss
libdir=${exec_prefix}/lib

LD_PRELOAD=${libdir}/libjemalloc.so.2
export LD_PRELOAD
exec "$@"
