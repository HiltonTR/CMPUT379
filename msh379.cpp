/*
# ------------------------------
# Author: Hilton Truong
# CCID: truong
# ID: 1615505
# Assignment 1 - msh.cpp -- a simple shell program for CMPUT 379
#     This file includes the following:
#     - Some commonly used header files
#     - functions to change the path, print the directory, list tasks
        run programs with up to 4 arguments, stop tasks, continue tasks
        terminate tasks, check the target task and returns information,
        exitting the program after terminating everything, and quitting 
        the program without terminating any heads,
#
#     This program is written largely as a C++ program
#    
#     File comes with a makefile and can be compiled by running make all
#     then ./msh379 to run.
#     
#     TO RUN PROGRAMS WITHOUT ./ DO:
#       PATH=$PATH:.
#       source ~/.bashrc
#
#     Everything else will be detailed in the report.
#
# ------------------------------
*/

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <sys/times.h>
#include <iterator>
#include <sstream>
#include <signal.h>
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

// create a struct to store information for each task
struct task {
    int index;
    pid_t pid;
    string cmd;
    bool running;
};
// stores all tasks
vector<task> allTasks;

// create a struct to store the parent process
struct parent {
    string user;
    string pid;
    string ppid;
    string state;
    string start;
    string cmd;
    string others;
};
// stores all processes
vector<parent> processes;


void set_cpu_limit(int time) {
    // 1. Set time limit to 10 mins
    rlimit cpuLimit{};
    cpuLimit.rlim_cur = 60*time;
    cpuLimit.rlim_max = 60*time;
    setrlimit(RLIMIT_CPU, &cpuLimit);
}

// Implementation for cdir
void changeDirectory(string tokens) {
    // defines values to get the home directory
    char cwd[MAXLINE];
    string home = getenv("HOME");
    // split the input string by /
    string delimiter = "/";
    string path1 = tokens.substr(0, tokens.find(delimiter));
    string path = tokens.c_str();
    // if the first value is $HOME replace it by the directory path
    if (path1 == "$HOME") {
        path.replace(0, 5, home.c_str());
    }
    // changes directory with chdir based on the output
    if (chdir(path.c_str()) < 0) {
        printf("cdir: failed (pathname= %s) \n", path.c_str());
    } else {
        getcwd(cwd, sizeof(cwd));
        printf("cdir: done (pathname= %s) \n", cwd);
    };
}

// Implementation for pdir
void printDirectory() {
    // prints the current working directory
    char cwd[256];
    getcwd(cwd, sizeof(cwd));
    printf("%s\n", cwd);
}

// Implementation for lstasks
void listTasks() {
    // loops through all the current running tasks and prints their information out
    // Inspiration to loop taken from: 
    // https://stackoverflow.com/questions/12702561/iterate-through-a-c-vector-using-a-for-loop 
    for (auto &current_task : allTasks) {
        if(current_task.running) {
            printf("%i: (pid: %i cmd:%s)\n", current_task.index, 
                   current_task.pid, current_task.cmd.c_str());
        }
    }
}

// Implementation for run pgm args*
void run(vector<string> &tokens) {
    // checks and makes the input is proper
    if (tokens.size() == 1) {
        printf("Error: Missing args\n");
    } else if(tokens.size() > 6) {
        printf("Error: Too many args\n");
    } else if (taskIndex > MAXTASKS){
        printf("Error: Too many jobs\n");
    } else {
        // define errno as 0. If there is an error, errno will change
        // and it will break and display that there is an error.
        errno = 0;
        // Creates a fork and makes sure the fork executes
        pid_t childPID = fork();
        if (childPID == -1){
            printf("Error: Could not fork\n");
            errno = 1;
        } else if (childPID == 0) {
            // if the fork forks properly, it will take the input token
            // and checks the amount of arguments it has and runs it with
            // execlp with up to 4 arguments after the program
            switch (tokens.size()) {
                case 2:
                    execlp(tokens.at(1).c_str(), tokens.at(1).c_str(), 
                           (char *) nullptr);
                    break;
                case 3:
                    execlp(tokens.at(1).c_str(), tokens.at(1).c_str(), 
                           tokens.at(2).c_str(), (char *) nullptr);
                    break;
                case 4:
                    execlp(tokens.at(1).c_str(), tokens.at(1).c_str(), 
                           tokens.at(2).c_str(), tokens.at(3).c_str(), 
                           (char *) nullptr);
                    break;
                case 5:
                    execlp(tokens.at(1).c_str(), tokens.at(1).c_str(), 
                           tokens.at(2).c_str(), tokens.at(3).c_str(), 
                           tokens.at(4).c_str(), (char *) nullptr);
                    break;
                case 6:
                    execlp(tokens.at(1).c_str(), tokens.at(1).c_str(), 
                           tokens.at(2).c_str(), tokens.at(3).c_str(), 
                           tokens.at(4).c_str(), tokens.at(5).c_str(), 
                           (char *) nullptr);    
                    break;
            }
        }
        // If there is an error, errno will flag
        if(errno){
            printf("Error: Issue running command\n");
            errno = 0;
        } else {
            // If everything runs successfully, create a new Task struct
            // and append the information about the task that was just 
            // created to it
            string cmdStr;
            int length = tokens.size();
            // concatenates cmd to one string
            for (int i=1; i < length; i++) {
                cmdStr.append(" " + tokens.at(i));
            }
            task newTask;
            newTask.index = taskIndex;
            newTask.pid = childPID;
            newTask.cmd = cmdStr;
            newTask.running = true;
            // add it to the list of all tasks
            allTasks.push_back(newTask);
            // increase the task counter
            taskIndex++;
        }

    } 
}

