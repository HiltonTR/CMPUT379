#include "Process.h"

map<pid_t, task> processes = map<pid_t, task>();

map<pid_t, task> *getProcessesMap() {
    return &processes;
}

task *getProcess(pid_t pid) {
    if (processes.find(pid) != processes.end()) {
        return &processes.at(pid);
    }

    return nullptr;
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
            } else if (item[0] == '<') {
                inputFile = item.erase(0,1);
            } else if (!(item.substr() == "&")){
                cmdList.push_back(item.substr());
            }
        }
    }
    // ^^^^^^^

    // check if it is running in the background vvvvvvv

    bool runInBackground = false;

    if (tokens.size() > 1) {
        if (tokens.back() == "&") { 
            runInBackground = true;
        } else {
            runInBackground = false;
        }
    }


    // ^^^^^^^

    pid_t pid = fork();
    if (pid < 0) {
        printf("Error: Could not fork\n");
        return;
    }


    int errorFlag = 0;
    if (pid == 0) { // child process
        // Check if redirect to output file
        // https://stackoverflow.com/questions/18086193/redirect-stdout-stderr-to-file-under-unix-c-again 
        if (outputFile != "") {

            int outFile = open(outputFile.c_str() , O_CREAT | O_WRONLY | O_APPEND, S_IRWXU);
            dup2(outFile, STDOUT_FILENO);
            close(outFile);
        }
        if (inputFile != "") {
            int inFile = open(inputFile.c_str() , O_RDONLY, S_IRWXU);
            dup2(inFile, STDIN_FILENO);
            close(inFile);
        }
        //cout << "size: " << cmdList.size() << endl;
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

        if (errorFlag == -1) {
            cout << "Could not execute " << cmdList.at(0) << endl;
            exit(1);
        }
        

    } else { // parent process
        string cmdStr;
        int length = tokens.size();
        for (int i=0; i < length; i++) {
            cmdStr.append(" " + tokens.at(i));
        }
        task newTask;
        newTask.pid = pid;
        newTask.cmd = cmdStr;
        newTask.running = true;
        processes.emplace(pid, newTask);

        // if it is not running in the background wait for it then delete it after it's done running
        if (runInBackground == false) {
            // https://stackoverflow.com/questions/21248840/example-of-waitpid-in-use
            int status;
            waitpid(pid, &status, 0);
            processes.erase(pid);
        }

    }

    return;
}