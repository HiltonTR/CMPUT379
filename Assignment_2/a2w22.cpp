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
        exit(EINVAL);
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

        //cout << pswj << " " << pswk << endl;

        IPs ip_range = split_ip(argv[5]);
        
        //cout << "asdf" << endl;

        masterSwitch(switchID, pswj, pswk, argv[2], ip_range);

    } else {
        printf("Invalid argument\n");
        exit(EXIT_FAILURE);
    }
    return 0;
}

IPs split_ip(string input) {
    stringstream ss(input);
    string item;
    vector<int> temp;
    IPs ip_range;

    while (getline(ss, item, '-')) {
        temp.push_back(stoi(item));
    }

    ip_range.low = temp[0];
    ip_range.high = temp[1];

    return ip_range;
}

string get_fifo(int start, int end) {
    return "fifo-" + to_string(start) + "-" + to_string(end);
}

string action_type_tostring(ActionType type) {
    if (type == FORWARD) return "FORWARD";
    else if (type == DROP) return "DROP";
}

void send_message(int fd, PktType type, string message) {
    char buffer[message.length() + 1];
    strcpy(buffer, message.c_str());
    if(write(fd, buffer, sizeof(buffer)) < 0) {
        printf("error");
    } else {
        cout << "succceded" << endl;
    }
}

vector<string> parse_message(string message) {
    stringstream ss(message);
    vector<string> placeholder;
    string buffer;

    while (getline(ss, buffer, ' ')) {
        placeholder.push_back(buffer);
    }
    return placeholder;
}

