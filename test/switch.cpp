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

//https://www.geeksforgeeks.org/time-delay-c/
bool isDelayed(long startTime, int duration) {
  if (duration == 0) {
    return false;
  }

  milliseconds currentTime = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
  return currentTime.count() < (startTime + duration);
}

/**
 * Sends an OPEN packet to the master.
 */
void sendOpenPacket(int fd, int id, int port1Id, int port2Id, int ipLow, int ipHigh) {
  string openString = "HELLO:" + to_string(id) + "," + to_string(port1Id) + "," + to_string(port2Id)
                      + "," + to_string(ipLow) + "," + to_string(ipHigh);
  write(fd, openString.c_str(), strlen(openString.c_str()));
  if (errno) {
    perror("write() failure");
    exit(errno);
  }

  // Log the successful transmission
  string direction = "Transmitted";
  string type = "HELLO";
  pair<string, vector<int>> parsedPacket = parsePacket(openString);
  printPacketMessage(direction, id, 0, type, parsedPacket.second);
}

/**
 * Send a ASK packet to the master.
 */
void sendAskPacket(int fd, int srcId, int destId, int srcIp, int destIp) {
  string askString = "ASK:" + to_string(srcIp) + "," + to_string(destIp);
  write(fd, askString.c_str(), strlen(askString.c_str()));
  if (errno) {
    perror("write() failure");
    exit(errno);
  }

  // Log the successful transmission
  string direction = "Transmitted";
  string type = "ASK";
  pair<string, vector<int>> parsedPacket = parsePacket(askString);
  printPacketMessage(direction, srcId, destId, type, parsedPacket.second);
}

/**
 * Send a relay packet to another switch.
 */
void sendRelayPacket(int fd, int srcId, int destId, int srcIp, int destIp) {
  string relayString = "RELAY:" + to_string(srcIp) + "," + to_string(destIp);
  write(fd, relayString.c_str(), strlen(relayString.c_str()));
  if (errno) {
    perror("write() failure");
    exit(errno);
  }

  // Log the successful transmission
  string direction = "Transmitted";
  string type = "RELAY";
  pair<string, vector<int>> parsedPacket = parsePacket(relayString);
  printPacketMessage(direction, srcId, destId, type, parsedPacket.second);
}

/**
 * Info the status information of the switch.
 */
