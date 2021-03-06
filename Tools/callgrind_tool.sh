#! /bin/bash 

BinRunFile=./test
ret=`valgrind --tool=callgrind  --separate-threads=yes  ${BinRunFile}`
echo $ret

`python gprof2dot.py -f callgrind -n10 -s callgrind.out.12345 > valgrind.dot`
`dot -Tpng valgrind.dot -o valgrind.png`

## mem leak check
## valgrind --tool=memcheck --log-file=   --trace-children=yes

#### or use others #####
##  用kcachegrind 打开 callgrind.out.12345 
##  LINUX： http://kcachegrind.sourceforge.net/html/Home.html
##  windows: https://sourceforge.net/projects/precompiledbin/files/?source=navbar