// Implementation for stop
void stopTask(vector<string> &tokens) {
    // checks and makes the input is proper
    if (tokens.size() < 2) {
        printf("Error: No task number\n");
    } else if (tokens.size() > 2) {
        printf("Error: Too many args\n");
    } else {
        // if the given task number is less than all current tasks 
        // it will send the stop signal and stop the task otherwise
        // it will print an error
        int taskNo = stoi(tokens.at(1));
        if (taskNo < allTasks.size()) {
            kill(allTasks.at(taskNo).pid, SIGSTOP);
            printf("Stopped task: %i\n", taskNo);
        } else {
            printf("Error: Can't find task: %i - not stopping\n", taskNo);
        }
    }
}

// Implementation for continue
void continueTask(vector<string> &tokens) {
    // checks and makes the input is proper
    if (tokens.size() < 2) {
        printf("Error: No task number\n");
    } else if (tokens.size() > 2) {
        printf("Error: Too many args\n");
    } else {
        // if the given task number is less than all current tasks 
        // it will send the continue signal and stop the task otherwise
        // it will print an error
        int taskNo = stoi(tokens.at(1));
        if (taskNo < allTasks.size()) {
            kill(allTasks.at(taskNo).pid, SIGCONT);
            printf("Continued task: %i\n", taskNo);
        } else {
            printf("Error: Can't find task: %i - not continuing\n", taskNo);
        }
    }
}

// Implementation for terminate
void terminate(vector<string> &tokens) {
    // checks and makes the input is proper
    if (tokens.size() < 2) {
        printf("Error: No task number\n");
    } else if (tokens.size() > 2) {
        printf("Error: Too many args\n");
    } else {
        // if the given task number is less than all current tasks 
        // it will send the kill signal if the task is currently 
        // not running and stop the task otherwise
        // it will print an error
        int taskNo = stoi(tokens.at(1));
        if (taskNo < allTasks.size()) {
            if(allTasks.at(taskNo).running == false) {
                printf("Error: Task %i is not running\n", taskNo);
                return;
            }
            kill(allTasks.at(taskNo).pid, SIGKILL);
            allTasks.at(taskNo).running = false;
            printf("Terminated task: %i\n", taskNo);
        } else {
            printf("Error: Can't find task: %i - not terminating\n", taskNo);
        }
    }    
}

// helper function for check 
parent processBuffer(const string &buffer) {
    // creates a parent struct to store the information obtained from popen
    parent newParent;
    stringstream sStream(buffer);
    string tokens;
    int i = 0;
    // splits the input into tokens and stores the values in the struct
    while (getline(sStream, tokens, ' ')) {
        if (tokens.length()) {
            if (i == 0) {
                newParent.user = tokens.c_str();
            } else if (i == 1) {
                newParent.pid = tokens.c_str();
            } else if (i == 2) {
                newParent.ppid = tokens.c_str();
            } else if (i == 3) {
                newParent.state = tokens.c_str();
            } else if (i == 4) {
                newParent.start = tokens.c_str();
            } else if (i == 5) {
                newParent.cmd = tokens.c_str();
            } else {
                newParent.others.append(tokens.c_str());               
            }
            i++;
        }
    }

    return newParent;
}

// helper function for check
vector<parent> getParent(vector<string> &tokens) {
    // this function gets the parent process (and children) from popen
    vector<parent> output;
    char buffer[256];
    // calls popen and as long as the pipe is open it takes the values and calls
    // processBuffer to store process then stores them into a vector of type parent
    // and returns it after closing the pipe.
    FILE* pipe = popen("ps -u $USER -o user,pid,ppid,state,start,cmd --sort start", "r");
    if (pipe) {
        while (!feof(pipe)) {
            if (fgets(buffer, 256, pipe) != NULL) {
                parent newParent = processBuffer(buffer);
                output.push_back(newParent);
            }
        }
    pclose(pipe);
    }
    return output;
}

