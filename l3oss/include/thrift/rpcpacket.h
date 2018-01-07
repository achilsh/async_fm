#ifndef __THRIFT_XYZ_RPC_PACKET_H__
#define __THRIFT_XYZ_RPC_PACKET_H__ 

#include <string>
#include <string.h>
#include <arpa/inet.h>

#include "thrift/protocol/TBinaryProtocol.h"
#include "thrift/transport/TBufferTransports.h"
#include "thrift/transport/TSocket.h"

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

using namespace std;

#define THRIFT_TFRAME_HEAD_LEN	    4

namespace apache { namespace thrift {
	class rpcpacket
	{
		public:
			boost::shared_ptr<TMemoryBuffer>    m_transport;
			boost::shared_ptr<TBinaryProtocol>  m_protocol;	
			std::string							m_strFunc;
			int32_t								m_seqid;
			
		public:
			rpcpacket()
			{
				m_transport = boost::shared_ptr<TMemoryBuffer>(new TMemoryBuffer());
				m_protocol = boost::shared_ptr<TBinaryProtocol>(new TBinaryProtocol(m_transport));
				m_strFunc = "";
				m_seqid   = -1;
			};

		public: //unpack
			int32_t GetFunc(/*input*/char * pBuf, /*input*/ unsigned int len, std::string& strFunc)
			{
				//buffer检查
				if( pBuf == NULL || len <= THRIFT_TFRAME_HEAD_LEN )
				{
					return -1;
				}
				
				try
				{

					//去掉头四个字节
					m_transport.get()->resetBuffer((uint8_t*)(pBuf + THRIFT_TFRAME_HEAD_LEN), len - THRIFT_TFRAME_HEAD_LEN);
					
					TMessageType messageType;
					int32_t      seqid;

					m_protocol.get()->readMessageBegin(strFunc, messageType, seqid);

					if( strFunc.length() > 0 )
					{
						m_strFunc = strFunc;
						m_seqid   = seqid;
					}

					return 0;
				}
				catch(exception &e)
				{
					return  -2;

				}

				return  -3;				
			};

			template<typename T>
			int32_t GetParam(/*out*/ T& param)
			{
				try
				{
					param.read(m_protocol.get());
					m_protocol.get()->readMessageEnd();
					m_protocol.get()->getTransport()->readEnd();	

					return 0;
				}
				catch(exception &e)
				{
					return  -1;

				}

				return -2;
			};

		public://packet
			template<typename T>
			int32_t packet(/*input*/uint8_t * pBuf, /*input*/ const uint32_t len, T& result)
			{
				//buffer检查
				if( pBuf == NULL || len <= THRIFT_TFRAME_HEAD_LEN )
				{
					return -1;
				}	

				if(m_strFunc.size() == 0 || m_seqid == -1 )
				{
					return -2;
				}
				

				boost::shared_ptr<TMemoryBuffer>   transport = boost::shared_ptr<TMemoryBuffer>(new TMemoryBuffer());
				boost::shared_ptr<TBinaryProtocol> protocol  = boost::shared_ptr<TBinaryProtocol>(new TBinaryProtocol(transport));

				try
				{
					transport->resetBuffer();
					protocol->writeMessageBegin(m_strFunc, protocol::T_REPLY,  m_seqid);
					result.write(protocol.get());
					protocol->writeMessageEnd();
					protocol->getTransport()->writeEnd();
					protocol->getTransport()->flush();

					uint32_t   packlen = 0; 	
					uint8_t *  pTmpBuf = NULL;
					transport->getBuffer(&pTmpBuf, &packlen);
					if( len < packlen + THRIFT_TFRAME_HEAD_LEN  )
					{
						return -3;
					}
					
					int32_t tmplen = htonl(packlen);
					memcpy((char*)pBuf, (char*)(&tmplen), sizeof(tmplen));

					memcpy(pBuf + THRIFT_TFRAME_HEAD_LEN, pTmpBuf, packlen);

					return (packlen + THRIFT_TFRAME_HEAD_LEN);
				}
				catch(exception &e)
				{
					return  -4;

				}
				
				return  -5;
				
			}

	};
}}// apache::thrift
#endif
