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

PacketInfo controllerInfo;
vector<Switch> switches;
vector<Session> sessions;

PacketInfo stats = {
{ {HELLO, 0}, {ASK, 0} },
{ {HELLO_ACK, 0}, {ADD, 0} }
};


void controller(int num_of_switch) {
    cout << "checkpoint 1" << endl;

    pollfd pfd[num_of_switch + 2];

    controllerInfo = init_controllerInfo();

    // non-blocking I/O polling from STDIN
    pfd[0].fd = STDIN_FILENO;
    pfd[0].events = POLLIN;
    pfd[0].revents = 0;

    cout << "checkpoint 2" << endl;
    // init fifo
    for (int i = 0; i < num_of_switch; i++) {
        string inFifo = get_fifo(i+1, 0);
        string outFifo = get_fifo(0, i+1);
        cout << inFifo << endl;
        cout << outFifo << endl;

        mkfifo(inFifo.c_str(),
            S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        if (errno) perror("Error: Could not create a FIFO connection.\n");
        errno = 0;

        mkfifo(outFifo.c_str(),
            S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        if (errno) perror("Error: Could not create a FIFO connection.\n");
        errno = 0;

        int inFd = open(inFifo.c_str(), O_RDONLY | O_NONBLOCK);
        if (errno) perror("Error: Could not open FIFO.\n");
        errno = 0;

        // Returns lowest unused file descriptor on success
        int outFd = open(outFifo.c_str(), O_RDWR | O_NONBLOCK);
        if (errno) perror("Error: Could not open FIFO.\n");
        errno = 0;      

        //cout << inFd << " " << outFd << endl;

        Session session = {inFifo, outFifo, inFd, outFd, 0};
        sessions.push_back(session);

        pfd[i+1].fd = inFd;
        pfd[i+1].events = POLLIN;
        pfd[i+1].revents = 0;
    }

    cout << "checkpoint 3" << endl;

    while(true) {
        
        poll(pfd, num_of_switch + 1, 0);
        char buffer[MAXBUF];
        if (errno) {
            printf("ERROR: Poll failure\n");
            exit(errno);
        }

        if (pfd[0].revents & POLLIN) {
            //ssize_t cmdin = read(pfd[0].fd, buffer, MAXBUF);
            string cmd;
            cin >> cmd;
            //if (!cmdin) {
            //    printf("stdin is closed.\n");
            //}
            //string cmd = string(buffer);
            //while (!cmd.empty() && !isalpha(cmd.back())) cmd.pop_back();
            cout << cmd << endl;

            if (cmd == "info") {
                printInfo();
            } else if (cmd == "exit") {
                printInfo();
                exit(0);
            } else {
                printf("Error: Unrecognized command. Please use info or exit.\n");
            }         
        }

    for (int i = 0; i < num_of_switch; i++) {
        if (pfd[i+1].revents & POLLIN) {
        char buffer[MAXBUF];
        cout << "[cont] Reading from " << sessions[i].inFifo << endl;
        read(sessions[i].inFd, buffer, MAXBUF);
        vector<string> message = parse_message(string(buffer)); // change
        controller_incoming_message_handler(message); // change
        }
    }
    }
}

void printInfo() {
    printf("Switch information: \n");
    for (auto s: switches) {
        printf("HELLO");
        printf("[sw%i]: port1= %i, port2= %i, ipranges= %i-%i\n", s.switchID,
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


void controller_incoming_message_handler(vector<string> message) {
    string type = message[0];
    cout << "Received: (src= sw" << message[1] << ", dest= cont) [" << message[0] << "]" << endl;
    
    if (type == "HELLO") {
    controllerInfo.received[HELLO] += 1;
    int switchID = stoi(message[1]);
    int port1 = stoi(message[2]) <= 0 ? -1 : stoi(message[2]);
    int port2 = stoi(message[3]) <= 0 ? -1 : stoi(message[3]);
    int port3_lo = stoi(message[4]);
    int port3_hi = stoi(message[5]);

    switches.push_back({ switchID, port1, port2, { port3_lo, port3_hi } });
    send_message(sessions[switchID-1].outFd, HELLO_ACK, "HELLO_ACK 0");
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
    send_message(sessions[switchID-1].outFd, ADD, ss.str());
    }
}





// may not be needed
void controller_exit_handler(vector<Session> sessions) {
    for (auto session : sessions) {
    close(session.inFd);
    close(session.outFd);
    }
    exit(EXIT_SUCCESS);
}

PacketInfo init_controllerInfo() {
    return stats;
}


