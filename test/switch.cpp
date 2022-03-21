#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iterator>
#include <map>
#include <sstream>
#include <tuple>
#include <utility>
#include <vector>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "general.h"

#define PFDS_SIZE 5
#define MASTER_ID 0
#define MAX_IP 1000
#define MAX_BUFFER 1024

using namespace std;
using namespace chrono;

/**
* A struct for storing the switch's packet counts
*/
typedef struct {
    int admit;
    int hello_ack;
    int add;
    int relayIn;
    int open;
    int ask;
    int relayOut;
} SwitchPacketCounts;

/**
* A struct representing a rule in the foward table
*/
typedef struct {
    int srcIpLow;
    int srcIpHigh;
    int destIpLow;
    int destIpHigh;
    string actionType;  // FORWARD, DROP
    int actionVal;
    int pktCount;
} fowardRule;


string makeFifo(int a, int b) {
  return "fifo-" + to_string(a) + "-" + to_string(b);
}

void printInfo(vector<fowardRule> &fowardTable, SwitchPacketCounts &counts) {
  printf("foward table:\n");
  int i = 0;
  for (auto &param : fowardTable) {
    printf("[%i] (srcIp= %i-%i, destIp= %i-%i, ", i, param.srcIpLow,
          param.srcIpHigh, param.destIpLow, param.destIpHigh);
    printf("action= %s:%i, pktCount= %i)\n", param.actionType.c_str(),
          param.actionVal, param.pktCount);
    i++;
  }
  printf("\n");
  printf("Packet Stats:\n");
  printf("\tReceived:    ADMIT:%i, HELLO_ACK:%i, ADD:%i, RELAYIN:%i\n", counts.admit, counts.hello_ack,
        counts.add, counts.relayIn);
  printf("\tTransmitted: OPEN:%i, ASK:%i, RELAYOUT:%i\n", counts.open, counts.ask,
        counts.relayOut);
}

