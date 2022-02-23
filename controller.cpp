#include "controller.h"

#include <algorithm>
#include <assert.h>
#include <iostream>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/signalfd.h>
#include <sys/types.h>
#include <tuple>
#include <unistd.h>

#include <poll.h>

using namespace std;


void controller(int switchNum) {
    pollfd pfd[switchNum+1];
    
}