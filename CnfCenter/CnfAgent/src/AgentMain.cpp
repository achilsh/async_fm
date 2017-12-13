#include "CnfAgentOne.h" 
using namespace SubCnfTask;

int main(int argc, char **argv) {
  SubCnfAgent* pCnfAgent = new CnfAgentOne();
  pCnfAgent->Run(argc, argv); 
  //
  return 0;
}
