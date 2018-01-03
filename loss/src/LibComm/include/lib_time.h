#ifndef __LIB_TIME_H__
#define __LIB_TIME_H__ 

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <map>
#include <set>

using namespace std;
#include "json/json.h"
#include "lib_string.h"
//
#ifndef likely
#define likely(x)   __builtin_expect(!!(x),1)
#endif

#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x),0)
#endif

namespace LIB_COMM {

template<class T>
static  std::string ToString(const T& t) {
  std::stringstream ios;
  ios << t ;
  return ios.str();
}
//
class LibTime {
 public:
  //是否闰年
  static inline bool LeapYear(uint32_t year)
  {
    return year%400 == 0 || (year%100 != 0 && year%4 == 0);
  }

  static int32_t IsLastDayOfMonth(time_t t) {
    struct tm st_time;
    if (unlikely(NULL == localtime_r(&t, &st_time))) {
      return -1;
    }
    int32_t today_month = st_time.tm_mon;//当前所在月份
    t = t + 86400;
    if (unlikely(NULL == localtime_r(&t, &st_time))) {
      return -2;
    }
    int32_t tomorrow_month = st_time.tm_mon;//明天所在月份
    if (likely(today_month == tomorrow_month)) {//今天和明天是同一个月,则今天不是本月最后一天
      return -3;
    }
    return 0;//今天是本月最后一天
  }

  static time_t GetFirstDayOfMonth(time_t tick)
  {
    struct tm st_time;
    localtime_r(&tick, &st_time);
    st_time.tm_sec = 0;
    st_time.tm_min = 0;
    st_time.tm_hour = 0;
    st_time.tm_mday = 1;

    return mktime(&st_time);
  }

  static int32_t TruncDay(time_t src,time_t& dst)
  {
    struct tm st_time;
    if (unlikely(NULL == localtime_r(&src, &st_time))) {
      return -1;
    }
    st_time.tm_hour = 0;
    st_time.tm_min = 0;
    st_time.tm_sec = 0;
    dst = mktime(&st_time);
    if (unlikely((time_t)-1 == dst)) {
      return -2;
    }
    return 0;
  }

  static time_t TruncDay(time_t t)
  {
    struct tm st_time;
    localtime_r(&t,&st_time);
    st_time.tm_hour = 0;
    st_time.tm_min = 0;
    st_time.tm_sec = 0;

    return mktime(&st_time);
  }

  static int32_t GetInterestDays(time_t begin, time_t end)
  {
    return ((TruncDay(end) - TruncDay(begin)) / 86400 + 1);
  }

  static int32_t AddDay(int32_t days,struct tm& t)
  {
    time_t unix_time = mktime(&t);
    if (unlikely(unix_time == -1))//invalid tm value
      return -1;
    unix_time += 3600*24;
    localtime_r(&unix_time, &t);
    return 0;
  }
  //向前或向后多天
  static inline int32_t AddDay(time_t src, int32_t days, time_t& dst)
  {
    int32_t kSecondOfDay = 24 * 3600;
    dst = src + kSecondOfDay * days;
    if (unlikely(dst < 0))
      return -1;
    return 0;
  }

  static inline time_t AddDay(time_t src, int32_t days)
  {
    return src + 24 * 3600 * days;
  }

  //向前或向后多月
  static int32_t AddMonth(const struct tm& src, int32_t months,
                          struct tm& dst)
  {
    int32_t kDayOfMonth[13] = {29,31,28,31,30,31,30,31,31,30,31,30,31};
    int32_t total = src.tm_year*12+src.tm_mon+months;
    if (total < 0)
      return -1;

    memcpy(&dst, &src, sizeof dst);

    dst.tm_year = total/12;
    dst.tm_mon = total%12;

    int32_t pos = LeapYear(dst.tm_year)? 0: (dst.tm_mon+1);

    dst.tm_mday = src.tm_mday<kDayOfMonth[pos]? src.tm_mday: kDayOfMonth[pos];

    return 0;
  }

