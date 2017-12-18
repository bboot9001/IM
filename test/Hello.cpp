#include <iostream>
#include "Lock.h"
BLock gLock;
int main()
{
	BAutoLock autoLock(&gLock);
	std::cout<<"hello cmake"<<std::endl;
	return 0;
}