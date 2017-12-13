/**
 * @file: StringTools.h
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x01
 * @date: 2017-11-18
 */
#ifndef   STRING_TOOLS_HPP_
#define   STRING_TOOLS_HPP_

#include <string>
#include <vector>

namespace loss {
class StringTools {
  public:
    static bool Split(const std::string& sSrc, 
                      const std::string& sDelim,
                      std::vector<std::string>& vsDst); 


};

//
}
#endif
