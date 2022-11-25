#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <string.h> //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h> //close
#include <arpa/inet.h> //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <string>
#include <iostream>
#include <chrono>
#include <ctime>
#include <fstream>
#include <map>

#include "tands.c"

using namespace std;

#define TRUE 1
#define FALSE 0

int server(char *argv[]);

#endif 