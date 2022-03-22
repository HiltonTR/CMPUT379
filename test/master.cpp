#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include "general.h"

#define MASTER_ID 0
#define MAX_BUFFER 1024
#define MAX_IP 1000

using namespace std;

/**
 * A struct for storing the Master's packet counts
 */
typedef struct {
    int hello;
    int ask;
    int add;
    int hello_ack;
} MasterPacketCounts;

/**
 * A struct that represents the information of a switch
 */
typedef struct {
    int id;
    int port1Id;
    int port2Id;
    int ipLow;
    int ipHigh;
} SwitchInfo;


void printInfo(vector<SwitchInfo> switchInfoTable, MasterPacketCounts &counts) {
    printf("Switch information:\n");
    for (auto &info : switchInfoTable) {
        printf("[psw%i]: port1= %i, port2= %i, port3= %i-%i\n", info.id, info.port1Id, info.port2Id,
            info.ipLow, info.ipHigh);
    }
    printf("\n");
    printf("Packet stats:\n");
    printf("    Received:    HELLO:%i, ASK:%i\n", counts.hello, counts.ask);
    printf("    Transmitted: HELLO_ACK:%i, ADD:%i\n", counts.hello_ack, counts.add);
}

void MasterLoop(int numSwitches, int portNumber) {
    // Table containing info about opened switches
    vector<SwitchInfo> switchInfoTable;

    // Mapping switch IDs to FDs
    map<int, int> idToFd;

    // Counts of each type of packet seen
    MasterPacketCounts counts = {0, 0, 0, 0};

    // Set up indices for easy reference
    int pfdsSize = numSwitches + 2;
    int mainSocket = pfdsSize - 1;

    struct pollfd pfds[pfdsSize];
    char buffer[MAX_BUFFER];

    // Set up STDIN for polling from
    pfds[0].fd = STDIN_FILENO;
    pfds[0].events = POLLIN;
    pfds[0].revents = 0;

    int sockets[1+numSwitches];
    int socketIndex = 1;
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
        for (int i = 0; i < numSwitches + 2; i++) close(pfds[i].fd);
        exit(errno);
    }

    // max connection requests
    if (listen(pfds[mainSocket].fd, numSwitches) < 0) {
        perror("listen() failed");
        for (int i = 0; i < numSwitches + 2; i++) close(pfds[i].fd);
        exit(errno);
    }

    vector<int> closedSwitches;

    while (true) {
        // poll the user for inputs and lists them out to the forwarding table
        if (poll(pfds, (nfds_t) pfdsSize, 0) == -1) { // Poll from all file descriptors
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
            trim(cmd); // Trim whitespace
            
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
                closedSwitches.push_back(i);
                //continue;
                }

                string packet = string(buffer);
                pair<string, vector<int>> receivedPacket = parsePacket(packet);
                string packetType = get<0>(receivedPacket);
                vector<int> packetMessage = get<1>(receivedPacket);

                // Log the successful received packet
                string direction = "Received";
                printPacketMessage(direction, i, MASTER_ID, packetType, packetMessage);

                // open -> hello
                if (packetType == "HELLO") {
                counts.hello++;
                switchInfoTable.push_back({packetMessage[0], packetMessage[1], packetMessage[2],
                                            packetMessage[3], packetMessage[4]});

                idToFd.insert({i, pfds[i].fd});

                // Ensure switch is not closed before sending
                if (find(closedSwitches.begin(), closedSwitches.end(), i) == closedSwitches.end()) {
                    string helloack = "HELLO_ACK:";
                    write(pfds[i].fd, helloack.c_str(), strlen(helloack.c_str()));

                    // Log the successful packet transmission
                    string direction = "Transmitted";
                    string type = "HELLO_ACK";
                    pair<string, vector<int>> parsedPacket = parsePacket(helloack);
                    printPacketMessage(direction, 0, i, type, parsedPacket.second);
                    
                }
                counts.hello_ack++;
                } else if (packetType == "ASK") {
                counts.ask++;

                int srcIp = packetMessage[0];
                int destIp = packetMessage[1];

                // Check for information in the switch info table
                for (auto &info : switchInfoTable) {
                    if (destIp >= info.ipLow && destIp <= info.ipHigh) {
                    int relayPort = 0;

                    // Ensure switch is not closed before sending
                    string addString = "ADD:" + to_string(1) + "," + to_string(info.ipLow) + "," + to_string(info.ipHigh)
                                        + "," + to_string(relayPort) + "," + to_string(srcIp);
                    write(idToFd[i], addString.c_str(), strlen(addString.c_str()));

                    // Log the successful packet transmission.
                    string direction = "Transmitted";
                    string type = "ADD";
                    pair<string, vector<int>> parsedPacket = parsePacket(addString);
                    printPacketMessage(direction, 0, 1, type, parsedPacket.second);


                    break;
                    }
                }


                counts.add++;
                } 
            }
        }   

        memset(buffer, 0, sizeof(buffer)); // Clear buffer

        
        // 
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