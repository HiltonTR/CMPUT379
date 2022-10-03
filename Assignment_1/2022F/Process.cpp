#include "Process.h"
// defines the map that will be used to store all the processes
// we use a struct to define task as we can have all the variables we need
// we can then put it as the value in the map and have the key the pid as that's what we search by
map<pid_t, task> processes;

// returns the pointer to the map, getter method to access it from commands.cpp
// inspiration from https://stackoverflow.com/questions/12851860/accessing-a-map-by-returning-a-pointer-to-it-c 
map<pid_t, task> *getProcessesMap() {
    return &processes;
}

// returns the pointer to the specific process otherwise it will return a null pointer
// same idea as above where we get the map from a pointer but intead here we will get the 
// struct with the information we need. We return a null pointer if it can't find anything.
// https://www.eskimo.com/~scs/cclass/notes/sx10d.html
task *getProcess(pid_t pid) {
    if (processes.find(pid) != processes.end()) {
        return &processes.at(pid);
    }
    return nullptr;
}

// this is the function that generates the process
void run(vector<string> &tokens) {
    // check for max size of running processes
    if (processes.size() >= MAX_PT_ENTRIES) {
        printf("More processes than allowed. \n");
        return;
    }
    // check for <  or > operators 
    vector<string> cmdList = vector<string>(); //creates a vector of strings where < > & aren't included
    string inputFile = "";
    string outputFile = "";
    if (tokens.size() > 0) {
        for (string item : tokens) {
            if(item[0] == '>') {
                outputFile = item.erase(0,1); // determines the output file
            } else if (item[0] == '<') {
                inputFile = item.erase(0,1); // determines the input file
            } else if (!(item.substr() == "&")){
                cmdList.push_back(item.substr());//appends to vector of strings where < > & aren't included
            }
        }
    }
    // check if it is running in the background
    bool runInBackground = false;
    // checking the last value if there's an ampersand
    if (tokens.size() > 1) {
        if (tokens.back() == "&") { 
            runInBackground = true;
        } else {
            runInBackground = false;
        }
    }

    // forks the process
    pid_t pid = fork();
    // if the fork didn't work, exit
    if (pid < 0) {
        printf("Error: Could not fork\n");
        exit(1);
        return;
    }

    // defines the error flag in case exec fails
    int errorFlag = 0;
    if (pid == 0) { // child process
        // redirects stdout 
        // inspired from https://stackoverflow.com/questions/18086193/redirect-stdout-stderr-to-file-under-unix-c-again 
        if (outputFile != "") {
            int outFile = open(outputFile.c_str() , O_CREAT | O_WRONLY | O_APPEND, S_IRWXU);
            dup2(outFile, STDOUT_FILENO);
            close(outFile);
        }
        // redirects stdin 
        if (inputFile != "") {
            int inFile = open(inputFile.c_str() , O_RDONLY, S_IRWXU);
            dup2(inFile, STDIN_FILENO);
            close(inFile);
        }
        // switch statement to call exec. It's ugly but it works
        // information taken from https://linux.die.net/man/3/execlp 
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

        // if any of the execs failed we will have an error stating that the command could not be exec'd
        if (errorFlag == -1) {
            cout << "Could not execute " << cmdList.at(0) << endl;
            exit(1);
        }
        

    } else { // parent process
        // here we copy over the input command string so we can paste it into the map
        string cmdStr;
        int length = tokens.size();
        for (int i=0; i < length; i++) {
            cmdStr.append(" " + tokens.at(i));
        }
        // sets the values of the struct 
        task newTask;
        newTask.pid = pid;
        newTask.cmd = cmdStr;
        newTask.running = true;
        processes.emplace(pid, newTask); // places the struct with the information into a map where it's mapped with the pid

        // if it is not running in the background wait for it then delete it after it's done running
        if (runInBackground == false) {
            // https://stackoverflow.com/questions/21248840/example-of-waitpid-in-use <- inspiriation from 
            int status;
            waitpid(pid, &status, 0);
            processes.erase(pid);
        }

    }

    return;
}