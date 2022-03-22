#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>
#include <algorithm>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>
#include <iostream>
#include "general.h"

#define MASTER_ID 0
#define MAX_BUFFER 1024
#define MAX_IP 1000

using namespace std;

// struct to hold the values of each packet attribute
typedef struct {
    int hello;
    int ask;
    int add;
    int hello_ack;
} MasterPacketCounts;

// struct to define values of a switch
typedef struct {
    int id;
    int port1Id;
    int port2Id;
    int ipLow;
    int ipHigh;
} SwitchInfo;

// prints information of the switch out nicely
void printInfo(vector<SwitchInfo> switchInfoTable, MasterPacketCounts &counts) {
    printf("Switch information:\n");
    for (auto &info : switchInfoTable) {
        printf("[psw%i]: port1= %i, port2= %i, port3= %i-%i\n", info.id, info.port1Id, info.port2Id, info.ipLow, info.ipHigh);
    }
    printf("\n");
    printf("Packet stats:\n");
    printf("    Received:    HELLO:%i, ASK:%i\n", counts.hello, counts.ask);
    printf("    Transmitted: HELLO_ACK:%i, ADD:%i\n", counts.hello_ack, counts.add);
}

void MasterLoop(int numSwitches, int portNumber) {
    char buffer[MAX_BUFFER];
    // Table containing info about opened switches
    vector<SwitchInfo> switchInfoTable;

    // Mapping switch IDs to FDs
    map<int, int> idToFd;

    // Counts of each type of packet seen
    MasterPacketCounts counts = {0, //hello
                                0, //ask
                                0, //add
                                0}; //hello ack

    // define variables so we dont have to redfine them everytime they're needed
    int pfdsSize = numSwitches + 2;
    struct pollfd pfds[pfdsSize];
    int mainSocket = pfdsSize - 1;
    int sockets[1+numSwitches];
    int socketIndex = 1;

    // Set up STDIN for polling from
    pfds[0].fd = STDIN_FILENO;
    pfds[0].events = POLLIN;
    pfds[0].revents = 0;


    // https://www.gta.ufrj.br/ensino/eel878/sockets/sockaddr_inman.html 
    struct sockaddr_in sin {};
    struct sockaddr_in foo {};
    socklen_t sinLength = sizeof(sin);
    socklen_t fooSize = sizeof(foo);

    // create managing socket
    // referenced: https://www.geeksforgeeks.org/socket-programming-cc/ 
    if ((pfds[mainSocket].fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error in creating socket.\n");
        for (int i = 0; i < numSwitches + 2; i++) close(pfds[i].fd);
        exit(errno);
    }

    pfds[mainSocket].events = POLLIN;
    pfds[mainSocket].revents = 0;

    // Bind managing socket to a name
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(portNumber);
    
    if (bind(pfds[mainSocket].fd, (struct sockaddr *) &sin, sinLength) < 0) {
        perror("bind() failed");
        for (int i = 0; i < numSwitches + 2; i++) {
            close(pfds[i].fd);
        }
        exit(errno);
    }

    // max connection requests
    if (listen(pfds[mainSocket].fd, numSwitches) < 0) {
        perror("listen() failed");
        for (int i = 0; i < numSwitches + 2; i++) {
            close(pfds[i].fd);
        }
        exit(errno);
    }


    while (true) {
        // poll the user for inputs and lists them out to the forwarding table
        if (poll(pfds, (nfds_t) pfdsSize, 0) == -1) { 
            perror("poll() failed");
            for (int i = 0; i < numSwitches + 2; i++) close(pfds[i].fd);
            exit(errno);
        }
        if (pfds[0].revents & POLLIN) {
            if (!read(pfds[0].fd, buffer, MAX_BUFFER)) {
                printf("Stdin is closed.\n");
                exit(EXIT_FAILURE);
            }

            string cmd = string(buffer);
            // string trim taken from
            // https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
            cmd.erase(cmd.begin(), find_if(cmd.begin(), cmd.end(), [](int ch) { return !isspace(ch); }));
            cmd.erase(find_if(cmd.rbegin(), cmd.rend(), [](int ch) { return !isspace(ch); }).base(), cmd.end());
            
            if (cmd == "info") {
                printInfo(switchInfoTable, counts);
            } else if (cmd == "exit") {
                printInfo(switchInfoTable, counts);
                for (int i = 0; i < numSwitches + 2; i++) close(pfds[i].fd);
                exit(EXIT_SUCCESS);
            } else {
                printf("Unknown command: <info> or <exit> are valid commands.\n");
            }
        }

        memset(buffer, 0, sizeof(buffer)); // Clear buffer

        // poll for fds
        for (int i = 1; i <= numSwitches; i++) {
            if (pfds[i].revents & POLLIN) {
                // check connection
                if (!read(pfds[i].fd, buffer, MAX_BUFFER)) {
                printf("Lost connection to sw%d.\n", i);
                close(pfds[i].fd);
                //continue;
                }

                string packet = string(buffer);
                string pktType = readPkt(packet).first;
                vector<int> pktMsg = readPkt(packet).second;

                string port1;
                if (pktMsg[1] == -1) {
                    port1 = "null";
                } else {
                    string foo = to_string(pktMsg[1]);
                    port1 = "psw" + foo;
                }

                string port2;
                if (pktMsg[2] == -1) {
                    port2 = "null";
                } else {
                    string foo = to_string(pktMsg[2]);
                    port2 = "psw" + foo;
                }

                cout << "Received (src= psw" << i << ", dest= master) [HELLO]:" << endl;
                cout << "\t (port0= master, port1= "<< port1 <<", port2= "<< port2 <<", port3= "<< pktMsg[3] <<"-"<< pktMsg[4]<< ")" << endl;          

                if (pktType == "HELLO") {
                counts.hello++;
                switchInfoTable.push_back({pktMsg[0], pktMsg[1], pktMsg[2], pktMsg[3], pktMsg[4]});
                idToFd.insert({i, pfds[i].fd});

                string helloack = "HELLO_ACK:";
                write(pfds[i].fd, helloack.c_str(), strlen(helloack.c_str()));
                cout << "Transmitted (src= master" << ", dest= psw" << i << ") [HELLO_ACK]" << endl;
                
                counts.hello_ack++;
                } else if (pktType == "ASK") {
                counts.ask++;

                int srcIp = pktMsg[0];
                int destIp = pktMsg[1];

                // Check for information in the switch info table
                for (auto &info : switchInfoTable) {
                    if (destIp >= info.ipLow && destIp <= info.ipHigh) {
                    int relayPort = 0;


                    string addString = "ADD:" + to_string(1) + "," + to_string(info.ipLow) + "," + to_string(info.ipHigh) + "," + to_string(relayPort) + "," + to_string(srcIp);
                    write(idToFd[i], addString.c_str(), strlen(addString.c_str()));

                    cout << "Transmitted (src= psw" << 0 << ", dest= psw" << 1 << ") [ADD]:" << endl;
                    cout <<"\t (srcIp= 0-1000, destIp= " << readPkt(addString).second[1] << "-" << readPkt(addString).second[2] << "," << ", action= " << 
                            readPkt(addString).second[3] << ", pktCount= 0" << endl;


                    break;
                    }
                }


                counts.add++;
                } 
            }
        }   

        memset(buffer, 0, sizeof(buffer)); // Clear buffer

        
        // poll socket for info
        if (pfds[mainSocket].revents & POLLIN) {
            if ((sockets[socketIndex] = accept(pfds[mainSocket].fd, (struct sockaddr *) &foo, &fooSize)) < 0) {
                perror("accept() failed");
                for (int i = 0; i < numSwitches + 2; i++) close(pfds[i].fd);
                exit(errno);
            }
            pfds[socketIndex].fd = sockets[socketIndex];
            pfds[socketIndex].events = POLLIN;
            pfds[socketIndex].revents = 0;

            // non-blocking socket
            if (fcntl(pfds[socketIndex].fd, F_SETFL, fcntl(pfds[socketIndex].fd, F_GETFL) | O_NONBLOCK) < 0) {
                perror("fcntl() failure");
                exit(errno);
            }

            socketIndex++;
        }

        memset(buffer, 0, sizeof(buffer)); // Clear buffer


    }

}