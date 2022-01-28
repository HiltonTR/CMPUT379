/*
# ------------------------------
# starter.cc -- a starter program file for CMPUT 379
#     This file includes the following:
#     - Some commonly used header files
#     - Functions WARNING and FATAL that can be used to report warnings and
#       errors after system calls
#     - A function to clear a VT100 screen (e.g., an xterm) by sending
#        the escape sequence "ESC[2J" followed by "ESC[H"
#     - A function to test a C++ vector whose elements are instances
#       of a struct.
#
#     This program is written largely as a C program, with the exception of
#     using some STL (Standard Template Library) container classes, and
#     possibly the C++ "new" operator.
#     (These two features are probably all we need from C++ in CMPUT 379.)
#    
#  Compile with:  g++ starter.cc -o starter         (no check for warnings)
#  	          g++ -Wall starter.cc -o starter   (check for warnings)
#		  g++ -ggdb starter.cc -o starter   (for debugging with gdb)
#
#  Usage:  starter  stringArg  intArg	 (e.g., starter abcd 100)
#
#  Author: Prof. Ehab Elmallah (for CMPUT 379, U. of Alberta)
# ------------------------------
*/

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cmath>
#include <ctime>
#include <cassert>
#include <cstring>
#include <cstdarg>             //Handling of variable length argument lists
#include <sys/times.h>

#include <sstream>
#include <signal.h>
#include <stdlib.h>
#include <tuple>
#include <algorithm>

#include <unistd.h>		
#include <sys/types.h>
#include <sys/stat.h>	
#include <sys/wait.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

// ------------------------------

#define MAXLINE 256

// ------------------------------------------------------------
// clrScreen() -- send VT100 escape sequence to clear the screen
// 	          (works fine in some cases)

void clrScreen () {
     char   ESC=033;		// the ESC character
     printf ("%c[2J", ESC); printf ("%c[H", ESC);
}

// ------------------------------------------------------------


void set_cpu_limit() {
    // 1. Set time limit to 10 mins
    rlimit cpuLimit{};
    cpuLimit.rlim_cur = 600;
    cpuLimit.rlim_max = 600;
    setrlimit(RLIMIT_CPU, &cpuLimit)
}

// ------------------------------ 
int main(int argc, char *argv[]) {
    // Set time limit to 10 mins
    set_cpu_limit();

    // 2. Call function times() to get CPU times
    tms start_CPU;

    static clock_t start_time = times(&start_CPU);

    // gets pid of process
    pid_t pid = getpid();






    tms end_CPU;
    static clock_t end_time = times(&end_CPU)
    printf("Real time:        %.2f sec.\n", (float)(end_time - start_time)/sysconf(_SC_CLK_TCK));
    printf("User Time:        %.2f sec.\n", (float)(end_CPU.tms_utime - start_CPU.tms_utime)/sysconf(_SC_CLK_TCK));
    printf("Sys Time:         %.2f sec.\n", (float)(end_CPU.tms_stime - start_CPU.tms_stime)/sysconf(_SC_CLK_TCK));
    printf("Child user time:  %.2f sec.\n", (float)(end_CPU.tms_cutime - start_CPU.tms_cutime)/sysconf(_SC_CLK_TCK));
    printf("Child sys time:   %.2f sec.\n", (float)(end_CPU.tms_cstime - start_CPU.tms_cstime)/sysconf(_SC_CLK_TCK));

    return 0;
}

