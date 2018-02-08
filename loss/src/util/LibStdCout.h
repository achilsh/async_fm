#ifndef _STD_DEF_COUT_H_
#define _STD_DEF_COUT_H_

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <assert.h>

namespace loss
{
#ifdef DEBUG_TEST

#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

// 通用宏处理
#define DEBUG_LEVEL 1


//    
#define err_quit(format, args...)\
{\
    printf(format,##args);\
    printf("\n");\
    fflush(stdout);\
    exit(0);\
}

#define LOG(format, args...)\
{\
    time_t now = time(NULL);\
    char time_buf[32];\
    char filepath[256]={0};\
    char fileName[128]={0};\
    strftime(time_buf, 31, "%d/%m/%y %H:%M:%S", localtime(&now));\
    strncpy(filepath,__FILE__,256);\
    char *tokenPtr=strtok(filepath,"/");\
    while(tokenPtr != NULL) \
    {\
        memset(fileName,0,128);\
        strncpy(fileName,tokenPtr,strlen(tokenPtr));\
        tokenPtr=strtok(NULL,"/");\
    }\
    char tmpstr[1028];\
    snprintf(tmpstr,1028,format,##args);\
    printf("%s:[%s:%d] %s\n", time_buf, fileName, __LINE__,tmpstr);\
    fflush(stdout);\
}

#define FUNC_ENTRY LOG("%s entry", __FUNCTION__)
#define FUNC_END LOG("%s end", __FUNCTION__)

#define DEBUG_LOG(format, args...)\
{\
    time_t now = time(NULL);\
    char time_buf[32];\
    char filepath[256]={0};\
    char fileName[128]={0};\
    strftime(time_buf, 31, "%d/%m/%y %H:%M:%S", localtime(&now));\
    strncpy(filepath,__FILE__,256);\
    char *tokenPtr=strtok(filepath,"/");\
    while(tokenPtr != NULL) \
    {\
        memset(fileName,0,128);\
        strncpy(fileName,tokenPtr,strlen(tokenPtr));\
        tokenPtr=strtok(NULL,"/");\
    }\
    char tmpstr[1028];\
    snprintf(tmpstr,1028,format,##args);\
    printf("[DEBUG] %s:[%s:%d] %s\n", time_buf, fileName, __LINE__,tmpstr);\
    fflush(stdout);\
}

#define WARN_LOG(format, args...)\
{\
    time_t now = time(NULL);\
    char time_buf[32];\
    char filepath[256]={0};\
    char fileName[128]={0};\
    strftime(time_buf, 31, "%d/%m/%y %H:%M:%S", localtime(&now));\
    strncpy(filepath,__FILE__,256);\
    char *tokenPtr=strtok(filepath,"/");\
    while(tokenPtr != NULL) \
    {\
        memset(fileName,0,128);\
        strncpy(fileName,tokenPtr,strlen(tokenPtr));\
        tokenPtr=strtok(NULL,"/");\
    }\
    char tmpstr[1028];\
    snprintf(tmpstr,1028,format,##args);\
    printf("%s[WARN] %s:[%s:%d] %s%s\n", BOLDRED, time_buf, fileName, __LINE__,tmpstr,RESET);\
    fflush(stdout);\
}

#define ERROR_LOG(format, args...)\
{\
    time_t now = time(NULL);\
    char time_buf[32];\
    char filepath[256]={0};\
    char fileName[128]={0};\
    strftime(time_buf, 31, "%d/%m/%y %H:%M:%S", localtime(&now));\
    strncpy(filepath,__FILE__,256);\
    char *tokenPtr=strtok(filepath,"/");\
    while(tokenPtr != NULL) \
    {\
        memset(fileName,0,128);\
        strncpy(fileName,tokenPtr,strlen(tokenPtr));\
        tokenPtr=strtok(NULL,"/");\
    }\
    char tmpstr[1028];\
    snprintf(tmpstr,1028,format,##args);\
    printf("%s[ERROR] %s:[%s:%d] %s%s\n", BOLDRED, time_buf, fileName, __LINE__,tmpstr, RESET);\
    fflush(stdout);\
}

#else

#define err_quit(format, args...) {}
#define LOG(format, args...) {}
#define DEBUG_LOG(format, args...) {}
#define WARN_LOG(format, args...) {}
#define ERROR_LOG(format, args...) {}

#endif 

}   /**< loss namespace */

#endif

