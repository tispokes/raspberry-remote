#include <wiringPi.h>

char nGroup[6];
int nSys;
int nSwitchNumber;
int nAction;
int nPlugs;
int nTimeout;
unsigned long nRawCode;
int PORT = 11337;
int GPIO_PIN = 0;

sa_family_t family = AF_INET6; // Set to AF_INET for IPv4 only, and to AF_INET6 for IPv6 AND IPv4

void error(const char *msg);
char* decToBinary(int n);
int getAddrElro(const char* nGroup, int nSwitchNumber);
int getAddrInt(const char* nGroup, int nSwitchNumber);
int getAddrRaw(unsigned long nRaw);

PI_THREAD(switchOn);
PI_THREAD(switchOff);
