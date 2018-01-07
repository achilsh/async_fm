#ifndef __THRIFT_SERIALIZE_H__
#define __THRIFT_SERIALIZE_H__

#include <iostream>
#include <sstream>
#include <string>
#include "protocol/TDebugProtocol.h"
#include "protocol/TBinaryProtocol.h"
#include "transport/TBufferTransports.h"
#include <string>

using namespace std;
using boost::shared_ptr;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

template<typename T>
class ThrifSerialize {
  public:
    static int ToString(T& tData, std::string& sData) {
      int iRet = 0;
      try {
        boost::shared_ptr<TMemoryBuffer> membuffer(new TMemoryBuffer());
        boost::shared_ptr<TBinaryProtocol> protocol(new TBinaryProtocol(membuffer));
        tData.write( protocol.get() );
        sData = membuffer->getBufferAsString();

      } catch (TException& ex) { 
        //m_ErrMsg = ex.what();
        iRet = -1;
      }
      return iRet;
    }

    static int FromString(const std::string& sData, T& tData) {
      int iRet = 0;
      do {
        if (sData.empty()) {
          iRet = -1;
          //m_ErrMsg = "input string is empty";
          break;
        }

        boost::shared_ptr<TMemoryBuffer>  membuffer(new TMemoryBuffer());
        boost::shared_ptr<TBinaryProtocol> protocol(new TBinaryProtocol(membuffer));

        try {
          membuffer->resetBuffer((uint8_t*)sData.data(), sData.size());
          tData.read( protocol.get() );

        } catch (TException& ex) {
          //m_ErrMsg = ex.what();
          iRet = -2;
          break;
        }
        //
      } while(0);

      return iRet;
    }
    /***
    static std::string GetSerializeErrMsg() {
      return m_ErrMsg;
    }
   static std::string m_ErrMsg;
   ***/ 
///
};

/***
template<typename T>
std::string ThrifSerialize<T>::m_ErrMsg = "";
***/
#endif
