#include "util/LibSingleton.h"
#include <iostream>

using namespace loss;
using namespace std;

class Test {
    public:
     Test() {}
     virtual ~Test() {}
     void SetX(int x) { m_X = x; }
     int GetX() const { return m_X; }
    private:
     int m_X;
};

int main()
{
    Test* pTest = SINGLETON(Test);
    pTest->SetX(1000);
    std::cout << pTest->GetX() << std::endl;
    return 0;
}

