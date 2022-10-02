#ifndef COMMANDS_H
#define COMMANDS_H

#include <iostream>
#include <string>
#include <iterator>
#include <sstream>
#include <sys/resource.h>
#include <cstring>
#include <iterator>
#include <vector>
#include <map>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>


#include "Process.h"

#define LINE_LENGTH 100
#define MAX_PT_ENTRIES 32
#define MAX_ARGS 7
#define MAX_LENGTH 20

using namespace std;

void printCPUTime();
int getRunTime(pid_t pid);
void listJobs();
void sleep(vector<string> &tokens);
bool checkTokenSize(vector<string> &tokens);
void functions(string cmd);

#endif