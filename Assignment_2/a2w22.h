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
  ACTION_DELIVER,
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

struct Session {
  std::string inFifo;
  std::string outFifo;
  int inFd;
  int outFd;
  int port;
};

struct Packet {
  PktType type;
  std::string meesage;
};

struct PacketInfo {
  std::map<PktType, int> received;
  std::map<PktType, int> transmitted;
};

struct Switch {
  int switchID;
  int port1;
  int port2;
  IPs ipranges;
};

vector<string> parse_message(string);
IPs split_ip(string);
string get_fifo(int, int);
void send_message(int, PktType, string);
string action_type_tostring(ActionType);


#endif