// helper function for check
void getState(string targetPID) {
    // this goes through all the current processes and gets the state of the process from
    // the given targetPID
    for (auto &currentProcess : processes) {
        if (currentProcess.pid == targetPID) {
            // if it is defunct, print the information of the process
            if(currentProcess.others.find("defunct") != string::npos) {
                printf("    target_pid= %s terminated\n", currentProcess.pid.c_str());
                printf("    USER       PID    PPID S  STARTED CMD\n");
                printf("    %s %s %s %s %s %s %s", currentProcess.user.c_str(), 
                currentProcess.pid.c_str(), currentProcess.ppid.c_str(), 
                currentProcess.state.c_str(), currentProcess.start.c_str(), 
                currentProcess.cmd.c_str(), currentProcess.others.c_str());
            } else {
                // if it is not defunct, print the information of the process
                printf("    target_pid= %s is running:\n", currentProcess.pid.c_str());
                printf("    USER       PID    PPID S  STARTED CMD\n");
                printf("    %s %s %s %s %s %s %s", currentProcess.user.c_str(), 
                currentProcess.pid.c_str(), currentProcess.ppid.c_str(), 
                currentProcess.state.c_str(), currentProcess.start.c_str(), 
                currentProcess.cmd.c_str(), currentProcess.others.c_str());
            }
        }
    }
}
// helper function for check
void get_c_processes(string targetPID){
    // this goes through all the processes and finds the children procceses
    // from the given targetPID and recursively calls itself to print the
    // descendants of the children if it has any
    for (auto &currentProcess : processes) {
        if (currentProcess.ppid == targetPID) {
            printf("    %s %s %s %s %s %s %s", currentProcess.user.c_str(), 
            currentProcess.pid.c_str(), currentProcess.ppid.c_str(), 
            currentProcess.state.c_str(), currentProcess.start.c_str(), 
            currentProcess.cmd.c_str(), currentProcess.others.c_str());
            get_c_processes(currentProcess.pid);
        }
    }

}

// Implementation of check
void check(vector<string> &tokens) {
    // checks and makes the input is proper
    if (tokens.size() < 2) {
        printf("Error: No task number\n");
    } else if (tokens.size() > 2) {
        printf("Error: Too many args\n");
    } else {
        // gets all the processes then get the state of the desired process
        // and print the descendant processes if it is not defunct
        string targetPID = tokens.at(1).c_str();
        processes = getParent(tokens);
        getState(targetPID);
        get_c_processes(targetPID);
    }
}

// Implementation for exit
void exitLoop() {
    // checks all the current tasks that are running and terminates
    // then prints them out before quitting program
    for (auto &currentTask : allTasks) {
        if (currentTask.running) {
            kill(currentTask.pid, SIGKILL);
            printf("Task: %i terminated\n", currentTask.pid);
        }
    }
}

// Implementation for quit
void quitLoop() {
    printf("WARNING: Exiting mainloop without terminating head processes\n");
}

// ------------------------------ 
int main(int argc, char *argv[]) {
    // Set time limit to 10 mins
    set_cpu_limit(10);

    // 2. Call function times() to get CPU times
    tms start_CPU, end_CPU;

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

        // tokenize the input
        // taken from https://stackoverflow.com/questions/236129/how-do-i-iterate-over-the-words-of-a-string#236803
        istringstream in_string(cmd);
        vector<string> tokens {
            istream_iterator<string>{in_string},
            istream_iterator<string>{}
        };

        // processes the input for the possible commands that the shell can take
        if (tokens.empty()) {
            printf("Missing input \n");
        } else if (tokens.at(0) == "cdir"){ // change directory 
            try {
            changeDirectory(tokens.at(1).c_str());
            } catch (const std::out_of_range) { // makes sure there are enough args
                printf("not enough arguments for cdir \n");
            }
        } else if (tokens.at(0) == "pdir"){ // print current working directory
            printDirectory();
        } else if (tokens.at(0) == "lstasks"){ // lists all running tasks
            listTasks();
        } else if (tokens.at(0) == "run"){ // run a program withup to 4 args
            run(tokens);
        } else if (tokens.at(0) == "stop"){ // stop a task
            stopTask(tokens);
        } else if (tokens.at(0) == "continue"){ // continue a task
            continueTask(tokens);
        } else if (tokens.at(0) == "terminate"){ // terminate a task
            terminate(tokens);
        } else if (tokens.at(0) == "check"){ // check the information of a task with ps
            check(tokens);
        } else if (tokens.at(0) == "exit"){ // quit the shell terminating all heads
            exitLoop();
            break;
        } else if (tokens.at(0) == "quit"){ // quit without terminating anything
            quitLoop();
            break;
        } else {
            printf("Invalid Command \n"); // not a valid command
        }

    }
    // print the times
    static clock_t end_time = times(&end_CPU);
    printf("Real:        %.2f sec.\n", (float)(end_time - start_time)/sysconf(_SC_CLK_TCK));
    printf("User:        %.2f sec.\n", (float)(end_CPU.tms_utime - start_CPU.tms_utime)/sysconf(_SC_CLK_TCK));
    printf("Sys:         %.2f sec.\n", (float)(end_CPU.tms_stime - start_CPU.tms_stime)/sysconf(_SC_CLK_TCK));
    printf("Child user:  %.2f sec.\n", (float)(end_CPU.tms_cutime - start_CPU.tms_cutime)/sysconf(_SC_CLK_TCK));
    printf("Child sys:   %.2f sec.\n", (float)(end_CPU.tms_cstime - start_CPU.tms_cstime)/sysconf(_SC_CLK_TCK));

    return 0;
}

