/**
 * @file: getopt_interface.h
 * @brief: 
 *
 * getopt() 使用:
 * char*optstring = "ab:c::";
 * 单个字符a          表示选项a没有参数            格式：-a即可，不加参数
 * 单字符加冒号b:     表示选项b有且必须加参数      格式：-b 100或-b100,但-b=100错
 * 单字符加2冒号c::   表示选项c可以有，也可以无     格式：-c200,  其它格式错误
 *
 * 过程中使用的全局变量:
 *  optarg —— 指向当前选项参数(如果有)的指针
 *  optind —— 再次调用 getopt() 时的下一个 argv指针的索引
 *
 *-----------------------------------------------------------------------------------
 *-----------------------------------------------------------------------------------
 * getopt_long相比getopt增加了长选项的解析,eg 增加解析长选项的功能如：--prefix --help
 * struct option {
 *    const char  *name;       参数名称 
 *    int          has_arg;    指明是否带有参数 
 *    int          *flag;      flag=NULL时,返回val;不为空时,*flag=val,返回0 
 *    int          val;        用于指定函数找到选项的返回值或flag非空时指定*flag的值
 * };
 * 
 * 其中 has_arg  指明是否带参数值,其数值可选: 
 *     no_argument:        表明长选项不带参数，如：--name, --help
 *     required_argument:  表明长选项必须带参数，如：--prefix /root或 --prefix=/root 
 *     optional_argument:  表明长选项的参数是可选的，如：--help或 --prefix=/root，其它都是错误
 *  
 *  函数参数，第三个参数为: longopts    指明了长参数的名称和属性
 *            第四个参数为: longindex   如果longindex非空，它指向的变量将记录当
 *            前找到参数符合longopts里的第几个元素的描述，即是longopts的下标值
 *
 *
 *
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2019-01-28
 */

#include <string>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

class GetOptBase {
 public:
  GetOptBase(int argc, char **argv, const std::string& sShortOpt = "");
  virtual ~GetOptBase() {}
  
  bool GetOptInterface();

 protected:
  virtual bool GetOptMethod() = 0;
  virtual void Usage() = 0;

  int         m_iArgc;
  char**      m_pArgv;
  std::string m_sShortOpts; 
  int         m_iOptVal;
};


/**
 * @brief: 定义一个解析具体参数的getopt接口类
 */
class GetOpt: public GetOptBase {
 public:
  //eg: shortopt data is "nht:x::" 
  GetOpt(int argc, char **argv, const std::string& sShortOpt = "");
  virtual ~GetOpt();

  
  bool GetNFlags() const { return m_flagN; }
  int  GetTm() const { return m_Tm; }
  int  GetX() const { return m_X; }

 protected:
  virtual bool GetOptMethod();
  virtual void Usage();


 private:
  bool m_flagN;
  int m_Tm;
  int m_X;
};


/**
 *
 * @brief:  定义一个解析具体参数的getopt_long接口类
 *
 */
class GetOptLong: public GetOptBase {
 public:
  //eg: shortopt data is "nht:x::" and long opt data is "--nflags or --help or
  //--tflags 11111 or --tflags=11111 or  --xflags=333333 or --xflags"
  GetOptLong(int argc, char **argv, const std::string& sShortOpt = "",  int iLongOptNums = 100);
  virtual ~GetOptLong(); 

  bool GetNFlags() { return m_nFlags; }
  int GetX()   { return m_xVal; }
  int GetTm()  { return m_tVal; }

 protected:
  virtual bool GetOptMethod();
  virtual void Usage();
  bool InitLongOpt();

 private:
  struct option* m_pLongOpts;
  int            m_iLongOptNums;

  bool           m_nFlags;
  int            m_tVal;
  int            m_xVal;

  int            m_iOptIndex;
};

