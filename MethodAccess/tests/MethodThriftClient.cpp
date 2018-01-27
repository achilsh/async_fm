/**
 * @file: MethodThriftClient.cpp
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-01-23
 */

#ifndef __MethodThriftClient__
#define __MethodThriftClient__

#include "transport/TBufferTransports.h"
#include "transport/TTransportException.h"
#include "protocol/TBinaryProtocol.h"
#include "protocol/TProtocolException.h"
#include "transport/TSocket.h"
#include "transport/TTransport.h"
#include "protocol/TProtocol.h"
#include "thriftcommon.h"

#include "interface_test_types.h"
#include "demosvr.h"

using namespace Test;
//
using namespace std;
using boost::shared_ptr;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using apache::thrift::transport::TMemoryBuffer;
using apache::thrift::transport::TTransportException;
using apache::thrift::protocol::TProtocolException;
using apache::thrift::transport::TSocket;
using apache::thrift::transport::TTransport;
using apache::thrift::transport::TFramedTransport;
using apache::thrift::protocol::TBinaryProtocol;
using apache::thrift::protocol::TProtocol;


class MethodThriftTest 
{
    public:
     MethodThriftTest(const std::string& sIp, unsigned int uiPort);
     virtual ~MethodThriftTest();
     void LoopTest(int iTimes);

    private:
     void Run();    
    private:
     std::string m_sIp;
     unsigned int m_uiPort;

#ifdef SHORT_CONNECT
     //
#else
     boost::shared_ptr<Test::demosvrClient> client;
#endif

};

MethodThriftTest::MethodThriftTest(const std::string& sIp, unsigned int uiPort)
    :m_sIp(sIp), m_uiPort(uiPort)
{
#ifdef SHORT_CONNECT
    //
#else
    boost::shared_ptr<TSocket> socket(new TSocket(m_sIp.c_str(), m_uiPort));
    socket->setRecvTimeout(200);
    socket->setConnTimeout(500);
    boost::shared_ptr<TTransport> transport(new TFramedTransport(socket));
    boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
    client.reset(new Test::demosvrClient(protocol));
    transport->open();
#endif
}

MethodThriftTest::~MethodThriftTest() 
{

}

void MethodThriftTest::LoopTest(int iTimes)
{
    int tBegin = time(NULL);
    for (int i = 0; i < iTimes; ++i) 
    {
        Run();
        usleep(10000);
    }
    int tEnd = time(NULL);
    std::cout << "cost tm: " << tEnd - tBegin 
        <<", test times: " << iTimes 
        << ",one test per second: " << (1.0*iTimes) / (1.0*(tEnd - tBegin)) <<  std::endl;
}

void MethodThriftTest::Run()   
{
    try 
    {
#ifdef SHORT_CONNECT
        boost::shared_ptr<TSocket> socket(new TSocket(m_sIp.c_str(), m_uiPort));
        socket->setRecvTimeout(2000);
        socket->setConnTimeout(2000);
        boost::shared_ptr<TTransport> transport(new TFramedTransport(socket));
        boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        boost::shared_ptr<Test::demosvrClient> client(new Test::demosvrClient(protocol));
        transport->open();
#else
        //
#endif
        static int ii =  1111;
        ping pi;
        pi.a = ii++;
        pi.b = "22222";

        pang result;
        client->pingping(result,  pi, 8888, "demo test" );

        cout << "result a = " << result.a << " b = " << result.b << endl;
    }
    catch(const TException& ex)
    {
        cout << ex.what() << endl;
    }
    catch (...)
    {
        cout << "unknown error" << std::endl;
    }
}


int main()
{
    std::string ip = "192.168.1.106";
    unsigned int port = 24000;
    int inums = 10000;

    MethodThriftTest test(ip, port);
    test.LoopTest(inums);
}
/////
#endif
