/*******************************************************************************
 * Project:  InterfaceServer
 * @file     ModuleHttp.hpp
 * @brief 
 * @author   
 * @date:    2015Äê11ÔÂ19ÈÕ
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_MODULEHTTP_MODULEHTTP_HPP_
#define SRC_MODULEHTTP_MODULEHTTP_HPP_

#include "cmd/Module.hpp"

#ifdef __cplusplus
extern "C" {
#endif
oss::Cmd* create();
#ifdef __cplusplus
}
#endif

namespace im
{
  class StepTestQuery; 
	class ModuleHttp: public oss::Module
	{
	public:
		ModuleHttp();
		virtual ~ModuleHttp();

		virtual bool AnyMessage(
			const oss::tagMsgShell& stMsgShell,
			const HttpMsg& oInHttpMsg);
    void SendAck(const std::string sErr = "");
  private:
    StepTestQuery* pStepTQry;
    oss::tagMsgShell m_tagMsgShell;
    HttpMsg m_oInHttpMsg;
	};

} /* namespace im */

#endif /* SRC_MODULEHELLO_MODULEHELLO_HPP_ */
