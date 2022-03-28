#include "alloc.h"
#include <chrono>
#include <iostream>
#include <thread>
//#include "memorymanager.h"
//#include "memorypool.h"
using namespace std::chrono;
using namespace std;
class A
{
public:
	int a[2];
};


void testFunc()
{
	int cnt = 500000;
	A* pArr[5000];
	while (--cnt)
	{
		for (int i = 0; i < 5000; ++i)
		{
			pArr[i] = new A;
		}
		for (int i = 0; i < 5000; ++i)
		{
			delete pArr[i];
		}
	}
}


int main()
{
	// MemoryManager::instance();
	thread th[2];
	time_point<high_resolution_clock> begin = high_resolution_clock::now();
	for (int i = 0; i < 2; ++i)
	{
		th[i] = thread(testFunc);
	}
	for (int i = 0; i < 2; ++i)
	{
		th[i].join();
	}
	cout << duration_cast<milliseconds>(high_resolution_clock::now() - begin).count() << endl;
}
