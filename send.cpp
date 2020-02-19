/*
 * Usage: ./send <systemCode> <unitCode> <command>
 * Command is 0 for OFF and 1 for ON
 */

#include "RCSwitch.h"
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <getopt.h>
//#include <iostream>

void printUsage() {
    printf("This is rasperry remote, an application to control remote plugs with the\nRaspberry Pi.\n");
    printf("Based on RCSwitch and wiringPi. See github.com/xkonni/raspberry-remote for\nfurther reference.\n");
    printf("Usage: \n sudo send [options] <systemCode> <unitCode> [<systemCode> <unitCode>] <command>\n or: sudo send -h\n\n");
    printf("Options:\n\n");
    printf(" -b, --binary:\n");
    printf("   Switches the instance to binary mode.\n");
    printf("   Binary mode means, that instead numbering the sockets by 00001 00010 00100\n");
    printf("   01000 10000, the sockets are numbered in real binary numbers as following:\n");
    printf("   00001 00010 00011 00100 00101 00110 and so on.\n");
    printf("   This means that your sockets need to be setup in this manner, which often\n");
    printf("   includes that the dedicated remote is rendered useless, but more than\n");
    printf("   6 sockets are supported.\n\n");
    printf(" -h, --help:\n");
    printf("   displays this help\n\n");
    printf(" -p X, --pin=X\n");
    printf("   Sets the pin to use to communicate with the sender.\n");
    printf("   Important note: when running as root, pin numbers are wiringPi numbers while\n");
    printf("   in user mode (-u, --user) pin numbers are BCM_GPIO numbers.\n");
    printf("   See http://wiringpi.com/pins/ for details.\n");
    printf("   Default: 0 in normal mode, 17 in user mode\n\n");
    printf(" -s, --silent:\n");
    printf("   Don't print any text, except for errors\n\n");
    printf(" -u, --user:\n");
    printf("   Switches the instance to user mode.\n");
    printf("   In this mode this program does not need root privileges. It is required to\n");
    printf("   export the GPIO pin using the gpio utility. Note that pin numbers are\n");
    printf("   BCM_GPIO numbers in this mode! the export command for the default port is\n");
    printf("   \"gpio export 17 out\". For more information about port numbering see\n");
    printf("   http://wiringpi.com/pins/.\n");
    printf(" -r, --raw:\n");
    printf("   Send Code with raw data: name <raw> [<bits> <protocol>]\n");
    printf("   [standard bits = 32, protocol = 2]\n\n");
}

int main(int argc, char *argv[]) {
    bool silentMode = false;
    bool binaryMode = false;
    bool userMode = false;
    bool rawMode = false;
    int pin = 0;
    int controlArgCount = 0;
    char *systemCode;
    int unitCode;
    int command;
    bool multiMode;

    int c;
    while (1) {
        static struct option long_options[] =
            {
              {"binary", no_argument, 0, 'b'},
              {"help", no_argument, 0, 'h'},
              {"pin", required_argument, 0, 'p'},
              {"silent", no_argument, 0, 's'},
              {"user", no_argument, 0, 'u'},
	      {"raw", no_argument, 0, 'r'},
              0
            };
        int option_index = 0;

        c = getopt_long (argc, argv, "bhp:sur",long_options, &option_index);
        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c) {
            case 'b':
                binaryMode = true;
                break;
            case 'p':
                pin = atoi(optarg);
                break;
            case 's':
                silentMode = true;
                break;
            case 'u':
                userMode = true;
                break;
	    case 'r':
		rawMode = true;
		break;
            case 'h':
                printUsage();
                return 0;
        }
    }

    // When started in user mode, wiringPiSetupSys() is used. In this mode
    // wiringPi uses the /sys/class/gpio interface and the GPIO pin are numbered
    // by the native Broadcom GPIO numbers, where wiringPi-number 0 translates to
    // bcm-number 17... so use this as default if no pin was set with -p
    // reference:
    //   http://wiringpi.com/reference/setup/
    //   http://wiringpi.com/pins/
    if (pin == 0 && userMode) {
      pin = 17;
    }

    controlArgCount = argc - optind;
    // we need at least 3 args: systemCode, unitCode and command
    if (controlArgCount >= 3) {
        if (!silentMode) {
            if (binaryMode) {
                printf("operating in binary mode\n");
            }
            if (userMode) {
                printf("operating in user mode\n");
            }
	    if (rawMode) {
		printf("operating in raw mode\n");
	    }
            printf("using pin %d\n", pin);
        }

        char *controlArgs[controlArgCount];
        int i = 0;
        while (optind < argc && i < controlArgCount) {
            controlArgs[i] = argv[optind++];
            i++;
        }

        multiMode = controlArgCount > 3;

        int numberOfActuators = (controlArgCount - 1) / 2;
        // check if there are enough arguments supplied, we need (numberOfActuators * 2) + 1

        if (controlArgCount != (numberOfActuators * 2) + 1) {
          printf("invalid set of control arguments\nuse <systemCode> <unitCode> <command> or\n");
          printf("<systemCode> <unitCode> [<systemCode> <unitCode>...] <command>\n");
          return 1;
        }

        if (multiMode && !silentMode) {
            printf("multi mode\n");
        }

        command = atoi(controlArgs[controlArgCount - 1]);
        if (userMode) {
            if (wiringPiSetupSys() == -1) return 1;
        } else {
            if (wiringPiSetup() == -1) return 1;
        }
        piHiPri(20);
        RCSwitch mySwitch = RCSwitch();
        mySwitch.setPulseLength(300);
        mySwitch.enableTransmit(pin);
	
	if (rawMode){
	    if (!silentMode)
		    printf("sending raw[%lu] bits[%i] protocol[%i]", raw, bits, protocol);
	    unsigned long raw = strtoul(argv[1], NULL, 0);
	    int bits = atoi(argv[2]);
	    int protocol = atoi(argv[3]);
	    mySwitch.setProtocol(protocol);
	    mySwitch.send(raw, bits);
	    return 0;
	}
        for (i = 0; i < numberOfActuators; i++) {
            int indexSystemCode = i * 2;
            int indexUnitCode = indexSystemCode + 1;
            systemCode = controlArgs[indexSystemCode];
            unitCode = atoi(controlArgs[indexUnitCode]);

            if (!silentMode) {
                printf("sending systemCode[%s] unitCode[%i] command[%i]\n", systemCode, unitCode, command);
            }
            if (binaryMode) {
                switch (command) {
                    case 1:
                        mySwitch.switchOnBinary(systemCode, unitCode);
                        break;
                    case 0:
                        mySwitch.switchOffBinary(systemCode, unitCode);
                        break;
                    default:
                        printf("command[%i] is unsupported\n", command);
                        printUsage();
                        if (!multiMode) {
                            return -1;
                        }
                }
            } else {
                switch (command) {
                    case 1:
                        mySwitch.switchOn(systemCode, unitCode);
                        break;
                    case 0:
                        mySwitch.switchOff(systemCode, unitCode);
                        break;
                    case 2:
                        // 00001 2 on binary coded
                        mySwitch.send("010101010001000101010001");
                        break;
                    case 3:
                        // 00001 2 on as TriState
                        mySwitch.sendTriState("FFFF0F0FFF0F");
                        break;
                    default:
                        printf("command[%i] is unsupported\n", command);
                        printUsage();
                        if (!multiMode) {
                            return -1;
                        }
                }
            }
        }
        return 0;
    } else {
        printUsage();
        return -1;
    }
}
