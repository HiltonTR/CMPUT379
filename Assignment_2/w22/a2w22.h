#ifndef A2W22_H
#define A2W22_H

#define MAX_SWITCH 7
#define MAXIP 1000
#define MAXPRI 0
#define MINPRI 4
#define MAXBUF 1024

#include <string>
#include <map>
#include <vector>
#include <algorithm> 

using namespace std;

typedef enum {
  LIST_CMD,
  EXIT_CMD,
  CMD_NOTFOUND
} CMDType;

typedef enum {
  FORWARD,
  DROP
} ActionType;

typedef enum {
  HELLO,
  HELLO_ACK,
  ASK,
  ADD,
  RELAYIN,
  ADMIT
} PktType;

struct IPs {
  int low;
  int high;
};

struct Packet {
  PktType type;
  string meesage;
};

struct PacketInfo {
  map<PktType, int> received;
  map<PktType, int> transmitted;
};

struct Switch {
  int switchID;
  int port1;
  int port2;
  IPs ipranges;
};

IPs split_ip(string);

string nameFifo(int, int);

void sendPacket(int, PktType, string);

#endif