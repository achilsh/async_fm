#include "GetOptInterface.h"
#include <iostream>


GetOptBase::GetOptBase(int argc, char **argv, const std::string& sShortOpt) 
                  :m_iArgc(argc), m_pArgv(argv), m_sShortOpts(sShortOpt), m_iOptVal(-1) {
    setbuf(stderr, NULL);
}

bool GetOptBase::GetOptInterface() {
    if (m_sShortOpts.empty()) {
        this->Usage();
        return false;
    }
    
    if ( m_iArgc <= 1 || m_pArgv == NULL) {
        this->Usage();
        return false;
    }

    if ( false == this->GetOptMethod() ) {
        this->Usage();
        return false;
    }
    return true;
}

/***************************************************************************/
GetOpt::GetOpt(int argc, char **argv, const std::string& sShortOpt) 
              :GetOptBase(argc, argv, sShortOpt), m_flagN(false), m_Tm(0) {
}

GetOpt::~GetOpt() {
}

bool GetOpt::GetOptMethod() {
    while( (m_iOptVal = getopt(m_iArgc, m_pArgv, m_sShortOpts.c_str())) != -1 ) {
        switch(m_iOptVal) {
            case 'n': 
                m_flagN = true; 
                break;

            case 't':
                if (optarg == NULL) {
                    return false;
                }
                m_Tm = atoi(optarg);
                break;

            case 'x':
                if ( optarg == NULL ) {
                    return false;
                }
                m_X = atoi(optarg);               
                break;

            case 'h':
                return false;

            default:
                return false;
        }
    }

    if (optind == 1) {
        printf("%d\n", optind);
        return false;
    }
    printf("optind: %d\n", optind);

    return true;
}


void GetOpt::Usage() {
    printf("Usage: \n");
    printf("%s\t\t-n\t\t\t\t\t\t\t\ttest flags n \n", m_pArgv[0]);
    printf("\t\t-t\t\t\t\tVal_time or -tVal_time\t\t\t\ttest time \n");
    printf("\t\t-xVal_x\t\t\t\ttest x \n");
    printf("\t\t-h\t\t\t\t\t\t\t\tprint help and exit\n");
}

/************************************************************************/
GetOptLong::GetOptLong(int argc, char **argv, 
                       const std::string& sShortOpt, 
                       int iLongOptNums )
                      : GetOptBase(argc, argv, sShortOpt), 
                        m_pLongOpts(NULL),
                        m_iLongOptNums(iLongOptNums),
                        m_nFlags(false), m_tVal(0), m_xVal(0), m_iOptIndex(0) {
   setbuf(stderr, NULL);
    
   m_pLongOpts = (struct option*)malloc(sizeof(struct option)*m_iLongOptNums);   
}

GetOptLong::~GetOptLong() {
    if (m_pLongOpts) {
        free(m_pLongOpts);
        m_pLongOpts = NULL;
    }
    m_iLongOptNums = 0;
}

bool GetOptLong::InitLongOpt() {
    if (m_pLongOpts == NULL) {
        return false;
    }

    int iIndex = 0;
    do {
        m_pLongOpts[iIndex++] = {"nflags", no_argument, 0, 'n'};
        if (iIndex >= m_iLongOptNums) {
            return false;
        }
    } while(0);
    
    do {
        m_pLongOpts[iIndex++] = {"help",   no_argument, 0, 'h'};
        if (iIndex >= m_iLongOptNums) {
            return false;
        }
    } while(0);

    do { 
        m_pLongOpts[iIndex++] = {"tflags", required_argument,0, 't'};
        if (iIndex >= m_iLongOptNums) {
            return false;
        }
    } while(0);

    do {
        m_pLongOpts[iIndex++] = {"xflags", optional_argument,0, 'x'};
        if (iIndex >= m_iLongOptNums) {
            return false;
        }
    } while(0);

    m_pLongOpts[iIndex++] = {0, 0, 0};
    return true;
}

bool GetOptLong::GetOptMethod() {
    if ( InitLongOpt()  == false )  {
        return false;
    }

    while ((m_iOptVal = getopt_long(m_iArgc, m_pArgv, 
                                    m_sShortOpts.c_str(),
                                    m_pLongOpts,
                                    &m_iOptIndex)) != -1) {
        switch(m_iOptVal) {
            case 'n':
                m_nFlags = true;
                break;
            case 'h':
                return false;

            case 't':
                if ( optarg == NULL ) {
                    return false;
                }
                m_tVal = atoi(optarg);
                break;

            case 'x':
                std::cout << "get x flags" << std::endl;
                if ( optarg != NULL ) {
                    m_xVal = atoi(optarg);
                }
                break;

            default:
                return false;
        }
    }
    return true;
}

void GetOptLong::Usage() {
    printf("Usage: \n");
    printf("%s\t\t-n, --nflags\t\t\t\t\t\t\t\t n_test_flag \n", m_pArgv[0]);
    printf("\t\t-t,   ---tflags=<t_val>[---tflags\tt_val]\t\t\t\t\t\t\t\tt_testval \n");
    printf("\t\t-x,   --xflags[--xflags=<x_val]\t\t\t\t\t\t\t\t\tx_testval\n");
    printf("\t\t-h,   --help\t\t\t\t\t\t\t\t\n");
}
