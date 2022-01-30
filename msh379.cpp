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

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <iterator>
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
#define MAXTASKS 32
int taskIndex = 0;

struct task {
    int index;
    pid_t pid;
    string cmd;
    bool running;
};

vector<task> allTasks;



void set_cpu_limit() {
    // 1. Set time limit to 10 mins
    rlimit cpuLimit{};
    cpuLimit.rlim_cur = 600;
    cpuLimit.rlim_max = 600;
    setrlimit(RLIMIT_CPU, &cpuLimit);
}

void changeDirectory(string tokens) {
    char cwd[MAXLINE];

    string home = getenv("HOME");
    string delimiter = "/";
    string path1 = tokens.substr(0, tokens.find(delimiter));
    string path = tokens.c_str();

    if (path1 == "$HOME") {
        path.replace(0, 5, home.c_str());
    }

    if (chdir(path.c_str()) < 0) {
        printf("cdir: failed (pathname= %s) \n", path.c_str());
    } else {
        getcwd(cwd, sizeof(cwd));
        printf("cdir: done (pathname= %s) \n", cwd);
    };
}

void printDirectory() {
    char cwd[256];
    getcwd(cwd, sizeof(cwd));
    printf("%s\n", cwd);
}

void listTasks() {
    for (auto &current_task : allTasks) {
        if(current_task.running) {
            printf("%i: (pid: %i cmd:%s)\n", current_task.index, current_task.pid, current_task.cmd.c_str());
        }
    }
}

void run(vector<string> &tokens) {

    if (tokens.size() == 1) {
        printf("missing args\n");
    } else if(tokens.size() > 6) {
        printf("too many args\n");
    } else if (taskIndex > MAXTASKS){
        printf("too many jobs\n");
    } else {
        errno = 0;
        pid_t childPID = fork();
        if (childPID == -1){
            printf("could not fork\n");
            errno = 1;
        } else if (childPID == 0) {
            switch (tokens.size()) {
                case 2:
                    execlp(tokens.at(1).c_str(), tokens.at(1).c_str(), (char *) nullptr);
                    break;
                case 3:
                    execlp(tokens.at(1).c_str(), tokens.at(1).c_str(), tokens.at(2).c_str(), (char *) nullptr);
                    break;
                case 4:
                    execlp(tokens.at(1).c_str(), tokens.at(1).c_str(), tokens.at(2).c_str(),
                           tokens.at(3).c_str(), (char *) nullptr);
                    break;
                case 5:
                    execlp(tokens.at(1).c_str(), tokens.at(1).c_str(), tokens.at(2).c_str(),
                           tokens.at(3).c_str(), tokens.at(4).c_str(), (char *) nullptr);
                    break;
                case 6:
                    execlp(tokens.at(1).c_str(), tokens.at(1).c_str(), tokens.at(2).c_str(),
                           tokens.at(3).c_str(), tokens.at(4).c_str(), tokens.at(5).c_str(), (char *) nullptr);    
                    break;
            }
        }

        if(errno){
            printf("issue running command\n");
            errno = 0;
        } else {
            string cmdStr;
            int length = tokens.size();
            for (int i=1; i < length; i++) {
                cmdStr.append(" " + tokens.at(i));
            }
            cout << cmdStr << endl;
            task newTask;
            newTask.index = taskIndex;
            newTask.pid = childPID;
            newTask.cmd = cmdStr;
            newTask.running = true;
            allTasks.push_back(newTask);
            taskIndex++;
        }

    } 
}



void stopTask(vector<string> &tokens) {
    if (tokens.size() < 2) {
        printf("No task number\n");
    } else if (tokens.size() > 2) {
        printf("Too many args\n");
    } else {
        int taskNo = stoi(tokens.at(1), nullptr, 10);
        if (taskNo < allTasks.size()) {
            kill(allTasks.at(taskNo).pid, SIGSTOP);
            printf("Stopped task: %i\n", taskNo);
        } else {
            printf("Failed to find task: %i - not stopping\n", taskNo);
        }
    }
}

void continueTask(vector<string> &tokens) {
    if (tokens.size() < 2) {
        printf("No task number\n");
    } else if (tokens.size() > 2) {
        printf("Too many args\n");
    } else {
        int taskNo = stoi(tokens.at(1), nullptr, 10);
        if (taskNo < allTasks.size()) {
            kill(allTasks.at(taskNo).pid, SIGCONT);
            printf("Continued task: %i\n", taskNo);
        } else {
            printf("Failed to find task: %i - not continuing\n", taskNo);
        }
    }
}

void terminate(vector<string> &tokens) {

}

void exitLoop() {

}

void quitLoop() {
    printf("Exiting mainloop without terminating head processes\n");
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


    // 3. Run the main loop prompting user to enter in commands
    for(;;) {
        string cmd;

        // clear the input and get the input
        cin.clear();
        printf("msh379 [%u]: ", pid);
        getline(cin, cmd);

        //tokenize the input
        // taken from https://stackoverflow.com/questions/236129/how-do-i-iterate-over-the-words-of-a-string#236803
        istringstream in_string(cmd);
        vector<string> tokens {
            istream_iterator<string>{in_string},
            istream_iterator<string>{}
        };
        if (tokens.empty()) {
            printf("Missing input \n");
        } else if (tokens.at(0) == "cdir"){
            try {
            changeDirectory(tokens.at(1).c_str());
            } catch (const std::out_of_range) {
                printf("not enough arguments for cdir \n");
            }
        } else if (tokens.at(0) == "pdir"){
            printDirectory();
        } else if (tokens.at(0) == "lstasks"){
            listTasks();
        } else if (tokens.at(0) == "run"){
            run(tokens);
        } else if (tokens.at(0) == "stop"){
            stopTask(tokens);
        } else if (tokens.at(0) == "continue"){
            continueTask(tokens);
        } else if (tokens.at(0) == "terminate"){
            printf("temp");
        } else if (tokens.at(0) == "check"){
            printf("temp");
        } else if (tokens.at(0) == "exit"){
            exitLoop();
            break;
        } else if (tokens.at(0) == "quit"){
            quitLoop();
            break;
        } else {
            printf("Invalid Command \n");   
        }
        




    }

    tms end_CPU;
    static clock_t end_time = times(&end_CPU);
    printf("Real time:        %.2f sec.\n", (float)(end_time - start_time)/sysconf(_SC_CLK_TCK));
    printf("User Time:        %.2f sec.\n", (float)(end_CPU.tms_utime - start_CPU.tms_utime)/sysconf(_SC_CLK_TCK));
    printf("Sys Time:         %.2f sec.\n", (float)(end_CPU.tms_stime - start_CPU.tms_stime)/sysconf(_SC_CLK_TCK));
    printf("Child user time:  %.2f sec.\n", (float)(end_CPU.tms_cutime - start_CPU.tms_cutime)/sysconf(_SC_CLK_TCK));
    printf("Child sys time:   %.2f sec.\n", (float)(end_CPU.tms_cstime - start_CPU.tms_cstime)/sysconf(_SC_CLK_TCK));

    return 0;
}

