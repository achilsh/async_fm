#include "StringTools.hpp"

namespace loss {


bool StringTools::Split(const std::string& sSrc, const std::string& sDelim,
                        std::vector<std::string>& vsDst) {
  size_t last = 0;
  size_t index= sSrc.find_first_of(sDelim, last);
  while (index != std::string::npos) {
    vsDst.push_back(sSrc.substr(last, index-last));
    last = index + 1;
    index = sSrc.find_first_of(sDelim, last);
  }
  if (index - last > 0) {
    vsDst.push_back(sSrc.substr(last,index-last)); 
  }
  return true;
}
//
}
