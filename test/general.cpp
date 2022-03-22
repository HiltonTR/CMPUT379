#include <algorithm>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <unistd.h>
#include <cstring>
#include <poll.h>
#include <iostream>

using namespace std;


pair<string, vector<int>> readPkt(string &s) {
  string packetType = s.substr(0, s.find(':'));
  vector<int> packetData;
  stringstream ss(s.substr(s.find(':') + 1));
  int i = 0;
  //https://stackoverflow.com/questions/1894886/parsing-a-comma-delimited-stdstring
  while (ss >> i) {
    packetData.push_back(i);
    if (ss.peek() == ','){
      ss.ignore();
    }
  }
  return make_pair(packetType, packetData);
}

/**
 * Print a formatted message based on a transmitted/received packet.
 */
void printPacketMessage(string &direction, int srcId, int destId, string &type, vector<int> msg) {
  string src = "psw" + to_string(srcId);
  string dest = "psw" + to_string(destId);

  string packetString;
  if (type == "HELLO") {
    dest = "master";

    string port1;
    if (msg[1] == -1) {
      port1 = "null";
    } else {
      string foo = to_string(msg[1]);
      port1 = "psw" + foo;
    }

    string port2;
    if (msg[2] == -1) {
      port2 = "null";
    } else {
      string foo = to_string(msg[2]);
      port2 = "psw" + foo;
    }

    packetString = ":\n         (port0= master, port1= "+ port1 + ", port2= " + port2 + ", port3= " +
                   to_string(msg[3]) + "-" + to_string(msg[4]) + ")";
                   
  } else if (type == "HELLO_ACK") {
    src = "master";
    packetString = "";
  } else if (type == "ASK") {
    dest = "master";

    packetString = ":  header= (srcIP= " + to_string(msg[0]) + ", destIP= " + to_string(msg[1]) +
                   ")";
  } else if (type == "ADD") {
    src = "master";

    string action;
    if (msg[0] == 0) {
      action = "DROP";
    } else if (msg[0] == 1) {
      action = "FORWARD";
    }

    packetString = ":\n         (srcIp= 0-1000, destIp= " + to_string(msg[1]) + "-" +
                   to_string(msg[2]) + ", action= " + action + ":" + to_string(msg[3]) +
                   ", pktCount= 0";
  } else if (type == "RELAY") {
    packetString = ":  header= (srcIP= " + to_string(msg[0]) +", destIP= " +
                   to_string(msg[1]) + ")";
  }

  printf("%s (src= %s, dest= %s) [%s]%s\n", direction.c_str(), src.c_str(),
         dest.c_str(), type.c_str(), packetString.c_str());
}
