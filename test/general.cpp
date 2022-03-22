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
