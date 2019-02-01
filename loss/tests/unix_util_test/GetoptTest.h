#include "GetOptInterface.h"
#include <iostream>
using namespace std;

//////////////////////////////////////////
void Test_GetOpt(int argc, char** argv) {
    std::string shortOpts("nht:x::");

#if 0
    GetOpt* pOpt = new GetOpt(argc, argv, shortOpts);
    bool bRet    = pOpt->GetOptInterface();
    if (bRet == false) {
        std::cout << "err" << std::endl;
        delete pOpt;
        return 0;

    }

    std::cout << "opt: " << pOpt->GetNFlags() << ", " 
        << pOpt->GetX() << ", "
        << pOpt->GetTm() << std::endl;
    delete pOpt;

#else 
    std::cout << "---------------------------- " << std::endl;
    GetOptLong* pLongOpt = new GetOptLong(argc, argv, shortOpts);
    bool bRet    = pLongOpt->GetOptInterface();
    if (bRet == false) {
        std::cout << "err" << std::endl;
        delete pLongOpt;
        return 0;

    }

    std::cout << "LongOpt: " << pLongOpt->GetNFlags() << ", "
        << pLongOpt->GetX() << ", "
        << pLongOpt->GetTm() << std::endl;
    delete pLongOpt;
#endif

    return 0;
}
