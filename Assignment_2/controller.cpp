#include "controller.h"
#include <iostream>
#include <stdio.h>
#include <sstream>
#include <algorithm>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

struct Session {
  string inFifo;
  string outFifo;
  int inFd;
  int outFd;
  int port;
};

PacketInfo controllerInfo;
vector<Switch> switches;
vector<Session> sessions;

PacketInfo stats = {
{ {HELLO, 0}, {ASK, 0} },
{ {HELLO_ACK, 0}, {ADD, 0} }
};


void controller(int num_of_switch) {

    pollfd pfd[num_of_switch + 2];

    controllerInfo = stats;

    // non-blocking I/O polling from STDIN
    pfd[0].fd = STDIN_FILENO;
    pfd[0].events = POLLIN;
    pfd[0].revents = 0;

    // init fifo
    for (int i = 0; i < num_of_switch; i++) {
        string inFifo = nameFifo(i+1, 0);
        string outFifo = nameFifo(0, i+1);
        mkfifo(inFifo.c_str(), 0666);
        if (errno) perror("Error: Could not create a FIFO connection.\n");
        errno = 0;

        mkfifo(outFifo.c_str(), 0666);
        if (errno) perror("Error: Could not create a FIFO connection.\n");
        errno = 0;

        int inFd = open(inFifo.c_str(), O_RDONLY | O_NONBLOCK);
        if (errno) perror("Error: Could not open FIFO.\n");
        errno = 0;

        // Returns lowest unused file descriptor on success
        int outFd = open(outFifo.c_str(), O_RDWR | O_NONBLOCK);
        if (errno) perror("Error: Could not open FIFO.\n");
        errno = 0;      


        Session session = {inFifo, outFifo, inFd, outFd, 0};
        sessions.push_back(session);

        pfd[i+1].fd = inFd;
        pfd[i+1].events = POLLIN;
        pfd[i+1].revents = 0;
    }


    while(true) {
        
        poll(pfd, num_of_switch + 1, 0);
        char buffer[MAXBUF];
        if (pfd[0].revents & POLLIN) {

            string cmd;
            cin >> cmd;
            if (cmd == "info") {
                printInfo();
            } else if (cmd == "exit") {
                printInfo();
                exit(0);
            } else {
                printf("Not a command. Use info or exit.\n");
            }         
        }

    for (int i = 0; i < num_of_switch; i++) {
        if (pfd[i+1].revents & POLLIN) {
        char buffer[MAXBUF];
        read(sessions[i].inFd, buffer, MAXBUF);
        string s = string(buffer);
        stringstream ss(s);
        vector<string> temp;
        string buf;

        while (getline(ss, buf, ' ')) {
            temp.push_back(buf);
        }
        controllerReceive(temp); // change
        }
    }
    }
}

void printInfo() {
    printf("Switch information: \n");
    for (auto s: switches) {
        printf("[psw%i]: port1= %i, port2= %i, port3= %i-%i\n", s.switchID,
           s.port1, s.port2, s.ipranges.low, s.ipranges.high);
    }
    printf("\n");
    printf("Packet stats:\n");
    receivedInfo();
    transmittedInfo();
}

void receivedInfo() {
    string received = "\tReceived: ";
    for (auto stat : controllerInfo.received) {
    int counter = 0;
    stringstream receivedstring;
    if (stat.first == HELLO) {
        receivedstring << "HELLO:" << stat.second;

    } else if (stat.first == ASK) {
        receivedstring << "ASK:" << stat.second;
    }

    if (counter != controllerInfo.received.size() - 1) {
        receivedstring << ", ";
    }
    received += receivedstring.str();
    counter += 1;
    }
    printf("%s\n", received.c_str());
}

void transmittedInfo() {
    string transmitted = "\tTransmitted: ";
    for (auto stat : controllerInfo.transmitted) {
    int counter = 0;
    stringstream transmittedstring;
    if (stat.first == HELLO_ACK) {
        transmittedstring << "HELLO_ACK:" << stat.second;
    } else if (stat.first == ADD) {
        transmittedstring << "ADD:" << stat.second;
    } 

    if (counter != controllerInfo.received.size() - 1) {
        transmittedstring << ", ";
    } 
    transmitted += transmittedstring.str();
    counter += 1;
    }
    printf("%s\n", transmitted.c_str());
}


void controllerReceive(vector<string> message) {
    string type = message[0];
    cout << "Received: (src= master, dest= sw" << message[1] <<") [" << message[0] << "]" << endl;
    if (type == "HELLO") {
    controllerInfo.received[HELLO] += 1;
    int switchID = stoi(message[1]);
    int port1 = stoi(message[2]) <= 0 ? -1 : stoi(message[2]);
    int port2 = stoi(message[3]) <= 0 ? -1 : stoi(message[3]);
    int port3low = stoi(message[4]);
    int port3high = stoi(message[5]);

    switches.push_back({ switchID, port1, port2, { port3low, port3high } });
    sendPacket(sessions[switchID-1].outFd, HELLO_ACK, "HELLO_ACK 0");
    controllerInfo.transmitted[HELLO_ACK] += 1;
    } else if (type == "ASK") {
    int switchID = stoi(message[1]);
    controllerInfo.received[ASK] += 1;
    stringstream ss;
    ss << "ADD " << message[1] << " ";
    bool if_found = false;
    for (auto sw : switches) {
        if (sw.ipranges.low <= stoi(message[2]) && stoi(message[2]) <= sw.ipranges.high) {
        ss << sw.port1 << " " << sw.port2 << " " << sw.ipranges.low << " " << sw.ipranges.high;
        if_found = true;
        break;
        }
    }
    if (!if_found) ss << message[2];
    controllerInfo.transmitted[ADD] += 1;
    sendPacket(sessions[switchID-1].outFd, ADD, ss.str());
    }
}







