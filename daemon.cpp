/**
 * RCSwitch daemon for the Raspberry Pi
 *
 * Setup
 *   Power to pin4
 *   GND   to pin6
 *   DATA  to pin11/gpio0
 *
 * Usage
 *   send axxxxxyyz to ip:port
 *   a		systemcode. 1 for classic elro, 2 for Intertechno 
 *   xxxxx  encoding
 *          00001 for first channel
 *   yy     plug
 *          16 for plug 1
 *          08 for plug 2
 *          04 for plug 3
 *          02 for plug 4
 *          01 for plug 5
 *   z      action
 *          0:off|1:on|2:status
 *
 * Examples of remote actions
 *   Switch plug A on 00001 to on
 *     echo 100001161 | nc localhost 11337
 *
 *   Switch plug B on 00001 to off
 *     echo 100001080 | nc localhost 11337
 *
 *   Get status of plug A on 00001
 *     echo 100001162 | nc localhost 11337
 *
 *   Switch intertechno plug 2 on group 2 to on
 *     echo 202021 | nc localhost 11337
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "daemon.h"
#include "RCSwitch.h"

RCSwitch mySwitch;

int main(int argc, char *argv[])
{
	/**
	* Setup wiringPi and RCSwitch
	* set high priority scheduling
	*/
	if (wiringPiSetup() == -1)
	{
		return 1;
	}
	piHiPri(10);
	mySwitch = RCSwitch();
	mySwitch.setPulseLength(300);
	usleep(50000);
	mySwitch.enableTransmit(GPIO_PIN);
	nPlugs = 1536; // increased from 1280 to 1536 (0x600) for extra state 'raw data'
	int nState[nPlugs];

	nTimeout = 0;
	memset(nState, 0, sizeof(nState));

	/**
	* setup socket
	*/
	int sockfd, newsockfd, portno;
	socklen_t clilen;
	char buffer[256];
	struct sockaddr_in6 serv_addr, cli_addr;
	int n;

	// printf(typeid(INADDR_ANY).name());
	// printf(typeid(in6addr_any).name());
	bzero((char *)&serv_addr, sizeof(serv_addr));
	portno = PORT;
	serv_addr.sin6_family = family;
	serv_addr.sin6_addr = IN6ADDR_ANY_INIT; // was: 'serv_addr.sin_addr.s_addr = INADDR_ANY;'
	serv_addr.sin6_port = htons(portno);

	// receiving socket
	sockfd = socket(family, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		error("ERROR opening socket");
	}
	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		error("ERROR on binding");
	}
	// sending socket
	newsockfd = socket(AF_INET6, SOCK_STREAM, 0);
	if (newsockfd < 0)
	{
		error("ERROR opening socket");
	}

	/*
	* start listening
	*/
	while (true)
	{
		listen(sockfd, 5);
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
		if (newsockfd < 0)
		{
			error("ERROR on accept");
		}
		bzero(buffer, 256);
		n = read(newsockfd, buffer, 255);
		if (n < 0)
		{
			error("ERROR reading from socket");
		}
		/*
		* get values
		*/
		printf("\nmessage: %s\n", buffer);
		if (strlen(buffer) >= 5)
		{
			nSys = buffer[0] - 48;
			switch (nSys)
			{
			// raw data 32bit, implemented by tispokes
			case 0:
			{
				mySwitch.setProtocol(2); // need longer pulse length here
				nAction = buffer[11] - 48;
				unsigned long nRawCode = 0l;
				for (int i = 1; i < 11; i++)
				{
					nRawCode *= 10l;
					nRawCode += buffer[i] - 48;
				}
				int nAddr = getAddrRaw(nRawCode);

				printf("nAddr: %i\n", nAddr);
				printf("nPlugs: %i\n", nPlugs);
				printf("nRawCode ON: %lu\n", nRawCode);
				printf("nRawCode OFF: %lu\n", nRawCode ^ (7 << 25));
				printf("nAction: %i\n", nAction);
				char msg[13];

				if (nAddr > 1535 || nAddr < 1280)
				{
					printf("Switch out of range: %lu\n", nRawCode);
					n = write(newsockfd, "2", 1);
				}
				else
				{
					switch (nAction)
					{
					// OFF
					case 0:
						nRawCode ^= (7 << 25);		 // XOR with 111(25*0) to flip these 3 bits for off-code
						mySwitch.send(nRawCode, 32); //off
						mySwitch.send(nRawCode, 32); //off
						nState[nAddr] = 0;
						sprintf(msg, "%d", nState[nAddr]);
						n = write(newsockfd, msg, 1);
						break;
					// ON
					case 1:
						mySwitch.send(nRawCode, 32); //on
						mySwitch.send(nRawCode, 32);
						nState[nAddr] = 1;
						sprintf(msg, "%d", nState[nAddr]);
						n = write(newsockfd, msg, 1);
						break;
					// STATUS
					case 2:
						sprintf(msg, "%d", nState[nAddr]);
						n = write(newsockfd, msg, 1);
						break;
					}
				}
			}
			break;
			//normal elro
			case 1:
			{
				mySwitch.setProtocol(1); // normal pulse length
										 // from dec2bin for the nSwitchNumber, cause new RCSwitch hasn't switchBinary()

				// for (int x = 1; x < 32; x++)
				// {
				// 	const char *bin = decToBinary(x);
				// while (p < &bin[5])
				// 	printf("%x ", *p++);
				// printf("\n");
				// }

				for (int i = 1; i < 6; i++)
				{
					nGroup[i - 1] = buffer[i];
				}
				nGroup[5] = '\0';
				nSwitchNumber = (buffer[6] - 48) * 10;
				nSwitchNumber += (buffer[7] - 48);
				nAction = buffer[8] - 48;
				nTimeout = 0;
				printf("nSys: %i\n", nSys);
				printf("nGroup: %s\n", nGroup);
				printf("nSwitchNumber: %i\n", nSwitchNumber);
				printf("nAction: %i\n", nAction);

				if (strlen(buffer) >= 10)
					nTimeout = buffer[9] - 48;
				if (strlen(buffer) >= 11)
					nTimeout = nTimeout * 10 + buffer[10] - 48;
				if (strlen(buffer) >= 12)
					nTimeout = nTimeout * 10 + buffer[11] - 48;

				/**
					* handle messages
					*/
				int nAddr = getAddrElro(nGroup, nSwitchNumber);
				printf("nAddr: %i\n", nAddr);
				printf("nPlugs: %i\n", nPlugs);
				char msg[13];
				if (nAddr > 1023 || nAddr < 0)
				{
					printf("Switch out of range: %s:%d\n", nGroup, nSwitchNumber);
					n = write(newsockfd, "2", 1);
				}
				else
				{
					switch (nAction)
					{
					//OFF
					case 0:
					{
						//piThreadCreate(switchOff);
						mySwitch.switchOff(nGroup, decToBinary(nSwitchNumber));
						nState[nAddr] = 0;
						sprintf(msg, "%d", nState[nAddr]);
						n = write(newsockfd, msg, 1);
						break;
					}
					//ON
					case 1:
					{
						//piThreadCreate(switchOn);
						mySwitch.switchOn(nGroup, decToBinary(nSwitchNumber));
						nState[nAddr] = 1;
						//sprintf(msg, "nState[%d] = %d", nAddr, nState[nAddr]);
						sprintf(msg, "%d", nState[nAddr]);
						n = write(newsockfd, msg, 1);
						break;
					}
					//STATUS
					case 2:
					{
						//sprintf(msg, "nState[%d] = %d", nAddr, nState[nAddr]);
						sprintf(msg, "%d", nState[nAddr]);
						n = write(newsockfd, msg, 1);
						break;
					}
					}
				}
				break;
			}

			//Intertechno
			case 2:
			{
				mySwitch.setProtocol(1); // normal pulse length
				nGroup[0] = buffer[1];
				nGroup[1] = buffer[2];
				nGroup[2] = '\0';
				nSwitchNumber = (buffer[3] - 48) * 10;
				nSwitchNumber += (buffer[4] - 48);
				nAction = buffer[5] - 48;
				nTimeout = 0;
				printf("nSys: %i\n", nSys);
				printf("nGroup: %s\n", nGroup);
				printf("nSwitchNumber: %i\n", nSwitchNumber);
				printf("nAction: %i\n", nAction);
				int nAddr = getAddrInt(nGroup, nSwitchNumber);
				printf("nAddr: %i\n", nAddr);
				printf("nPlugs: %i\n", nPlugs);
				char msg[13];
				if (nAddr > 1279 || nAddr < 1024)
				{
					printf("Switch out of range: %s:%d\n", nGroup, nSwitchNumber);
					n = write(newsockfd, "2", 1);
				}
				else
				{
					printf("computing systemcode for Intertechno Type B house[%s] unit[%i] ... ", nGroup, nSwitchNumber);
					char pSystemCode[14];
					switch (atoi(nGroup))
					{
					// house/family code A=1 - P=16
					case 1:
					{
						printf("1/A ... ");
						strcpy(pSystemCode, "0000");
						break;
					}
					case 2:
					{
						printf("2/B ... ");
						strcpy(pSystemCode, "F000");
						break;
					}
					case 3:
					{
						printf("3/C ... ");
						strcpy(pSystemCode, "0F00");
						break;
					}
					case 4:
					{
						printf("4/D ... ");
						strcpy(pSystemCode, "FF00");
						break;
					}
					case 5:
					{
						printf("5/E ... ");
						strcpy(pSystemCode, "00F0");
						break;
					}
					case 6:
					{
						printf("6/F ... ");
						strcpy(pSystemCode, "F0F0");
						break;
					}
					case 7:
					{
						printf("7/G ... ");
						strcpy(pSystemCode, "0FF0");
						break;
					}
					case 8:
					{
						printf("8/H ... ");
						strcpy(pSystemCode, "FFF0");
						break;
					}
					case 9:
					{
						printf("9/I ... ");
						strcpy(pSystemCode, "000F");
						break;
					}
					case 10:
					{
						printf("10/J ... ");
						strcpy(pSystemCode, "F00F");
						break;
					}
					case 11:
					{
						printf("11/K ... ");
						strcpy(pSystemCode, "0F0F");
						break;
					}
					case 12:
					{
						printf("12/L ... ");
						strcpy(pSystemCode, "FF0F");
						break;
					}
					case 13:
					{
						printf("13/M ... ");
						strcpy(pSystemCode, "00FF");
						break;
					}
					case 14:
					{
						printf("14/N ... ");
						strcpy(pSystemCode, "F0FF");
						break;
					}
					case 15:
					{
						printf("15/O ... ");
						strcpy(pSystemCode, "0FFF");
						break;
					}
					case 16:
					{
						printf("16/P ... ");
						strcpy(pSystemCode, "FFFF");
						break;
					}
					default:
					{
						printf("systemCode[%s] is unsupported\n", nGroup);
						return -1;
					}
					}
					printf("got systemCode[%s] ", nGroup);
					switch (nSwitchNumber)
					{
					// unit/group code 01-16
					case 1:
					{
						printf("1 ... ");
						strcat(pSystemCode, "0000");
						break;
					}
					case 2:
					{
						printf("2 ... ");
						strcat(pSystemCode, "F000");
						break;
					}
					case 3:
					{
						printf("3 ... ");
						strcat(pSystemCode, "0F00");
						break;
					}
					case 4:
					{
						printf("4 ... ");
						strcat(pSystemCode, "FF00");
						break;
					}
					case 5:
					{
						printf("5 ... ");
						strcat(pSystemCode, "00F0");
						break;
					}
					case 6:
					{
						printf("6 ... ");
						strcat(pSystemCode, "F0F0");
						break;
					}
					case 7:
					{
						printf("7 ... ");
						strcat(pSystemCode, "0FF0");
						break;
					}
					case 8:
					{
						printf("8 ... ");
						strcat(pSystemCode, "FFF0");
						break;
					}
					case 9:
					{
						printf("9 ... ");
						strcat(pSystemCode, "000F");
						break;
					}
					case 10:
					{
						printf("10 ... ");
						strcat(pSystemCode, "F00F");
						break;
					}
					case 11:
					{
						printf("11 ... ");
						strcat(pSystemCode, "0F0F");
						break;
					}
					case 12:
					{
						printf("12 ... ");
						strcat(pSystemCode, "FF0F");
						break;
					}
					case 13:
					{
						printf("13 ... ");
						strcat(pSystemCode, "00FF");
						break;
					}
					case 14:
					{
						printf("14 ... ");
						strcat(pSystemCode, "F0FF");
						break;
					}
					case 15:
					{
						printf("15 ... ");
						strcat(pSystemCode, "0FFF");
						break;
					}
					case 16:
					{
						printf("16 ... ");
						strcat(pSystemCode, "FFFF");
						break;
					}
					default:
					{
						printf("unitCode[%i] is unsupported\n", nSwitchNumber);
						return -1;
					}
					}
					strcat(pSystemCode, "0F"); // mandatory bits
					switch (nAction)
					{
					case 0:
					{
						strcat(pSystemCode, "F0");
						mySwitch.sendTriState(pSystemCode);
						printf("sent TriState signal: pSystemCode[%s]\n", pSystemCode);
						nState[nAddr] = 0;
						//sprintf(msg, "nState[%d] = %d", nAddr, nState[nAddr]);
						sprintf(msg, "%d", nState[nAddr]);
						n = write(newsockfd, msg, 1);
						break;
					}
					case 1:
					{
						strcat(pSystemCode, "FF");
						mySwitch.sendTriState(pSystemCode);
						printf("sent TriState signal: pSystemCode[%s]\n", pSystemCode);
						nState[nAddr] = 1;
						//sprintf(msg, "nState[%d] = %d", nAddr, nState[nAddr]);
						sprintf(msg, "%d", nState[nAddr]);
						n = write(newsockfd, msg, 1);
						break;
					}
					case 2:
					{
						//sprintf(msg, "nState[%d] = %d", nAddr, nState[nAddr]);
						sprintf(msg, "%d", nState[nAddr]);
						n = write(newsockfd, msg, 1);
						break;
					}
					default:
					{
						printf("command[%i] is unsupported\n", nAction);
						return -1;
					}
					}
				}
				break;
			}
			default:
			{
				printf("wrong systemkey!\n");
			}
			}
		}
		else
		{
			printf("message corrupted or incomplete");
		}
		if (n < 0)
		{
			error("ERROR writing to socket");
			close(newsockfd);
		}
	}

	/**
	 * terminate
	 */
	//close(newsockfd);
	close(sockfd);
	return 0;
}