  static int32_t AddMonth(time_t src, int32_t months, time_t& dst)
  {
    struct tm tm_src = {0};
    struct tm tm_dst = {0};

    if (NULL == localtime_r(&src, &tm_src))
      return -1;

    if (0 != AddMonth(tm_src, months, tm_dst))
      return -1;

    dst = mktime(&tm_dst);

    return 0;
  }

  static int32_t AddMonthYingzt(time_t src, int32_t months, time_t& dst)
  {
    time_t tmp_day;
    tmp_day = src - 86400;
    struct tm *tt;
    tt = localtime(&tmp_day);
    if(tt->tm_mday > 28)
    {
      tt->tm_mday = 28;
      tmp_day = mktime(tt);
    }

    int32_t total = tt->tm_year * 12 + tt->tm_mon + months;
    tt->tm_year = total / 12;
    tt->tm_mon = total % 12;

    dst = mktime(tt);
    return 0;
  }

  //获取某个时间月份的天数
  static int32_t GetMonthDay(time_t src, int32_t& month_day)
  {
    struct tm tm_src = {0};
    if (NULL == localtime_r(&src, &tm_src))
      return -1;

    int32_t kDayOfMonth[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    month_day = LeapYear(tm_src.tm_year) && tm_src.tm_mon == 1 ? 29 : kDayOfMonth[tm_src.tm_mon];

    return 0;
  }

  //向前或向后多年
  static inline int32_t AddYear(const struct tm& tm_src, int32_t years,
                                struct tm& tm_dst)
  {
    return AddMonth(tm_src, years*12, tm_dst);
  }

  static inline int32_t AddYear(time_t src, int32_t years, time_t& dst)
  {
    return AddMonth(src, years*12, dst);
  }

  static int32_t StrDate8(const struct tm& t,std::string& str_date)
  {
    char buf[64] = {0};
    size_t len = strftime(buf,sizeof(buf),"%Y%m%d",&t);
    str_date.assign(buf, len);
    return 0;
  }
  static int32_t StrDate8(time_t t,std::string& str_date)//[YYYYmmdd]
  {
    struct tm* p = localtime(&t);
    if (NULL == p)
      return -1;
    char buf[64] = {0};
    size_t len = strftime(buf,sizeof(buf),"%Y%m%d",p);
    str_date.assign(buf, len);
    return 0;
  }

  static string StrDate8(time_t t)//[YYYYmmdd]
  {
    string str_date;
    struct tm* p = localtime(&t);
    if (NULL == p)
      return "";

    char buf[64] = {0};
    size_t len = strftime(buf,sizeof(buf),"%Y%m%d",p);
    str_date.assign(buf, len);

    return str_date;
  }

  static int32_t StrDate8Ch(time_t t,std::string& str_date)//[YYYY年mm月dd日]
  {
    struct tm* p = localtime(&t);
    if (NULL == p)
      return -1;
    char buf[64] = {0};
    size_t len = strftime(buf,sizeof(buf),"%Y年%m月%d日",p);
    str_date.assign(buf, len);
    return 0;
  }

  static string StrDate8Ch(time_t t)//[YYYY年mm月dd日]
  {
    struct tm* p = localtime(&t);
    if (NULL == p)
      return "";
    char buf[64] = {0};
    size_t len = strftime(buf,sizeof(buf),"%Y年%m月%d日",p);

    string str_date;
    str_date.assign(buf, len);

    return str_date;
  }

  static string StrDate6Ch(time_t t)//[YYYY年mm月]
  {
    struct tm* p = localtime(&t);
    if (NULL == p)
      return "";
    char buf[64] = {0};
    size_t len = strftime(buf,sizeof(buf),"%Y年%m月",p);

    string str_date;
    str_date.assign(buf, len);

    return str_date;
  }

  static int32_t UnStrDate8(const std::string& str_time,time_t& t)//[YYYYmmdd]
  {
    if (str_time.size() != 8)
      return -1;

    struct tm tm_date;
    memset(&tm_date,0,sizeof tm_date);
    tm_date.tm_year = atoi(str_time.substr(0,4).c_str())-1900;
    tm_date.tm_mon = atoi(str_time.substr(4,2).c_str())-1;
    tm_date.tm_mday = atoi(str_time.substr(6,2).c_str());

    t = mktime(&tm_date);
    if (time_t(-1) == t)
      return -2;
    return 0;
  }

  static int32_t UnStrDate6(const std::string& str_time,time_t& t)//[YYYY-m-d] or [YYYY-mm-dd]
  {
    struct tm tm_date;
    memset(&tm_date,0,sizeof tm_date);

    if(str_time.find('-') == string::npos)
      return -1;

    std::size_t first_slit = str_time.find_first_of('-');
    std::size_t last_split = str_time.find_last_of('-');

    tm_date.tm_year = atoi(str_time.substr(0,first_slit).c_str())-1900;
    tm_date.tm_mon = atoi(str_time.substr(first_slit+1,last_split-first_slit).c_str())-1;
    tm_date.tm_mday = atoi(str_time.substr(last_split+1).c_str());

    t = mktime(&tm_date);
    if (time_t(-1) == t)
      return -2;
    return 0;
  }

  static int32_t UnStrDate6BySeg(const std::string& str_time, char seg, time_t& t)//[YYYYsegmsegd]
  {
    struct tm tm_date;
    memset(&tm_date,0,sizeof tm_date);

    if(str_time.find(seg) == string::npos)
      return -1;

    std::size_t first_slit = str_time.find_first_of(seg);
    std::size_t last_split = str_time.find_last_of(seg);

    tm_date.tm_year = atoi(str_time.substr(0,first_slit).c_str())-1900;
    tm_date.tm_mon = atoi(str_time.substr(first_slit+1,last_split-first_slit).c_str())-1;
    tm_date.tm_mday = atoi(str_time.substr(last_split+1).c_str());

    t = mktime(&tm_date);
    if (time_t(-1) == t)
      return -2;
    return 0;
  }

  static int32_t UnStrCommDate8(const std::string& str_time,time_t& t)//[YYYYmdd or YYYYmmdd]
  {
    struct tm tm_date;
    memset(&tm_date,0,sizeof tm_date);
    tm_date.tm_year = atoi(str_time.substr(0,4).c_str())-1900;
    if(str_time.size() == 8)
    {
      tm_date.tm_mon = atoi(str_time.substr(4,2).c_str())-1;
      tm_date.tm_mday = atoi(str_time.substr(6,2).c_str());
    }
    else
    {
      tm_date.tm_mon = atoi(str_time.substr(4,1).c_str())-1;
      tm_date.tm_mday = atoi(str_time.substr(5,2).c_str());
    }

    t = mktime(&tm_date);
    if (time_t(-1) == t)
      return -2;
    return 0;
  }

  static int32_t CurrentDate8(std::string& str_date)//YYYYmmdd
  {
    time_t now = time(NULL);
    return StrDate8(now, str_date);
  }

  static int32_t StrDate10(time_t t,std::string& str_date)//[YYYY-mm-dd]
  {
    struct tm* p = localtime(&t);
    if (NULL == p)
      return -1;
    char buf[64] = {0};
    size_t len = strftime(buf,sizeof(buf),"%Y-%m-%d",p);
    str_date.assign(buf, len);
    return 0;
  }

  static string StrDate10(time_t t)//[YYYY-mm-dd]
  {
    string str_date;
    struct tm* p = localtime(&t);
    if (NULL == p)
      return "";

    char buf[64] = {0};
    size_t len = strftime(buf,sizeof(buf),"%Y-%m-%d",p);
    str_date.assign(buf, len);

    return str_date;
  }

  static int32_t CurrentDate10(std::string& str_date)//YYYY-mm-dd
  {
    time_t now = time(NULL);
    return StrDate10(now, str_date);
  }

  static string CurrentDate10()
  {
    return StrDate10(time(NULL));
  }

  static int32_t StrTime14(time_t t,std::string& str_time)//[YYYYmmddHHMMSS]
  {
    struct tm* p = localtime(&t);
    if (NULL == p)
      return -1;
    char buf[64] = {0};
    size_t len = strftime(buf,sizeof(buf),"%Y%m%d%H%M%S",p);
    str_time.assign(buf, len);
    return 0;
  }

  static string StrTime14(time_t t)//[YYYYmmddHHMMSS]
  {
    struct tm* p = localtime(&t);
    if (NULL == p)
      return "";

    char buf[64] = {0};
    size_t len = strftime(buf,sizeof(buf),"%Y%m%d%H%M%S",p);
    string str_time;
    str_time.assign(buf, len);

    return str_time;
  }

  static int32_t UnStrTime14(const std::string& str_time,time_t& t)//[YYYYmmddHHMMSS]
  {
    if (str_time.size() != 14)
      return -1;

    struct tm tm_time;
    tm_time.tm_year = atoi(str_time.substr(0,4).c_str())-1900;
    tm_time.tm_mon = atoi(str_time.substr(4,2).c_str())-1;
    tm_time.tm_mday = atoi(str_time.substr(6,2).c_str());
    tm_time.tm_hour = atoi(str_time.substr(8,2).c_str());
    tm_time.tm_min = atoi(str_time.substr(10,2).c_str());
    tm_time.tm_sec = atoi(str_time.substr(12,2).c_str());

    t = mktime(&tm_time);
    if (time_t(-1) == t)
      return -2;
    return 0;
  }

  static int32_t UnStrTime14V2(const std::string& str_time,time_t& t)//[YYYY-mm-dd HH:MM:SS]
  {
    if (str_time.size() != 19)
      return -1;

    struct tm tm_time;
    tm_time.tm_year = atoi(str_time.substr(0,4).c_str())-1900;
    tm_time.tm_mon = atoi(str_time.substr(5,2).c_str())-1;
    tm_time.tm_mday = atoi(str_time.substr(8,2).c_str());
    tm_time.tm_hour = atoi(str_time.substr(11,2).c_str());
    tm_time.tm_min = atoi(str_time.substr(14,2).c_str());
    tm_time.tm_sec = atoi(str_time.substr(17,2).c_str());

    t = mktime(&tm_time);
    if (time_t(-1) == t)
      return -2;
    return 0;
  }

  static int32_t CurrentTime14(std::string& str_time)//YYYYmmddHHMMSS
  {
    time_t now = time(NULL);
    return StrTime14(now, str_time);
  }

  static int32_t StrTime19(time_t t,std::string& str_time)//[YYYY-mm-dd HH:MM:SS]
  {
    struct tm* p = localtime(&t);
    if (NULL == p)
      return -1;
    char buf[64] = {0};
    size_t len = strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M:%S",p);
    str_time.assign(buf, len);
    return 0;
  }

  static string StrTime19(time_t t)
  {
    struct tm* p = localtime(&t);
    if (NULL == p)
      return "";

    char buf[64] = {0};
    size_t len = strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M:%S",p);

    string str_time;
    str_time.assign(buf, len);

    return str_time;
  }

  static int32_t CurrentTime19(std::string& str_time)//[YYYY-mm-dd HH:MM:SS]
  {
    time_t now = time(NULL);
    return StrTime19(now, str_time);
  }

  static string CurrentTime19()//[YYYY-mm-dd HH:MM:SS]
  {
    time_t now = time(NULL);
    return StrTime19(now);
  }


  static string TimeToStr(time_t t = 0)
  {
    std::string str;
    StrTime19(t, str);

    return str;
  }

  static int32_t NextTimePoint(int32_t hour, int32_t min, int32_t second, time_t& next_time)
  {
    time_t now = time(NULL);
    struct tm st_now = {0};
    localtime_r(&now, &st_now);
    int32_t delta = 0;
    if (st_now.tm_hour*3600 + st_now.tm_min*60 + st_now.tm_sec > hour*3600 + min*60 + second) {//指定时刻已过,下个时刻在明天
      delta = 86400;
    }
    st_now.tm_hour  = hour;
    st_now.tm_min   = min;
    st_now.tm_sec   = second;
    next_time = mktime(&st_now) + delta;
    return 0;
  }

  static int32_t DayOfWeek(time_t unix_time, int32_t& day_of_week)
  {
    struct tm st_tm = {0};
    if (NULL == localtime_r(&unix_time, &st_tm))
      return -1;
    day_of_week = st_tm.tm_wday;
    return 0;
  }

  static int32_t IsWorkingDay(time_t unix_time,const Json::Value &holiday,const Json::Value &working_day)
  {
    /******************************************************
    holiday : 国家法定假期
    {
      "2015-1":[1,2,3],//1月份1,2,3号放元旦假
      "2015-2":[18,19,20,21,22,23,24],
      "2015-4":[4,5,6],"5":[1,2,3],
      "2015-6":[20,21,22],
      "2015-9":[26,27],
      "2015-10":[1,2,3,4,5,6,7]
    }
    working_day : 法定假期的周末调休
    {
      "2015-1":[4],//元旦假期调休
      "2015-2":[15,28],
      "2015-10":[10]
    }
    *****************************************************/
    struct tm st_tm = {0};
    if (NULL == localtime_r(&unix_time, &st_tm))
    {
      return -1;
    }

    int year = st_tm.tm_year + 1900;
    int mon = st_tm.tm_mon + 1;
    int mon_day = st_tm.tm_mday;
    int week_day = st_tm.tm_wday;

    std::string str_key = ToString(static_cast<long long>(year)) + "-" + ToString(static_cast<long long>(mon));

    for(uint32_t i = 0; i < holiday[str_key].size(); ++i)
    {
      if(mon_day == holiday[str_key][i].asInt())
      {
        /*节假日*/
        return 0;
      }
    }

    if(week_day == 6 || week_day == 0)
    {
      for(uint32_t i = 0; i < working_day[str_key].size(); ++i)
      {
        if(mon_day == working_day[str_key][i].asInt())
        {
          /*周末调休*/
          return 1;
        }
      }

      /*正常周末*/
      return 0;
    }

    /*正常工作日*/
    return 1;
  }

/*注意此接口 : 如果当天是工作日,当天也计算在内*/
static int32_t NextNumWorkingDay(time_t unix_time,const Json::Value &holiday,const Json::Value &working_day,time_t &next_working_day,int n)
{
  time_t day_time = TruncDay(unix_time);
  while(n)
  {
    int ret = IsWorkingDay(day_time,holiday,working_day);
    if(ret == -1)
    {
      return -1;
    }

    if(ret == 1)
    {
      /*是工作日*/
      n--;
    }

    if(n) day_time += 86400;
  }

  next_working_day = day_time;
  return 0;
}

/*注意此接口 : 当天不参与判断*/
static int32_t NextNumWorkingDayWithoutToday(time_t unix_time,const Json::Value &holiday,const Json::Value &working_day,time_t &next_working_day,int n)
{
  time_t day_time = LIB_COMM::LibTime::TruncDay(unix_time);
  while(n)
  {
    if(n) day_time += 86400;
    int ret = LIB_COMM::LibTime::IsWorkingDay(day_time,holiday,working_day);
    if(ret == -1)
    {
      return -1;
    }

    if(ret == 1)
    {
      /*是工作日*/
      n--;
    }
  }

  next_working_day = day_time;
  return 0;
}

static bool IsTradeDay2015(int64_t time_point)
{
  /*
     2015-06-22
     2015-10-01
     2015-10-02
     2015-10-05
     2015-10-06
     2015-10-07
     */
  static std::set<long> trade_day_set;
  if (trade_day_set.empty()) {
    long trade_day = 0;
    UnStrDate8("20150101", trade_day);trade_day_set.insert(trade_day);
    UnStrDate8("20150102", trade_day);trade_day_set.insert(trade_day);
    UnStrDate8("20150218", trade_day);trade_day_set.insert(trade_day);
    UnStrDate8("20150219", trade_day);trade_day_set.insert(trade_day);
    UnStrDate8("20150220", trade_day);trade_day_set.insert(trade_day);
    UnStrDate8("20150223", trade_day);trade_day_set.insert(trade_day);
    UnStrDate8("20150224", trade_day);trade_day_set.insert(trade_day);
    UnStrDate8("20150406", trade_day);trade_day_set.insert(trade_day);
    UnStrDate8("20150501", trade_day);trade_day_set.insert(trade_day);
    UnStrDate8("20150622", trade_day);trade_day_set.insert(trade_day);
    UnStrDate8("20151001", trade_day);trade_day_set.insert(trade_day);
    UnStrDate8("20151002", trade_day);trade_day_set.insert(trade_day);
    UnStrDate8("20151005", trade_day);trade_day_set.insert(trade_day);
    UnStrDate8("20151006", trade_day);trade_day_set.insert(trade_day);
    UnStrDate8("20151007", trade_day);trade_day_set.insert(trade_day);
  }
  long  date_point = 0;
  TruncDay(time_point, date_point);
  if (trade_day_set.end() != trade_day_set.find(date_point))//match holiday
    return false;

  int32_t day_week = -1;
  DayOfWeek(time_point, day_week);
  if (0 == day_week || 6 == day_week)//weekend
    return false;

  return true;
}

static int32_t PreNumWorkingDayNoToday(time_t unix_time,const Json::Value &holiday,const Json::Value &working_day,time_t &next_working_day,int n)
{
  /*不包括当天的前N个工作日*/
  time_t day_time = TruncDay(unix_time - 86400);
  while(n)
  {
    int ret = IsWorkingDay(day_time,holiday,working_day);
    if(ret == -1)
    {
      return -1;
    }

    if(ret == 1)
    {
      /*是工作日*/
      n--;
    }

    if(n) day_time -= 86400;
  }

  next_working_day = day_time;
  return 0;
}

static int32_t WorkingDayDiff(time_t start_time,time_t end_time,
                              const Json::Value &holiday,const Json::Value &working_day,
                              int32_t &diff)
{//工作日间隔,start_time和end_time当天也参与判断
  time_t start_day = 0,end_day = 0;
  int32_t flag = 0;
  if(start_time <= end_time)
  {
    start_day = TruncDay(start_time);
    end_day = TruncDay(end_time);
  }
  else
  {
    start_day = TruncDay(end_time);
    end_day = TruncDay(start_time);
    flag = 1;
  }

  //LOG_INFO("start_day = %ld,end_day = %ld",start_day,end_day);
  diff = 0;
  for(time_t tmp = start_day; tmp <= end_day; tmp += 86400)
  {
    //string str_tmp;
    //StrDate8(tmp,str_tmp);
    //LOG_INFO("tmp = %s",str_tmp.c_str());
    int32_t ret = IsWorkingDay(tmp,holiday,working_day);
    if(ret == -1)
    {
      return -1;
    }

    if(ret == 1)
    {
      diff++;
    }
  }

  if(flag) diff = 0 - diff;
  return 0;
}

static time_t str19ToTime(const char *str_time)
{
  struct tm* tmp_time = (struct tm*)malloc(sizeof(struct tm));
  strptime(str_time,"%Y-%m-%d %H:%M:%S", tmp_time);
  time_t t = mktime(tmp_time);
  free(tmp_time);
  return t;
};

static time_t str14ToTime(const char *str_time)
{
  struct tm* tmp_time = (struct tm*)malloc(sizeof(struct tm));
  strptime(str_time,"%Y%m%d%H%M%S", tmp_time);
  time_t t = mktime(tmp_time);
  free(tmp_time);
  return t;
};

static time_t str8ToTime(const char *str_time)
{
  struct tm* tmp_time = (struct tm*)malloc(sizeof(struct tm));
  memset(tmp_time, 0, sizeof(struct tm));
  strptime(str_time,"%Y%m%d", tmp_time);
  time_t t = mktime(tmp_time);
  free(tmp_time);
  return t;
};

static time_t str10ToTime(const char *str_time)
{
  struct tm* tmp_time = (struct tm*)malloc(sizeof(struct tm));
  memset(tmp_time, 0, sizeof(struct tm));
  strptime(str_time,"%Y-%m-%d", tmp_time);
  time_t t = mktime(tmp_time);
  free(tmp_time);
  return t;
};

static int32_t GetCurrentHour()
{
  time_t tt = time(NULL);
  tm* t= localtime(&tt);
  return t->tm_hour;
}

static int32_t GetCurrentMinute()
{
  time_t tt = time(NULL);
  tm* t= localtime(&tt);
  return t->tm_min;
}
//
}; 
//
}
#endif 
