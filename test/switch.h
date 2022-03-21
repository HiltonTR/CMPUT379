#ifndef SWITCH_H
#define SWITCH_H

#include <fstream>
#include <tuple>

using namespace std;

void switchLoop(int id, int port1Id, int port2Id, int ipLow, int ipHigh, ifstream &in,
                string &ipAddress, int portNumber);

#endif