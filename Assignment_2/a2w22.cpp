#include "a2w22.h"
#include "controller.h"
#include "switch.h"
#include <sstream>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <algorithm> 
using namespace std;

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 6) {
        printf("ERROR: invalid argument format:\n"
               "    a2w22 master nSwitch\n"
               "    a2w22 psw1 dataFile (null|pswj) (null|pswk) IPlow-IPhigh\n");
        exit(0);
    }

    if (argv[1] == string("master") && argc == 3 && stoi(argv[2]) <= MAX_SWITCH) {
        controller(stoi(argv[2]));

    } else if (string(argv[1]).find("psw") != string::npos && argc == 6) {

        int switchID = stoi(string(argv[1]).substr(3));
        int pswj = -1;
        int pswk = -1;
        if (argv[3] != string("null")){
            pswj = stoi(string(argv[3]).substr(3,1)); //
        }
        if (argv[4] != string("null")){
            pswk = stoi(string(argv[4]).substr(3,1)); //
        } 
        IPs ip_range = split_ip(argv[5]);
        masterSwitch(switchID, pswj, pswk, argv[2], ip_range);

    } else {
        printf("Invalid argument\n");
        exit(0);
    }
    return 0;
}
//https://stackoverflow.com/a/68397097
IPs split_ip(string input) {
    stringstream ss(input);
    string foo;
    vector<string> temp;
    IPs ip_range;

    while (getline(ss, foo, '-')) {
        temp.push_back(foo);
    }

    ip_range.low = stoi(temp[0]);
    ip_range.high = stoi(temp[1]);

    return ip_range;
}

string nameFifo(int start, int end) {
    return "fifo-" + to_string(start) + "-" + to_string(end);
}


void sendPacket(int fd, PktType type, string message) {
    char buffer[message.length() + 1];
    strcpy(buffer, message.c_str());
    if(write(fd, buffer, sizeof(buffer)) < 0) {
        printf("error");
    }
}
