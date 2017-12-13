#ifndef __LIB_STRING_H__
#define __LIB_STRING_H__
////////////////////////
#include <iostream>
#include <vector>
#include <set>
#include <math.h>
#include "lib_common.h"
#include <sstream>

#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost::algorithm;

typedef map<string, string> MapValue;
typedef vector<MapValue > VectorMap;

namespace LIB_COMM {
//
class LibString {

 public:
  static void str2vec(const std::string& src, std::vector<std::string>& vec,
                      const std::string seg = ",") {
    if(src.empty()) { return; }

    std::string str_tmp;
    size_t pos_start = 0, pos_end = 0;

    do {
      pos_end = src.find(seg, pos_start);
      if (pos_end == std::string::npos) {
        str_tmp = src.substr(pos_start);
      } else {
        str_tmp = src.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + seg.length();
      }

      vec.push_back(str_tmp);
    }while(pos_end != std::string::npos);

    return;
  }

  static void str2vec(const std::string& src, std::vector<int32_t>& vec,
                      const std::string seg = ",") {
    if(src.empty()) { return; }

    std::string str_tmp;
    size_t pos_start = 0, pos_end = 0;

    do {
      pos_end = src.find(seg, pos_start);
      if (pos_end == std::string::npos) {
        str_tmp = src.substr(pos_start);
      } else {
        str_tmp = src.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + seg.length();
      }

      int i = atoi(str_tmp.c_str());
      vec.push_back(i);
    }while(pos_end != std::string::npos);

    return;
  }

  static void str2vec(const std::string& src, std::vector<uint32_t>& vec,
                      const std::string seg = ",") {
    if(src.empty()) { return; }

    std::string str_tmp;
    size_t pos_start = 0, pos_end = 0;

    do {
      pos_end = src.find(seg, pos_start);
      if (pos_end == std::string::npos) {
        str_tmp = src.substr(pos_start);
      } else {
        str_tmp = src.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + seg.length();
      }

      uint32_t i = atoi(str_tmp.c_str());
      vec.push_back(i);
    }while(pos_end != std::string::npos);

    return;
  }

  static void str2vec(const std::string& src, std::vector<uint64_t>& vec,
                      const std::string seg = ",") {
    if(src.empty()) { return; }

    std::string str_tmp;
    size_t pos_start = 0, pos_end = 0;

    do {
      pos_end = src.find(seg, pos_start);
      if (pos_end == std::string::npos) {
        str_tmp = src.substr(pos_start);
      } else {
        str_tmp = src.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + seg.length();
      }

      uint64_t i = strtoul(str_tmp.c_str(), NULL, 10);
      vec.push_back(i);
    }while(pos_end != std::string::npos);

    return;
  }

  static void str2vec(const std::string& src, std::vector<float>& vec,
                      const std::string seg = ",") {
    if(src.empty()) { return; }

    std::string str_tmp;
    size_t pos_start = 0, pos_end = 0;

    do {
      pos_end = src.find(seg, pos_start);
      if(pos_end == std::string::npos) {
        str_tmp = src.substr(pos_start);
      } else {
        str_tmp = src.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + seg.length();
      }

      float i = atof(str_tmp.c_str());
      vec.push_back(i);
    }while(pos_end != std::string::npos);

    return;
  }

  static void str2set(const std::string& src, std::set<int32_t>& set_output,
                      const std::string seg = ",") {
    if(src.empty()) { return; }

    std::string str_tmp;
    size_t pos_start = 0, pos_end = 0;

    do {
      pos_end = src.find(seg, pos_start);
      if(pos_end == std::string::npos) {
        str_tmp = src.substr(pos_start);
      } else {
        str_tmp = src.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + seg.length();
      }

      int32_t i = atoi(str_tmp.c_str());
      set_output.insert(i);
    }while(pos_end != std::string::npos);

    return;
  }

  static void str2set(const std::string& src, std::set<uint64_t>& set_output,
                      const std::string seg = ",") {
    if(src.empty()) { return; }

    std::string str_tmp;
    size_t pos_start = 0, pos_end = 0;

    do {
      pos_end = src.find(seg, pos_start);
      if(pos_end == std::string::npos) {
        str_tmp = src.substr(pos_start);
      } else {
        str_tmp = src.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + seg.length();
      }

      uint64_t i = strtoul(str_tmp.c_str(),NULL,10);
      set_output.insert(i);
    }while(pos_end != std::string::npos);

    return;
  }

