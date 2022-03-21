#ifndef GENERAL_H
#define GENERAL_H

#include <string>
#include <utility>
#include <vector>

using namespace std;

string makeFifoName(int senderId, int receiverId);

pair<string, vector<int>> parsePacket(string &s);

void trim(string &s);

void printPacketMessage(string &direction, int srcId, int destId, string &type, vector<int> msg);

#endif