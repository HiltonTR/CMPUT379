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

void switchLoop(int id, int port1ID, int ipLow, int ipHigh, ifstream &in, string &ipAddress, int portNumber) {
    vector<forwardRule> forwardTable;
    fowardTable.push_back({0, MAX_IP, ipLow, ipHigh, "FORWARD", 3, 0}); 

    map<int, int> portToFd; // Map port number to FD
    map<int, int> portToId; // Map port number to switch ID

    // count each type of packet

    SwitchPacketCounts counts = {0, 0, 0, 0, 0, 0, 0};

    int socketIndex = PFDS_SIZE - 1;

    char buffer[MAX_BUFFER];
    struct pollfd pfds[PFDS_SIZE];

    // Set up STDIN for polling from
    pfds[0].fd = STDIN_FILENO;
    pfds[0].events = POLLIN;
    pfds[0].revents = 0;

    // start of socket stuff
    struct sockaddr_in server {};

    // Creating socket file descriptor
    if ((pfds[socketIndex].fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket() failed");
        exit(errno);
    }
    memset(&server, 0, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_port = htons(portNumber);

    if (connect(pfds[socketIndex].fd, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror("connect() failed");
        exit(errno);
    }

    pair<int, int> masterToFd = make_pair(0, pfds[socketIndex].fd);
    portToFd.insert(masterToFd);
    pair<int, int> masterToId = make_pair(0, master_ID);
    portToId.insert(masterToId);

    // Send an OPEN packet to the master
    sendOpenPacket(pfds[socketIndex].fd, id, port1Id, port2Id, ipLow, ipHigh);
    counts.open++;

    // Set socket to non-blocking
    if (fcntl(pfds[socketIndex].fd, F_SETFL, fcntl(pfds[socketIndex].fd, F_GETFL) | O_NONBLOCK) < 0) {
        perror("fnctl() failure");
        exit(errno);
    }

    // end of socket stuff
    pair<int, int> masterToFd = make_pair(0, pfds[socketIndex].fd);
    portToFd.insert(masterToFd);
    pair<int, int> masterToId = make_pair(0, master_ID);
    portToId.insert(masterToId);
    // FIFO stuff 
    // here we will create reading fifos for the ports

    if (port1Id != -1) {
        pair<int, int> port1Connection = make_pair(1, port1Id);
        portToId.insert(port1Connection);
        string fifoName = makeFifoName(port1ID, id);
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
        string fifoName = makeFifoName(port2ID, id);
        mkfifo(fifoName.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        int port2Fd = open(fifoName.c_str(), O_RDONLY | O_NONBLOCK);
        pfds[2].fd = port2Fd;
        pfds[2].events = POLLIN;
        pfds[2].revents = 0;
    }
}