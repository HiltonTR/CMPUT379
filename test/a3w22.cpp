#include <fcntl.h>
#include <sys/resource.h>
#include <fstream>
#include <sstream>
#include <string>
#include <tuple>
#include <netdb.h>
#include <cstring>
#include <arpa/inet.h>
#include <iostream>
#include "master.h"
#include "switch.h"
#include "general.h"

#define MAX_NSW 7
#define MAX_IP 1000

using namespace std;

tuple<int, int> getIpRange(string input) {
	int ipLow = 0;
	int ipHigh = 0;

    stringstream ss(input);
    string foo;
    vector<string> temp;

    while (getline(ss, foo, '-')) {
        temp.push_back(foo);
    }

    ipLow = stoi(temp[0]);
    ipHigh = stoi(temp[1]);

	if (ipHigh < ipLow) {
		printf("Error: Invalid range.\n");
		exit(EXIT_FAILURE);
	}

	return make_tuple(ipLow, ipHigh);
}



int main(int argc, char **argv) {

	if (argc < 2) {
		printf("Too few arguments.\n");
		return EXIT_FAILURE;
	}

	string mode = argv[1];  // cont or swi
	if (mode == "master") {
		int numSwitches = (int) strtol(argv[2], (char **) nullptr, 10);
		if (argc != 4 || numSwitches > MAX_NSW || numSwitches < 1 || errno) {
		printf("Error: Invalid entry.\n");
		return EXIT_FAILURE;
		}

		int portNumber = stoi(argv[3]);
		MasterLoop(numSwitches, portNumber);

	} else if (mode.find("sw") != std::string::npos) {
		if (argc != 8) {
		printf("Error: Invalid argument.\n");
		return EXIT_FAILURE;
		}

    	ifstream in(argv[2]);

		//cout << string(argv[1]).substr(3, 1) << endl;
		int switchId = stoi(string(argv[1]).substr(3));
		int switchId1 = -1;
        int switchId2 = -1;
        if (argv[3] != string("null")){
            switchId1 = stoi(string(argv[3]).substr(3,1)); 
        }
        if (argv[4] != string("null")){
            switchId2 = stoi(string(argv[4]).substr(3,1)); 
        } 

		tuple<int, int> ipRange = getIpRange(argv[5]);

		string serverAddress = argv[6];
		cout << serverAddress << endl;
		string ipAddress = serverAddress;

		printf("Found IP: %s\n", ipAddress.c_str());

		int portNumber = stoi(argv[7]);

		switchLoop(switchId, switchId1, switchId2, get<0>(ipRange), get<1>(ipRange), in, ipAddress,
				portNumber);
	} else {
		printf("Error: Invalid mode specified. Expected master or pswi.\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
