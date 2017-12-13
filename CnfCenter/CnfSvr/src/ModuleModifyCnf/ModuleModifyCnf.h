#ifndef  _MODULE_MODIFY_CNF_H_
#define _MODULE_MODIFY_CNF_H_

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
  class ModuleModifyCnf: public oss::Module {
    public:
      ModuleModifyCnf();
      virtual ~ModuleModifyCnf();
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
