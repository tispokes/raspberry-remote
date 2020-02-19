
# Defines the RPI variable which is needed by rc-switch/RCSwitch.h
CXXFLAGS=-DRPI

all: codesend RFSniffer daemon

send: RCSwitch.o send.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $+ -o $@ -lwiringPi -lwiringPiDev -lcrypt
	
codesend: RCSwitch.o codesend.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $+ -o $@ -lwiringPi -lwiringPiDev -lcrypt
	
RFSniffer: RCSwitch.o RFSniffer.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $+ -o $@ -lwiringPi -lwiringPiDev -lcrypt
	
daemon: RCSwitch.o daemon.o
	$(CXX) -Wall -pthread $+ -o $@ -Wall -fpermissive -lwiringPi $(LDFLAGS)

clean:
	$(RM) *.o send codesend daemon RFSniffer

