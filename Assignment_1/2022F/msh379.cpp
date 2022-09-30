/*
# ------------------------------
# Author: Hilton Truong
# CCID: truong
# ID: 1615505
# Assignment 1 - msh.cpp -- a simple shell program for CMPUT 379
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
#include <sys/types.h>
#include <sys/wait.h>
#include <map>

#include <assert.h> // assert
using namespace std;

// ------------------------------

#define LINE_LENGTH 100
#define MAX_PT_ENTRIES 32
#define MAX_ARGS 7
#define MAX_LENGTH 20

struct task {
    pid_t pid;
    string cmd;
    bool running;
};

map<pid_t, task> processes = map<pid_t, task>();

// https://stackoverflow.com/questions/20167685/measuring-cpu-time-in-c
void printCPUTime() {
    struct rusage r;

    getrusage(RUSAGE_SELF, &r);

    printf("CPU time: %lu s\nSys time: %lu s\n", r.ru_utime.tv_sec, r.ru_stime.tv_sec);

    return;
}

// https://unix.stackexchange.com/questions/7870/how-to-check-how-long-a-process-has-been-running
int getRunTime(pid_t pid) {
    int runTime;
    char buffer[100];
    FILE *fp;

    sprintf(buffer, "ps -p %d -o times=", pid);
    fp = popen(buffer, "r");
    fscanf(fp, "%d", &runTime);
    pclose(fp);

    return runTime;
}

void listJobs() {
    printf("Running processes: \n");

    if (!processes.empty()){
        printf(" #      PID S SEC COMMAND\n");
        int index = 0;

        for (auto &item : processes) {
            task currentTask = item.second;
            cout << "current : "<< currentTask.cmd << endl;

            int runTime = getRunTime(currentTask.pid);
            printf(
                    "%2d: %7d %i %3d %s\n",
                    index,
                    currentTask.pid,
                    currentTask.running,
                    runTime, 
                    currentTask.cmd.c_str());

            index++;
        }
    }
    printf("Processes = %lu active \n", processes.size());
    printf("Completed processes: \n");
    printCPUTime();
    return;
}

void sleep(vector<string> &tokens) {
    if (tokens.size() < 2) {
        printf("Missing time argument. \n");
        return;
    }
    sleep(stoi(tokens.at(1)));
    return;
}

bool checkTokenSize(vector<string> &tokens){
    if (tokens.size() > MAX_ARGS) {
        printf("Too many arguments. \n");
        return false;
    }

    for (int i = 0; i < tokens.size(); i++) {
        string token = tokens.at(i);

        if (token.length() > MAX_LENGTH) {
            printf("Argument is too long. \n");
            return false;
        }
    }

    return true;
}

void run(vector<string> &tokens) {

    // check for max size of running processes vvvvvvv
    if (processes.size() >= MAX_PT_ENTRIES) {
        printf("Exceeds max process count. \n");
        return;
    }

    // ^^^^^^^


    // check for <  or > operators vvvvvvv
    vector<string> cmdList = vector<string>();
    string inputFile = "";
    string outputFile = "";
    if (tokens.size() > 0) {
        for (string item : tokens) {
            if(item[0] == '>') {
                outputFile = item.erase(0,1);
                cout << "output: " << outputFile << endl;
            } else if (item[0] == '<') {
                cmdList.push_back(inputFile.substr());
                cout << "input: " << inputFile << endl;
            } else if (!(item.substr() == "&")){
                cmdList.push_back(item.substr());
            }
        }
    }

    cout << "cmdList: "<<endl;
    for(auto i: cmdList) {
        cout << i << endl;
    }
    cout << endl;


    // ^^^^^^^

    // check if it is running in the background vvvvvvv

    bool runInBackground = false;

    if (tokens.size() > 1) {
        cout << "last char" << tokens.back() << endl;
        if (tokens.back() == "&") {
            runInBackground = true;
        } else {
            runInBackground = false;
        }
    }

    cout << "T/F: " << runInBackground << endl;

    // ^^^^^^^

    pid_t pid = fork();
    if (pid < 0) {
        printf("Error: Could not fork\n");
        return;
    }

    int errorFlag = 0;
    if (pid == 0) { // child process
        printf("child process here\n");
        cout << "size" << cmdList.size() << endl;
        switch (cmdList.size()) {
            case 1:
                errorFlag = execlp(cmdList.at(0).c_str(), cmdList.at(0).c_str(), 
                        (char *) nullptr);
                break;
            case 2:
                errorFlag = execlp(cmdList.at(0).c_str(), cmdList.at(0).c_str(), 
                        cmdList.at(1).c_str(), (char *) nullptr);
                break;
            case 3:
                errorFlag = execlp(cmdList.at(0).c_str(), cmdList.at(0).c_str(), 
                        cmdList.at(1).c_str(), cmdList.at(2).c_str(), 
                        (char *) nullptr);
                break;
            case 4:
                errorFlag = execlp(cmdList.at(0).c_str(), cmdList.at(0).c_str(), 
                        cmdList.at(1).c_str(), cmdList.at(2).c_str(), 
                        cmdList.at(3).c_str(), (char *) nullptr);
                break;
            case 5:
                errorFlag = execlp(cmdList.at(0).c_str(), cmdList.at(0).c_str(), 
                        cmdList.at(1).c_str(), cmdList.at(2).c_str(), 
                        cmdList.at(3).c_str(), cmdList.at(4).c_str(), 
                        (char *) nullptr);    
                break;
            case 6:
                errorFlag = execlp(cmdList.at(0).c_str(), cmdList.at(0).c_str(), 
                        cmdList.at(1).c_str(), cmdList.at(2).c_str(), 
                        cmdList.at(3).c_str(), cmdList.at(4).c_str(), 
                        cmdList.at(5).c_str(), 
                        (char *) nullptr);    
                break;
            case 7:
                errorFlag = execlp(cmdList.at(0).c_str(), cmdList.at(0).c_str(), 
                        cmdList.at(1).c_str(), cmdList.at(2).c_str(), 
                        cmdList.at(3).c_str(), cmdList.at(4).c_str(), 
                        cmdList.at(5).c_str(), cmdList.at(6).c_str(),  
                        (char *) nullptr);    
                break;
        }

        cout << errorFlag << endl;
        cout << "executed" << endl;
        if (errorFlag == -1) {
            cerr << "Could not execute " << cmdList.at(0) << endl;
            exit(1);
        }
               

        // Check if redirect to output file
        cout << outputFile << endl;
        if (outputFile != "") {
            freopen(outputFile.c_str(), "w", stdout);
        }

    } else { // parent process
        printf("parent process here\n");
        string cmdStr;
        int length = tokens.size();
        for (int i=0; i < length; i++) {
            cmdStr.append(" " + tokens.at(i));
        }
        cout << "commandStr: " << cmdStr << endl;
        task newTask;
        newTask.pid = pid;
        newTask.cmd = cmdStr;
        newTask.running = true;
        processes.emplace(pid, newTask);

        // if it is not running in the background wait for it then delete it after it's done running
        if (runInBackground == true) {
            // https://stackoverflow.com/questions/21248840/example-of-waitpid-in-use
            int status;
            waitpid(pid, &status, 0);
            processes.erase(pid);
        }

    }

    return;
}

void functions(string cmd) {
    if (cmd.length() > LINE_LENGTH) {
        printf("Maximum line is %d characters \n", LINE_LENGTH);
        return;
    }

    // tokenize the input
    // taken from https://stackoverflow.com/questions/236129/how-do-i-iterate-over-the-words-of-a-string#236803
    istringstream in_string(cmd);
    vector<string> tokens {
        istream_iterator<string>{in_string},
        istream_iterator<string>{}
    };


    if(checkTokenSize(tokens)){
        if (tokens.at(0) == "exit"){ 
            printf("\n");
        } else if (tokens.at(0) == "jobs"){ 
            printf("\n");
            listJobs();
        } else if (tokens.at(0) == "kill"){ 
            printf("\n");
        } else if (tokens.at(0) == "resume"){ 
            printf("\n");
        } else if (tokens.at(0) == "sleep"){ 
            sleep(tokens);
        } else if (tokens.at(0) == "suspend"){ 
            printf("\n");
        } else if (tokens.at(0) == "wait"){ 
            printf("\n");
        } else {
            printf("running: \n"); // not a valid command
            run(tokens);
        }

    }

    return;
}


// ------------------------------ 
int main(int argc, char *argv[]) {

    for (string cmd; cout << "Shell379: ";){
        getline(cin, cmd);
        if (cmd.empty()){
            cout << "empty command" << endl;
        } else {
            functions(cmd);
        }
    }

    return 0;
}

