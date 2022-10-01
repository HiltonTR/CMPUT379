#include "Commands.h"

// https://stackoverflow.com/questions/20167685/measuring-cpu-time-in-c
void printCPUTime() {
    struct rusage r;

    getrusage(RUSAGE_CHILDREN, &r);

    printf("CPU time: %ld s\nSys time: %ld s\n", r.ru_utime.tv_sec, r.ru_stime.tv_sec);

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

    if (!getProcessesMap()->empty()){
        printf(" #      PID S SEC COMMAND\n");
        int index = 0;

        for (auto &item : *getProcessesMap()) {
            task currentTask = item.second;
            // cout << "current : "<< currentTask.cmd << endl;

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
    printf("Processes = %u active \n", (int) getProcessesMap()->size());
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