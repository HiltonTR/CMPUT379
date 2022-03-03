#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "a2w22.h"
#include <vector>
#include <string>
using namespace std;

void controller(int);

void controller_exit_handler(vector<Session>);// may not be needed
void printInfo();
void receivedInfo();
void transmittedInfo();

void send_open_to_controller();
void controllerReceive(vector<string>);
Switch find_switch_by_ip(int);

#endif