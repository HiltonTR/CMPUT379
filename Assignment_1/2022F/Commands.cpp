#include "Commands.h"

// https://stackoverflow.com/questions/20167685/measuring-cpu-time-in-c
void printCPUTime() {
    struct rusage r;

    getrusage(RUSAGE_CHILDREN, &r);

    printf("User time: %ld s\nSys time: %ld s\n", r.ru_utime.tv_sec, r.ru_stime.tv_sec);

    return;
}

// https://unix.stackexchange.com/questions/7870/how-to-check-how-long-a-process-has-been-running
int getRunTime(pid_t pid) {
    int runTime;
    char buffer[1024];
    FILE *fp;

    sprintf(buffer, "ps -p %d -o times=", pid);
    fp = popen(buffer, "r");
    fscanf(fp, "%d", &runTime);
    pclose(fp);

    return runTime;
}

void exit() {
    for (auto &it : *getProcessesMap()) {
        pid_t pid = it.second.pid;
        kill(pid, SIGKILL);
    }
    getProcessesMap()->clear();
    printf("Resources used: \n");
    printCPUTime();
    fflush(stdout);
    exit(1);
}

void listJobs() {
    printf("Running processes: \n");

    if (!getProcessesMap()->empty()){
        printf(" #      PID S SEC COMMAND\n");
        int index = 0;

        for (auto &item : *getProcessesMap()) {
            task currentTask = item.second;
            string status = "";
            if(currentTask.running == 1){
                status = "R";
            } else {
                status = "S";
            }

            int runTime = getRunTime(currentTask.pid);
            printf(
                    "%2d: %7d %s %3d %s\n",
                    index,
                    currentTask.pid,
                    status.c_str(),
                    runTime, 
                    currentTask.cmd.c_str());

            index++;
        }
    }
    printf("Processes = %u active \n", (int) getProcessesMap()->size());
    printf("Completed processes: \n");
    printCPUTime();
    printf("\n");
    return;
}

void kill(vector<string> &tokens) {
    if(tokens.size() != 2) {
        printf("Invalid argment size for kill. \n");
    } else {
        pid_t pid = stoi(tokens.at(1));
        task *currentItem = getProcess(pid);

        if (currentItem != nullptr) {
            kill(pid, SIGKILL);
            getProcessesMap()->erase(pid);
        } else {
            printf("Can not find pid %d \n", pid);
        }

    }
}

void resume(vector<string> &tokens) {
    if(tokens.size() != 2) {
        printf("Invalid argment size for resume. \n");
    } else {
        pid_t pid = stoi(tokens.at(1));
        task *currentItem = getProcess(pid);

        if (currentItem != nullptr) {
            kill(pid, SIGCONT);
            currentItem->running = true;
        } else {
            printf("Can not find pid %d \n", pid);
        }

    }
}


void sleep(vector<string> &tokens) {
    if (tokens.size() < 2) {
        printf("Missing time argument. \n");
        return;
    }
    sleep(stoi(tokens.at(1)));
    return;
}

void suspend(vector<string> &tokens) {
    if(tokens.size() != 2) {
        printf("Invalid argment size for suspend. \n");
    } else {
        pid_t pid = stoi(tokens.at(1));
        task *currentItem = getProcess(pid);

        if (currentItem != nullptr) {
            kill(pid, SIGSTOP);
            currentItem->running = false;
        } else {
            printf("Can not find pid %d \n", pid);
        }

    }
}


void wait(vector<string> &tokens) {
    if(tokens.size() != 2) {
        printf("Invalid argment size for suspend. \n");
    } else {
        pid_t pid = stoi(tokens.at(1));
        task *currentItem = getProcess(pid);
        int status;
        if (currentItem != nullptr) {
            //https://linux.die.net/man/2/waitpid
            if (waitpid(pid, &status, 0) > 0) {
                getProcessesMap()->erase(pid);
            }
        } else {
            printf("Can not find pid %d \n", pid);
        }

    }
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
            exit();
        } else if (tokens.at(0) == "jobs"){ 
            printf("\n");
            listJobs();
        } else if (tokens.at(0) == "kill"){ 
            printf("\n");
            kill(tokens);
        } else if (tokens.at(0) == "resume"){ 
            printf("\n");
            resume(tokens);
        } else if (tokens.at(0) == "sleep"){ 
            sleep(tokens);
        } else if (tokens.at(0) == "suspend"){ 
            printf("\n");
            suspend(tokens);
        } else if (tokens.at(0) == "wait"){ 
            wait(tokens);
            printf("\n");
        } else {
            run(tokens);
        }

    }

    return;
}