  static void str2set(const std::string& src, std::set<string>& set_output,
                      const std::string seg = ",") {
    if(src.empty()) { return; }

    std::string str_tmp;
    size_t pos_start = 0, pos_end = 0;

    do {
      pos_end = src.find(seg, pos_start);
      if(pos_end == std::string::npos) {
        str_tmp = src.substr(pos_start);
      } else {
        str_tmp = src.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + seg.length();
      }

      set_output.insert(str_tmp);
    }while(pos_end != std::string::npos);

    return;
  }

  static void str2map(const string& src, map<string, string>& map_output, 
                      const string seg = "=")
  {
    vector<string> vec_temp;
    str2vec(src, vec_temp, seg);
    map_output[vec_temp[0]] = vec_temp[1];

    return ;
  }

  static void str2map(const string& src, map<string, string>& map_output, 
                      const string& seg1, const string&  seg2)
  {
    vector<string> vec_temp;
    str2vec(src, vec_temp, seg1);

    for (unsigned i = 0; i < vec_temp.size(); ++i)
    {
      str2map(vec_temp[i], map_output, seg2);
    }

    return ;
  }

  template <class T>
  static void vec2str(const vector<T>& vec, string& src, const string seg = ",")
  {
    src.clear();
    if (0 == vec.size())
      return ;

    for (uint32_t i = 0; i < vec.size(); ++i)
      src += type2str(vec[i]) + seg;

    src = src.substr(0, src.length() - seg.length());

    return;
  }

  template <class T>
  static string vec2str(const vector<T>& vec, const string seg = ",")
  {
    if (0 == vec.size())
      return "";

    string src;
    for (uint32_t i = 0; i < vec.size(); ++i)
      src += type2str(vec[i]) + seg;

    src = src.substr(0, src.length() - seg.length());

    return src;
  }

  template <class T>
  static void set2str(const set<T>& set_data, string& src, const string seg = ",")
  {
    src.clear();
    if (0 == set_data.size())
    {
      return;
    }

    for (typename std::set<T>::const_iterator it = set_data.begin(); it != set_data.end(); ++it)
    {
      src += type2str(*it) + seg;
    }

    src = src.substr(0, src.length() - 1);
    return;
  }

  template <class T>
  static std::string type2str(T v) {
    std::stringstream s;
    s << v;

    return s.str();
  }

  template <class T>
  static std::string FormatString(T v, int num) {
    std::string str_output = LibString::type2str(v);
    int count = num - str_output.length();
    while (count-- > 0)
      str_output = "0" + str_output;

    return str_output;
  }

  static std::string FormatString(int64_t data, int num) {

    std::string str_output = LibString::type2str(data>0 ? data : -data);
    int count = num - str_output.length();
    while (count-- > 0)
    {
      if (data < 0 && count == 0)
        str_output = "-" + str_output;
      else
        str_output = "0" + str_output;
    }

    return str_output;
  }

  static std::string FormatString(int32_t data, int num) {    
    std::string str_output = LibString::type2str(data>0 ? data : -data);
    int count = num - str_output.length();
    while (count-- > 0)
    {
      if (data < 0 && count == 0)
        str_output = "-" + str_output;
      else
        str_output = "0" + str_output;
    }

    return str_output;
  }

  static void Replace(std::string& str,
                      const std::string& strSrc,
                      const std::string& strDst){
    std::string::size_type pos = 0;
    while((pos = str.find(strSrc, pos)) != std::string::npos) {
      str.replace(pos, strSrc.length(), strDst);
      pos += strDst.length();
    }

    return;
  }

  static std::string Base16Decode(const std::string strHex)
  {
    unsigned char c, s;
    std::string strResult;
    for (size_t i = 0; i < strHex.size(); i=i+2)
    {
      s = 0x20 | (strHex[i]);
      if( s >= '0' && s <='9')
      {
        c = s - '0';
      }
      else if(s >= 'a' && s <= 'f')
      {
        c = s - 'a' + 10;
      }
      else
      {
        break;
      }

      c <<=4;
      s = 0x20 | (strHex[i+1]);
      if ( s >= '0' && s <= '9')
      {
        c += s - '0';
      }
      else if ( s >= 'a' && s <= 'f')
      {
        c += s - 'a' + 10;
      }
      else
      {
        break;
      }

      strResult.push_back(c);
    }

    return strResult;
  }

