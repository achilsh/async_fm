#include "base_task.h"
#include <iostream>
#include <unistd.h>
#include <stdlib.h>

using namespace std;
using namespace BASE_TASK;

class TestBaseTask: public BaseTask {
  public:
   TestBaseTask() {}
   virtual ~TestBaseTask() {}
   virtual int TaskInit(loss::CJsonObject& oJsonConf, uint32_t uiWkI);
   virtual int HandleLoop();

  private:
   int m_Test;
};

int TestBaseTask::TaskInit(loss::CJsonObject& oJsonConf, uint32_t uiWkId) {
  TLOG4_INFO("TestBaseTask buis init, workid: %d", uiWkId);
  return 0;
}

int TestBaseTask::HandleLoop() {
  TLOG4_INFO("child proc is run.... ");
  static int i = 0;
  sleep(1);
  if (++i%10 == 0) {
    //abort();
  }
  return 0;
}
int main(int argc, char*argv[]) {
  TestBaseTask test;
  test.Run(argc, argv);
  std::cout << "this is task lib " << std::endl;
  return 0;
}
