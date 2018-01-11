/**
 * @file: base_timer.h
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-01-09
 */

#ifndef __BASE_TIMER_H__
#define __BASE_TIMER_H__

#include <stdlib.h>
#include <stdint.h>
#include "ev.h"

//using namespace loss;
namespace BASE_TIMER {

enum BASETIME_TMOUTRET {
  RET_TIMER_INIT = 0,
  RET_TIMER_RUNNING = 1,
  RET_TIMER_COMPLETE = 2,
};

class BaseTimer {
  public:
   BaseTimer(struct ev_loop* ptrLoop, ev_tstamp timerTm = 60.0);
   virtual ~BaseTimer();
   
   /** PreStart() && TimeOut() done by derive class ***/
   virtual bool PreStart() = 0;
   virtual BASETIME_TMOUTRET TimeOut() =  0;
   
   /** StartTimer() called client  ***/ 
   bool StartTimer();
  private:
   static void BaseTimerCallBack(struct ev_loop* loop, 
                                 struct ev_timer* watcher, int revents);
   bool TimeOutProc();
   ev_tstamp GetTimeout();  
  private:
    bool Init();
  protected:
   struct ev_loop* m_pLoop;
   
   ev_timer* m_pTimerWatcher;
   ev_tstamp m_TmoutTm;
   ev_tstamp m_curTm;
   //
   bool m_Init;
};

/////
}
//
#endif
