#ifndef SWITCH_H
#define SWITCH_H

#include "a2w22.h"
#include <string>
#include <vector>

struct TrafficRule {
  int switchID;
  int src_ip;
  int dest_ip;
};

struct ft {
  IPs src_range;
  IPs dest_range;
  ActionType actionType;
  int action_value; // the switch port to which the packet should be forwarded
  int pktCount; // packet count
};


extern int switchID;

void masterSwitch(int, int, int, string, IPs);
vector<ft> init_forwardingTable(IPs);
vector<Session> init_session(int, int, int);
void switch_list_handler();
void print_forwardingTable();
void send_open_to_controller(int, int, int);
PacketInfo init_switch_stats();
void print_switch_stats();
void switchReceive(vector<string>);
vector<string> cache_traffic_file(string);
TrafficRule parse_traffic_rule(string);
void add_drop_rule(int);
int add_forward_rule(int, IPs);
int find_matching_rule(int);
int find_matching_session(int);

#endif