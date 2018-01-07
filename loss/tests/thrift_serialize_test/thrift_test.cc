#include "thrift_serialize.h"
#include "hello_test_types.h"
#include <iostream>
using  namespace std;

using namespace Test;

int main() {
  OneTest t, r;
  t.__set_fOne("11111");

  std::string oStr;
  int iRet = ThrifSerialize<OneTest>::ToString(t,oStr);
  if (iRet != 0) {
    //std::cout << "serialize err: "  << ThrifSerialize<OneTest>::GetSerializeErrMsg() << std::endl;
    return 0;
  }
  iRet = ThrifSerialize<OneTest>::FromString(oStr,r);
  if (iRet != 0) {
    //std::cout << "serialize err: "  << ThrifSerialize<OneTest>::GetSerializeErrMsg() << std::endl;
    return 0;
  }
  std::cout << "reget value: " << r.fOne << std::endl;

  return 0;
}