  static std::string Base16Encode(const std::string strBin)
  {
    const char sHex[] = "0123456789abcdef";
    std::string strHex;
    strHex.reserve(strBin.size()*2);

    for (size_t i = 0; i < strBin.size(); ++i)
    {
      strHex.push_back(sHex[uint8_t(strBin[i]) >> 4]);
      strHex.push_back(sHex[uint8_t(strBin[i]) & 15]);
    }

    return strHex;
  }

  static void ParseParams(const std::string& params, MapValue& map_params, const std::string &str_spilt)
  {
    std::vector<std::string> vStr;

    split(vStr, params, is_any_of(str_spilt));

    for (size_t j = 0; j < vStr.size(); ++j)
    {
      size_t pos = vStr[j].find_first_of("=");
      map_params.insert(std::make_pair(vStr[j].substr(0,pos),vStr[j].substr(pos+1)));
    }
  }

  static std::string FormatValue(int64_t value, int32_t precision = 2)
  {
    int precision_value = 0;
    if (precision == 0)
    {
      return LibString::type2str(value);
    }
    else if (precision > 0 && precision < 20)
    {
      precision_value = pow(10, precision);
    }
    else
    {
      return "";
    }
    if (value % precision_value == 0)
    {
      return LibString::type2str(value / precision_value);
    }
    else
    {
      return LibString::type2str(value / precision_value) + "." + LibString::FormatString(value % precision_value, precision);
    }
    return "";
  }

  static std::string removeWhitespace(const std::string& s)
  {
    std::string result;
    for(unsigned int i = 0; i < s.length(); ++ i)
    {
      if(!isspace(static_cast<unsigned char>(s[i])))
      {
        result += s[i];
      }
    }
    return result;
  }

  static std::string toLower(const std::string& s)
  {
    std::string result;
    result.reserve(s.size());
    for(unsigned int i = 0; i < s.length(); ++i)
    {
      if(isascii(s[i]))
      {
        result += tolower(static_cast<unsigned char>(s[i]));
      }
      else
      {
        result += s[i];
      }
    }
    return result;
  }

  static std::string toUpper(const std::string& s)
  {
    string result;
    result.reserve(s.size());
    for(unsigned int i = 0; i < s.length(); ++i)
    {
      if(isascii(s[i]))
      {
        result += toupper(static_cast<unsigned char>(s[i]));
      }
      else
      {
        result += s[i];
      }
    }
    return result;
  }

  static std::string trim(const string& s)
  {
    static const string delim = " \t\r\n";
    static string::size_type beg = s.find_first_not_of(delim);
    if(beg == string::npos)
    {
      return "";
    }
    else
    {
      return s.substr(beg, s.find_last_not_of(delim) - beg + 1);
    }
  }

  static std::string urlEncode(const std::string& str)
  {
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
      if (isalnum((unsigned char)str[i]) ||
          (str[i] == '-') ||
          (str[i] == '_') ||
          (str[i] == '.') ||
          (str[i] == '~'))
        strTemp += str[i];
      else if (str[i] == ' ')
        strTemp += "+";
      else
      {
        strTemp += '%';
        strTemp += toHex((unsigned char)(str[i] >> 4));
        strTemp += toHex((unsigned char)(str[i] % 16));
      }
    }
    return strTemp;
  }

