
#ifndef _THRIFT_COMMON_H_
#define _THRIFT_COMMON_H_

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
class ThriftUtil
{
    public:
        static int serialize(T& indata, string& bin)
        {
            int ret = 0;
            try
            {
                shared_ptr<TMemoryBuffer> membuffer(new TMemoryBuffer());
                shared_ptr<TBinaryProtocol> protocol(new TBinaryProtocol(membuffer));
                indata.write( protocol.get() );
                bin = membuffer->getBufferAsString();
            }
            catch(TException& e)
            {
                ret = -1;
            }   

            return ret;

        }
        static int deSerialize(const string& bin, T& outdata)
        {
            if(bin.size() == 0 )
            {
                return -1;
            }

            int ret = 0;
            shared_ptr<TMemoryBuffer>  membuffer(new TMemoryBuffer());
            shared_ptr<TBinaryProtocol> protocol(new TBinaryProtocol(membuffer));     
            try
            {
                membuffer->resetBuffer((uint8_t*)bin.data(), bin.size());
                outdata.read( protocol.get() );
            }
            catch(TException& e)
            {
                ret = -2;
            }       

            return ret;     
        }

        static string dump_msg(const T& msg)
        {
            string str;
            try
            {
                shared_ptr<TMemoryBuffer> membuffer(new TMemoryBuffer());
                shared_ptr<TDebugProtocol> protocol(new TDebugProtocol(membuffer));
                msg.write(protocol.get());
                str = membuffer->getBufferAsString();
            }
            catch(TException& e)
            {
                str = e.what();
            }

            return str;

        }
        static string dump_msg(const string& binmsg)
        {
            T msg;
            int32_t ret = deSerialize(binmsg, msg);
            if (0 != ret)
            {
                ostringstream sBuffer;
                sBuffer << "deSerialize error :" << ret;
                return sBuffer.str();
            }

            return dump_msg(msg);
        }
};

#endif
