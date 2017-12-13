#ifndef _MODULE_QUERY_CNF_H_
#define _MODULE_QUERY_CNF_H_ 

#include <map>
#include "cmd/Module.hpp"
#include "CnfSrv_com.h"

#ifdef __cplusplus
extern "C" {
#endif
  oss::Cmd* create();
#ifdef __cplusplus
}
#endif

namespace CNF_SRV {
  class ModuleQueryCnf: public oss::Module {
    public:
     ModuleQueryCnf();
     virtual ~ModuleQueryCnf();
     virtual bool Init();
     virtual bool AnyMessage(
         const oss::tagMsgShell& stMsgShell,
         const HttpMsg& oInHttpMsg);
    private:
     void SendAck(int ino, const std::string serrmsg);
    private:
     oss::tagMsgShell m_tagMsgShell;
     HttpMsg m_oInHttpMsg;
     std::map<std::string, CnfSrvOp*> m_CnfSrvOp;
  };
}

#endif
