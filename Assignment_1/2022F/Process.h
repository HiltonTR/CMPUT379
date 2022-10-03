#ifndef PROCESS_H
#define PROCESS_H

#include <cstdio>
#include <cstdlib>
#include <map>
#include <iostream>
#include <vector>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

#define LINE_LENGTH 100
#define MAX_PT_ENTRIES 32
#define MAX_ARGS 7
#define MAX_LENGTH 20


using namespace std;
// here we create a struct to have all the information about the process that we need
// we can then put it as the value in the map and have the key the pid as that's what we search by
struct task {
    pid_t pid;
    string cmd;
    bool running;
};

map<pid_t, task> *getProcessesMap();
task *getProcess(pid_t pid);
void run(vector<string> &tokens);

#endif