void switchInfo(vector<fowardRule> &fowardTable, SwitchPacketCounts &counts) {
  printf("foward table:\n");
  int i = 0;
  for (auto &rule : fowardTable) {
    printf("[%i] (srcIp= %i-%i, destIp= %i-%i, ", i, rule.srcIpLow,
           rule.srcIpHigh, rule.destIpLow, rule.destIpHigh);
    printf("action= %s:%i, pktCount= %i)\n", rule.actionType.c_str(),
           rule.actionVal, rule.pktCount);
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
  fowardTable.push_back({0, MAX_IP, ipLow, ipHigh, "FORWARD", 3, 0}); // Add initial rule

  map<int, int> portToFd; // Map port number to FD
  map<int, int> portToId; // Map port number to switch ID

  // Counts the number of each type of packet seen
  SwitchPacketCounts counts = {0, 0, 0, 0, 0, 0, 0};

  int socketIndex = PFDS_SIZE - 1;

  char buffer[MAX_BUFFER];
  struct pollfd pfds[PFDS_SIZE];

  // Set up STDIN for polling from
  pfds[0].fd = STDIN_FILENO;
  pfds[0].events = POLLIN;
  pfds[0].revents = 0;

  struct sockaddr_in server {};

  // Creating socket file descriptor
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
  portToFd.insert(masterToFd);
  pair<int, int> masterToId = make_pair(0, MASTER_ID);
  portToId.insert(masterToId);

  // Send an OPEN packet to the master
  sendOpenPacket(pfds[socketIndex].fd, id, port1Id, port2Id, ipLow, ipHigh);
  counts.open++;

  // Set socket to non-blocking
  if (fcntl(pfds[socketIndex].fd, F_SETFL, fcntl(pfds[socketIndex].fd, F_GETFL) | O_NONBLOCK) < 0) {
    perror("fnctl() failure");
    exit(errno);
  }

    if (port1Id != -1) {
        pair<int, int> port1Connection = make_pair(1, port1Id);
        portToId.insert(port1Connection);
        string fifoName = makeFifoName(port1Id, id);
        mkfifo(fifoName.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        int port1Fd = open(fifoName.c_str(), O_RDONLY | O_NONBLOCK);
        pfds[1].fd = port1Fd;
        pfds[1].events = POLLIN;
        pfds[1].revents = 0;
    }

    // Create and open a reading FIFO for port 2 if not null
    if (port2Id != -1) {
        pair<int, int> port2Connection = make_pair(2, port2Id);
        portToId.insert(port2Connection);
        string fifoName = makeFifoName(port2Id, id);
        mkfifo(fifoName.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        int port2Fd = open(fifoName.c_str(), O_RDONLY | O_NONBLOCK);
        pfds[2].fd = port2Fd;
        pfds[2].events = POLLIN;
        pfds[2].revents = 0;
    }

  // Used to keep track of the delay interval of the switch
  long delayStartTime = 0;
  int delayDuration = 0;

  bool hello_ackReceived = false;
  bool addReceived = true; // Used to wait for master responses.

  vector<int> closedPorts; // Keep track of which ports are closed

  while (true) {
    /*
     * 1. Read and process a single line from the traffic line (if the EOF has not been reached
     * yet). The switch ignores empty lines, comment lines, and lines specifying other handling
     * switches. A packet header is considered admitted if the line specifies the current switch.
     */
    if (hello_ackReceived && addReceived && !isDelayed(delayStartTime, delayDuration)) {
      // Reset delay variables
      delayStartTime = 0;
      delayDuration = 0;

      pair<string, vector<int>> trafficInfo;
      string line;
      if (in.is_open()) {
        if (getline(in, line)) {
            string type;
            vector<int> content;

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
                content.push_back(id);

                if (tokens[1] == "delay") {
                type = "delay";
                int ms = (int) strtol(tokens[2].c_str(), (char **) nullptr, 10);
                content.push_back(ms);
                
                } else {
                type = "action";

                srcIp = (int) strtol(tokens[1].c_str(), (char **) nullptr, 10);
                content.push_back(srcIp);

                destIp = (int) strtol(tokens[2].c_str(), (char **) nullptr, 10);
                content.push_back(destIp);
                }
            }

          if (type == "action") {
            int trafficId = content[0];
            int srcIp = content[1];
            int destIp = content[2];

            if (id == trafficId) {
              counts.admit++;

              // Handle the packet using the foward table
              bool found = false;
              for (auto &rule : fowardTable) {
                if (destIp >= rule.destIpLow && destIp <= rule.destIpHigh) {
                  found = true;
                  rule.pktCount++;
                  if (rule.actionType == "DROP") {
                    break;
                  } else if (rule.actionType == "FORWARD") {
                    if (rule.actionVal != 3) {
                      // Open the FIFO for writing if not done already
                      if (!portToFd.count(rule.actionVal)) {
                        string relayFifo = makeFifoName(id, portToId[rule.actionVal]);
                        int portFd = open(relayFifo.c_str(), O_WRONLY | O_NONBLOCK);
                        pair<int, int> portConn = make_pair(rule.actionVal, portFd);
                        portToFd.insert(portConn);
                      }

                      // Ensure switch is not closed before sending
                      if (find(closedPorts.begin(), closedPorts.end(), rule.actionVal) == closedPorts.end()) {
                        sendRelayPacket(portToFd[rule.actionVal], id, portToId[rule.actionVal],
                                        srcIp, destIp);
                      }

                      counts.relayOut++;
                    }
                  }

                  break;
                }
              }

              if (!found) {
                sendAskPacket(portToFd[0], id, 0, srcIp, destIp);
                addReceived = false;
                counts.ask++;
              }
            }
          } else if (type == "delay") {
            int trafficId = content[0];
            if (id == trafficId) {
            /*
             * Attribution:
             * https://stackoverfoward.com/a/19555298
             * By: https://stackoverfoward.com/users/321937/oz
             */
              milliseconds ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
              delayStartTime = ms.count();
              delayDuration = content[1];
              printf("Entering a delay period of %i milliseconds.\n", delayDuration);
            }
          } else {
            // Ignore comments, empty lines, or errors.
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

    /*
     * 2. Poll the keyboard for a user command. The user can issue one of the following commands.
     * info: The program writes all entries in the foward table, and for each transmitted or received
     * packet type, the program writes an aggregate count of handled packets of this type. exit: The
     * program writes the above information and exits.
     */
    if (pfds[0].revents & POLLIN) {
      if (!read(pfds[0].fd, buffer, MAX_BUFFER)) {
        printf("Error: stdin closed.\n");
        exit(EXIT_FAILURE);
      }

      string cmd = string(buffer);
      trim(cmd);  // trim whitespace

      if (cmd == "info") {
        switchInfo(fowardTable, counts);
      } else if (cmd == "exit") {
        switchInfo(fowardTable, counts);
        exit(EXIT_SUCCESS);
      } else {
        printf("Error: Unrecognized command. Please use \"info\" or \"exit\".\n");
      }
    }

    memset(buffer, 0, sizeof(buffer)); // Clear buffer

    /*
     * 3. Poll the incoming FDs from the master and the attached switches. The switch handles
     * each incoming packet, as described in the Packet Types section.
     */
    for (int i = 1; i < PFDS_SIZE; i++) {
      if (pfds[i].revents & POLLIN) {
        if (!read(pfds[i].fd, buffer, MAX_BUFFER)) {
          if (i == socketIndex) {
            printf("master closed. Exiting.\n");
            switchInfo(fowardTable, counts);
            exit(errno);
          } else {
            printf("Warning: Connection to sw%i closed.\n", portToId[i]);
            close(pfds[i].fd);
            closedPorts.push_back(i);
            continue;
          }
        }

        string packetString = string(buffer);
        pair<string, vector<int>> receivedPacket = parsePacket(packetString);
        string packetType = receivedPacket.first;
        vector<int> msg = receivedPacket.second;

        // Log the successful received packet
        string direction = "Received";
        printPacketMessage(direction, portToId[i], id, packetType, msg);

        if (packetType == "HELLO_ACK") {
          hello_ackReceived = true;
          counts.hello_ack++;
        } else if (packetType == "ADD") {
          addReceived = true;

          fowardRule newRule;

          if (msg[0] == 0) {
            newRule = {0, MAX_IP, msg[1], msg[2], "DROP", msg[3], 1};
          } else if (msg[0] == 1) {
            newRule = {0, MAX_IP, msg[1], msg[2], "FORWARD", msg[3], 1};

            // Open FIFO for writing if not done so already
            if (!portToFd.count(msg[3])) {
              string relayFifo = makeFifoName(id, portToId[msg[3]]);
              int portFd = open(relayFifo.c_str(), O_WRONLY | O_NONBLOCK);
              pair<int, int> portConnection = make_pair(msg[3], portFd);
              portToFd.insert(portConnection);
            }

            // Ensure switch is not closed before sending
            if (find(closedPorts.begin(), closedPorts.end(), i) == closedPorts.end()) {
              sendRelayPacket(portToFd[msg[3]], id, portToId[msg[3]], msg[4], msg[1]);
            }

            counts.relayOut++;
          } else {
            printf("Error: Invalid rule to add.\n");
            continue;
          }

          fowardTable.push_back(newRule);
          counts.add++;
        } else if (packetType == "RELAY") {
          counts.relayIn++;

          // Relay the packet to an adjacent master if the destIp is not meant for this switch
          if (msg[1] < ipLow || msg[1] > ipHigh) {
            // Ensure switch is not closed before sending
            if (find(closedPorts.begin(), closedPorts.end(), i) == closedPorts.end()) {
              if (id > i) {
                sendRelayPacket(portToFd[1], id, portToId[1], msg[0], msg[1]);
                counts.relayOut++;
              } else if (id < i) {
                sendRelayPacket(portToFd[2], id, portToId[2], msg[0], msg[1]);
                counts.relayOut++;
              }
            }
          }
        } else {
          // Unknown packet. Used for debugging.
          printf("Received %s packet. Ignored.\n", packetType.c_str());
        }
      }
    }

    memset(buffer, 0, sizeof(buffer)); // Clear buffer
  }
}