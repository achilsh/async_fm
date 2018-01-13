/*******************************************************************************
* Project:  AsyncServer
* @file     main.cpp
* @brief 
* @author   
* @date:    2015年8月6日
* @note
* Modify history:
******************************************************************************/

#include "unix_util/proctitle_helper.h"
#include "labor/duty/OssManager.hpp"

int main(int argc, char* argv[])
{
    signal(SIGPIPE, SIG_IGN);
    if (argc != 2)
    {
        std::cerr << "para num error!" << std::endl;
        exit(-1);
    }
	ngx_init_setproctitle(argc, argv);
    oss::OssManager oManager(argv[1]);
    oManager.Run();
	return(0);
}
