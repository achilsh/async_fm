#include "alarm_push.h"


int main(int argc, char**argv) {
  AlarmPush* pPush = new AlarmPush();
  pPush->Run(argc, argv);

  return 0;
}