/**
 * error output
 */
void error(const char *msg)
{
	perror(msg);
	exit(1);
}

/**
 * calculate the array address of the power state for elro
 */
int getAddrElro(const char *nGroup, int nSwitchNumber)
{
	int tempgroup = atoi(nGroup);
	int group = 0;
	for (int i = 0; i < 5; i++)
	{
		if ((tempgroup % 10) == 1)
		{
			group = group | (1 << i);
		}
		tempgroup = tempgroup / 10;
	}

	int switchnr = nSwitchNumber & 0b00011111;
	int result = (group << 5) | switchnr;
	printf("result: %i\n", result);
	return result;
}

/**
 * calculate the array address of the power state for intertechno
 */
int getAddrInt(const char *nGroup, int nSwitchNumber)
{
	return ((atoi(nGroup) - 1) * 16) + (nSwitchNumber - 1) + 1024;
}

/**
 * calculate the array address of the power state for 32-bit
 */
int getAddrRaw(unsigned long nRaw)
{
	return nRaw % (nPlugs - 1 - 1280) + 1280; // 1280 is the old size of the array, cutting the RawCode to a max of 255 in this case
}

// reverse calculator for dec2bin
char* decToBinary(int n)
{
	// array to store binary number
	const int len = 5;
	static char binaryNum[len];

	int i = len - 1;
	while (i >= 0)
	{
		// storing remainder in binary array, as char
		binaryNum[i] = n % 2 + 48;
		n = n / 2;
		i--;
	}
	binaryNum[len] = '\0'; // End of Array

	return binaryNum;
}

PI_THREAD(switchOn)
{
	printf("switchOnThread: %d\n", nTimeout);
	char tGroup[6];
	int tSwitchNumber;
	memcpy(tGroup, nGroup, sizeof(tGroup));
	tSwitchNumber = nSwitchNumber;
	sleep(nTimeout * 60);
	mySwitch.switchOn(tGroup, tSwitchNumber);
	return 0;
}

PI_THREAD(switchOff)
{
	printf("switchOffThread: %d\n", nTimeout);
	char tGroup[6];
	int tSwitchNumber;
	memcpy(tGroup, nGroup, sizeof(tGroup));
	tSwitchNumber = nSwitchNumber;
	sleep(nTimeout * 60);
	mySwitch.switchOff(tGroup, tSwitchNumber);
	return 0;
}