/**
* Main event loop for the switch. Polls all FDs. Sends and receives packets of varying types to
* communicate within the SDN.
*/
void switchLoop(int id, int port1Id, int port2Id, int ipLow, int ipHigh, ifstream &in,
                string &ipAddress, int portNumber) {
    vector<fowardRule> fowardTable; // foward rule table
    fowardTable.push_back({0, MAX_IP, ipLow, ipHigh, "FORWARD", 3, 0}); 

    map<int, int> fdid; // fd id
    map<int, int> swid; // switch id 

    // Counts the number of each type of packet seen
    SwitchPacketCounts counts = {0, //admit
                                0, //hello ack
                                0, //add
                                0, //relay in
                                0, //open
                                0, //ask
                                0}; //relayout

    int socketIndex = PFDS_SIZE - 1;

    char buffer[MAX_BUFFER];
    struct pollfd pfds[PFDS_SIZE];

    // polling
    pfds[0].fd = STDIN_FILENO;
    pfds[0].events = POLLIN;
    pfds[0].revents = 0;

    struct sockaddr_in server {};

    // socket file desc so we can get status
    if ((pfds[socketIndex].fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket() failure");
        exit(errno);
    }
    memset(&server, 0, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_port = htons(portNumber);

    if (connect(pfds[socketIndex].fd, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror("connect() failure");
        exit(errno);
    }

    pair<int, int> masterToFd = make_pair(0, pfds[socketIndex].fd);
    fdid.insert(masterToFd);
    pair<int, int> masterToId = make_pair(0, MASTER_ID);
    swid.insert(masterToId);

    // Send an OPEN packet to the master

    string openString = "HELLO:" + to_string(id) + "," + to_string(port1Id) + "," + to_string(port2Id)
                        + "," + to_string(ipLow) + "," + to_string(ipHigh);
    write(pfds[socketIndex].fd, openString.c_str(), strlen(openString.c_str()));

    // Log the successful transmission
    string direction = "Transmitted";
    string type = "HELLO";
    pair<string, vector<int>> parsedPacket = parsePacket(openString);
    printPacketMessage(direction, id, 0, type, parsedPacket.second);



  counts.open++;

  // Set socket to non-blocking
  if (fcntl(pfds[socketIndex].fd, F_SETFL, fcntl(pfds[socketIndex].fd, F_GETFL) | O_NONBLOCK) < 0) {
    perror("fnctl() failure");
    exit(errno);
  }
    // read fifo for port 1
    if (port1Id != -1) {
        pair<int, int> port1Connection = make_pair(1, port1Id);
        swid.insert(port1Connection);
        string fifoName = makeFifo(port1Id, id);
        mkfifo(fifoName.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        int port1Fd = open(fifoName.c_str(), O_RDONLY | O_NONBLOCK);
        pfds[1].fd = port1Fd;
        pfds[1].events = POLLIN;
        pfds[1].revents = 0;
    }

    // read fifo for port 2
    if (port2Id != -1) {
        pair<int, int> port2Connection = make_pair(2, port2Id);
        swid.insert(port2Connection);
        string fifoName = makeFifo(port2Id, id);
        mkfifo(fifoName.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        int port2Fd = open(fifoName.c_str(), O_RDONLY | O_NONBLOCK);
        pfds[2].fd = port2Fd;
        pfds[2].events = POLLIN;
        pfds[2].revents = 0;
    }

  // Used to keep track of the delay interval of the switch
  int delay = 0;

  bool hello_ackReceived = false;
  bool addReceived = true; // Used to wait for master responses.

  vector<int> closedPorts; // Keep track of which ports are closed

  while (true) {
    /*
    * 1. Read and process a single line from the traffic line (if the EOF has not been reached
    * yet). The switch ignores empty lines, comment lines, and lines specifying other handling
    * switches. A packet header is considered admitted if the line specifies the current switch.
    */
    if (hello_ackReceived && addReceived && delay==0) {
      // Reset delay variables
      delay = 0;

      pair<string, vector<int>> trafficInfo;
      string line;
      if (in.is_open()) {
        if (getline(in, line)) {
            string type;
            vector<int> data;

            int id = 0;
            int srcIp = 0;
            int destIp = 0;

            istringstream iss(line);
            vector<string> tokens{istream_iterator<string>{iss}, istream_iterator<string>{}};

            if (line.length() < 1) {
                continue;
            } else if (line.substr(0, 1) == "#") {
                continue;
            } else {
                id = -1;
                if (tokens[0] != string("null")){
                    id = stoi(string(tokens[0]).substr(3,1)); //
                }
                data.push_back(id);

                if (tokens[1] == "delay") {
                type = "delay";
                int ms = stoi(tokens[2].c_str());
                data.push_back(ms);

                } else {
                type = "action";

                srcIp = stoi(tokens[1].c_str());
                data.push_back(srcIp);

                destIp = stoi(tokens[2].c_str());
                data.push_back(destIp);
                }
            }

          if (type == "action") {
            int trafficId = data[0];
            int srcIp = data[1];
            int destIp = data[2];

            if (id == trafficId) {
              counts.admit++;

              // Handle the packet using the foward table
              bool found = false;
              for (auto &param : fowardTable) {
                if (destIp >= param.destIpLow && destIp <= param.destIpHigh) {
                  found = true;
                  param.pktCount++;
                  if (param.actionType == "DROP") {
                    break;
                  } else if (param.actionType == "FORWARD") {
                    if (param.actionVal != 3) {
                      // Open the FIFO for writing if not done already
                      if (!fdid.count(param.actionVal)) {
                        string relayFifo = makeFifo(id, swid[param.actionVal]);
                        int portFd = open(relayFifo.c_str(), O_WRONLY | O_NONBLOCK);
                        fdid.insert(make_pair(param.actionVal, portFd));
                      }

                      // Ensure switch is not closed before sending
                      if (find(closedPorts.begin(), closedPorts.end(), param.actionVal) == closedPorts.end()) {
                        string relayString = "RELAY:" + to_string(srcIp) + "," + to_string(destIp);
                        write(fdid[param.actionVal], relayString.c_str(), strlen(relayString.c_str()));

                        // Log the successful transmission
                        string direction = "Transmitted";
                        string type = "RELAY";
                        pair<string, vector<int>> parsedPacket = parsePacket(relayString);
                        printPacketMessage(direction, srcIp, destIp, type, parsedPacket.second);
                      }

                      counts.relayOut++;
                    }
                  }

                  break;
                }
              }

              if (!found) {
                string askString = "ASK:" + to_string(srcIp) + "," + to_string(destIp);
                write(fdid[0], askString.c_str(), strlen(askString.c_str()));

                // Log the successful transmission
                string direction = "Transmitted";
                string type = "ASK";
                pair<string, vector<int>> parsedPacket = parsePacket(askString);
                printPacketMessage(direction, id, 0, type, parsedPacket.second);



                addReceived = false;
                counts.ask++;
              }
            }
          } else if (type == "delay") {
                int trafficId = data[0];
                if (id == trafficId) {
                    //https://www.geeksforgeeks.org/time-delay-c/
                    milliseconds ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
                    delay = data[1];
                    printf("Entering a delay period of %i milliseconds.\n", delay);
                }
            } else {
                continue;
            }
        } else {
            in.close();
            }   
        }
    }

    // Poll from all file descriptors
    if (poll(pfds, (nfds_t) PFDS_SIZE, 0) == -1) {
      perror("poll() failure");
      exit(errno);
    }
    // poll for user input
    if (pfds[0].revents & POLLIN) {
        if (!read(pfds[0].fd, buffer, MAX_BUFFER)) {
            printf("Error: stdin closed.\n");
            exit(EXIT_FAILURE);
        }

        string cmd = string(buffer);
        trim(cmd);  // trim whitespace

        if (cmd == "info") {
            printInfo(fowardTable, counts);
        } else if (cmd == "exit") {
            printInfo(fowardTable, counts);
            exit(EXIT_SUCCESS);
        } else {
            printf("Unknown command: <info> or <exit> are valid commands.\n");
        }
    }

    memset(buffer, 0, sizeof(buffer)); // Clear buffer

    // poll for fds
    for (int i = 1; i < PFDS_SIZE; i++) {
        if (pfds[i].revents & POLLIN) {
            if (!read(pfds[i].fd, buffer, MAX_BUFFER)) {
            if (i == socketIndex) {
                printf("master closed. Exiting.\n");
                printInfo(fowardTable, counts);
                exit(errno);
            } else {
                printf("Warning: Connection to sw%i closed.\n", swid[i]);
                close(pfds[i].fd);
                closedPorts.push_back(i);
                continue;
            }
            }

            string packetString = string(buffer);
            pair<string, vector<int>> receivedPacket = parsePacket(packetString);
            string packetType = receivedPacket.first;
            vector<int> pkt = receivedPacket.second;

            // Log the successful received packet
            string direction = "Received";
            printPacketMessage(direction, swid[i], id, packetType, pkt);

            if (packetType == "HELLO_ACK") {
                hello_ackReceived = true;
                counts.hello_ack++;
            } else if (packetType == "ADD") {
                addReceived = true;

                fowardRule newRule;

            if (pkt[0] == 0) {
                newRule = {0, MAX_IP, pkt[1], pkt[2], "DROP", pkt[3], 1};
            } else if (pkt[0] == 1) {
                newRule = {0, MAX_IP, pkt[1], pkt[2], "FORWARD", pkt[3], 1};

                // Open FIFO for writing if not done so already
                if (!fdid.count(pkt[3])) {
                string relayFifo = makeFifo(id, swid[pkt[3]]);
                int portFd = open(relayFifo.c_str(), O_WRONLY | O_NONBLOCK);
                fdid.insert(make_pair(pkt[3], portFd));
                }

                // Ensure switch is not closed before sending
                string relayString = "RELAY:" + to_string(pkt[4]) + "," + to_string(pkt[1]);
                write(fdid[pkt[3]], relayString.c_str(), strlen(relayString.c_str()));

                // Log the successful transmission
                string direction = "Transmitted";
                string type = "RELAY";
                pair<string, vector<int>> parsedPacket = parsePacket(relayString);
                printPacketMessage(direction, pkt[4], pkt[1], type, parsedPacket.second);
            

                counts.relayOut++;
            }

            fowardTable.push_back(newRule);
            counts.add++;
            } else if (packetType == "RELAY") {
            counts.relayIn++;

            // Relay the packet to an adjacent master if the destIp is not meant for this switch
            if (pkt[1] < ipLow || pkt[1] > ipHigh) {
                // Ensure switch is not closed before sending
                if (find(closedPorts.begin(), closedPorts.end(), i) == closedPorts.end()) {
                if (id > i) {
                    string relayString = "RELAY:" + to_string(pkt[0]) + "," + to_string(pkt[1]);
                    write(fdid[1], relayString.c_str(), strlen(relayString.c_str()));

                    // Log the successful transmission
                    string direction = "Transmitted";
                    string type = "RELAY";
                    pair<string, vector<int>> parsedPacket = parsePacket(relayString);
                    printPacketMessage(direction, pkt[0], pkt[1], type, parsedPacket.second);

                    counts.relayOut++;
                } 
                }
            }
            }
        }
    }

    memset(buffer, 0, sizeof(buffer)); // Clear buffer
  }
}