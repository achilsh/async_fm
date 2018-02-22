#! /usr/bin/stap
# 这里核心的想法就是通过systemtap 找到malloc, realloc 返回的地址, 可以通过systemtap 里面的$return 来获得, 并记录.
# 然后再记录free 的时候是否对这些地址进行过free. 可以通过 $mem 来获得.

# 对某一个地址调用的malloc, free的次数. 
# 如果 = 0, 说明正常free掉, 
# 如果 = 1, 说明malloc, 但是还没被free
# 如果 > 1, 说明这个地址被多次给malloc返回给用户, 肯定不正常
# 如果 < 1, 说明这个地址被多次free 也就是我们常说的double free 问题
#

probe begin {
    printf("=========> begin probe mem leak <==========\n");
}

# 记录内存分配和释放的计数关联数组
global gMemRefTbl

# 记录内存分配和释放的调用堆栈关联数组
global gMemBtTbl

#用来记录上次操作的时间
global gTime

#
probe process("/lib64/libc.so.6").function("__libc_malloc").return,process("/lib64/libc.so.6").function("__libc_calloc").return {
if (target() == pid()) {
        gMemRefTbl[$return]++
        gMemBtTbl[$return] = sprint_ubacktrace()
        gTime[$return] = gettimeofday_s()
    }
}

probe process("/lib64/libc.so.6").function("__libc_free").call {
    if (target() == pid() && gTime[$mem] !=0) {
        gMemRefTbl[$mem]--
        if (gMemRefTbl[$mem] == 0) {
            if ($mem != 0) {
                printf("normal malloc and free\n")
                gMemBtTbl[$mem] = sprint_ubacktrace()
            }
        } else if (gMemRefTbl[$mem] < 0 && $mem != 0) {
            printf("-------------------------\n")
            printf("double free, call stack:[ %p ], ref: %d\n",$mem, gMemRefTbl[$mem])
            print_ubacktrace()
            printf("last time free backtrace: \n %s\n", gMemBtTbl[$mem])
            printf("-------------------------\n")
        }
    }
}

#定时打印内存泄漏地方
probe timer.s(5) {
    printf("===========> end probe mem leak <==========\n")  
    #输出产生泄漏的内存是在哪里分配的地方
    foreach(mem in gMemRefTbl) {
        if (gMemRefTbl[mem] > 0 && gettimeofday_s() - gTime[mem] > 10) {
            printf("\n\nref: %d, mem leak addr =>: %s\n\n", gMemRefTbl[mem], gMemBtTbl[mem])
        }
    }
}