  static std::string urlDecode(const std::string& str)
  {
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
      if (str[i] == '+') strTemp += ' ';
      else if (str[i] == '%')
      {
        unsigned char high = fromHex((unsigned char)str[++i]);
        unsigned char low = fromHex((unsigned char)str[++i]);
        strTemp += high*16 + low;
      }
      else strTemp += str[i];
    }
    return strTemp;
  }

  static int32_t splitToVec(const char *sep, const std::string &str, 
                            std::vector<std::string> &vec)
  {
    vec.clear();

    if (NULL == sep || str.empty()) return -1;

    const char *s1 = str.c_str();
    const char *s2;
    int len = strlen(sep);
    int pos = vec.size() - 1;
    std::string nullString;

    vec.reserve(10);
    while ((s2 = strstr(s1, sep)))
    {   
      vec.push_back(nullString);
      vec[++pos].assign(s1, s2 - s1);
      s1 = s2 + len;
    }   
    vec.push_back(s1);

    return 0;
  }

  static int32_t splitToMap(const char *sep1, const char *sep2, 
                            const std::string &str, std::map<std::string, std::string> &map)
  {
    map.clear();

    if (NULL == sep1 || NULL == sep2 || str.empty()) return -1;

    const char *s1 = str.c_str();
    const char *s2;
    const char *s3;
    int len1 = strlen(sep1);
    int len2 = strlen(sep2);
    std::string key;

    while ((s2 = strstr(s1, sep1)))
    {
      s3 = strstr(s1, sep2);
      if (NULL == s3 || s2 < s3)
      {
        s1 = s2 + len1;

        return -1;
      }

      key.assign(s1, s3 - s1);
      map[key].assign(s3 + len2, s2 - s3 - len2);
      s1 = s2 + len1;
    }
    s3 = strstr(s1, sep2);
    if (NULL == s3) return -1;

    key.assign(s1, s3 - s1);
    map[key].assign(s3 + len2, strlen(s3) - len2);

    return 0;
  }

  static bool isNumStr(const std::string &str)
  {    
    int32_t len = str.length();
    const char *src = str.c_str();

    for (int i = 0; i < len; ++i) 
    {    
      if (!isdigit(src[i]))
        return false;
    } 

    return true;
  }

  static uint64_t murmurHash(const std::string &str)
  {
    return murmurHash((const uint8_t *)str.c_str(), str.length());
  }

  static uint64_t murmurHash(const uint8_t *key, int len)
  {
    uint64_t h, k;

    h = 0 ^ len;
    while (len >= 4) {
      k  = key[0];
      k |= key[1] << 8;
      k |= key[2] << 16; 
      k |= key[3] << 24; 

      k *= 0x5bd1e995;
      k ^= k >> 24; 
      k *= 0x5bd1e995;

      h *= 0x5bd1e995;
      h ^= k;

      key += 4;
      len -= 4;
    }   

    switch (len) {
      case 3:
        h ^= key[2] << 16; 
      case 2:
        h ^= key[1] << 8;
      case 1:
        h ^= key[0];
        h *= 0x5bd1e995;
    }   

    h ^= h >> 13; 
    h *= 0x5bd1e995;
    h ^= h >> 15; 

    return labs(h);
  }

  static std::string generateRandom(const std::string &seed, const int32_t len)
  {
    std::string random;
    uint64_t hash = murmurHash(seed);

    random.resize(len);
    char *buf = (char *)random.c_str();
    for (int i = 0; i < len; ++i)
    {
      if (hash > 0)
      {
        *buf++ = (char)(hash % 10 + 48);
        hash /= 10;
      }else
      {
        hash = murmurHash((uint8_t *)random.c_str(), i - 1);
      }
    }

    return random;
  }

  static void splitUtf8String(const std::string &str, std::vector<std::string> &list)
  {
    int32_t bytes = 0; 
    int32_t len = str.length();
    const char *str1 = str.c_str();
    std::string tmp; 

    list.clear();
    list.reserve(len);
    while (len > 0) 
    {    
      bytes = getUtf8Bytes((uint8_t)*(str1));
      tmp.assign(str1, bytes);
      str1 += bytes;
      len -= bytes;
      list.push_back(tmp);
    }    
  } 

 private:
  static int32_t getUtf8Bytes(const uint8_t firstByte)
  {
    int32_t bytes = 0; 
    uint8_t src = firstByte;

    while (src & 0x80)
    {    
      ++bytes;
      src <<= 1;
    }    

    return 0 == bytes ? 1 : bytes;
  }

  static inline unsigned char toHex(unsigned char x)
  {
    return  x > 9 ? x + 55 : x + 48;
  }

  static inline unsigned char fromHex(unsigned char x)
  {
    unsigned char y;
    if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
    else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
    else if (x >= '0' && x <= '9') y = x - '0';
    return y;
  }
};

}
#endif
