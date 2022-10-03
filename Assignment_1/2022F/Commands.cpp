#include "Commands.h"

// this function gets the amount of cpu time used. Referenced from:
// https://stackoverflow.com/questions/20167685/measuring-cpu-time-in-c
void printCPUTime() {
    // create a struct for the times 
    struct rusage r;
    // we want the time of the children run: https://man7.org/linux/man-pages/man2/getrusage.2.html 
    getrusage(RUSAGE_CHILDREN, &r);
    printf("User time: %ld s\nSys time: %ld s\n", r.ru_utime.tv_sec, r.ru_stime.tv_sec);
    return;
}

// https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po 
int getRunTime(pid_t pid) {
    // we create a variable here for the runtime of the program and a buffer so we can manipulate the file.
    char buffer[128];
    int runTime;
    // https://www.unix.com/programming/169871-print-ancestor-list-c-name-pid.html 
    // man pages for the ps https://man7.org/linux/man-pages/man1/ps.1.html 
    sprintf(buffer, "ps -p %d -o times=", pid);
    // so here we want to read from the buffer and grab the values
    FILE *temp = popen(buffer, "r");
    // we read from the buffer then close the file
    fscanf(temp, "%d", &runTime);
    pclose(temp);
    // return the time that it runs for
    return runTime;
}

// here we want to loop and check for all the pids in the table and kill them
void exit() {
    for (auto &currentTask : *getProcessesMap()) {
        // call the getter and loop through every item and call sigkill
        pid_t pid = currentTask.second.pid;
        kill(pid, SIGKILL);
    }
    // empty the table and print the resouces used and flush the buffer then exit
    getProcessesMap()->clear();
    printf("Resources used: \n");
    printCPUTime();
    fflush(stdout);
    exit(1);
}

// here want to see all the jobs that are currently running
void listJobs() {
    printf("Running processes: \n");
    // if the map is not empty, we print out  the header
    if (!getProcessesMap()->empty()){
        printf(" #      PID S SEC COMMAND\n");
        // create an index so we know which item we'll be on
        int index = 0;
        // iterate through the map and check the stats of the item and determine it's status
        for (auto &item : *getProcessesMap()) {
            task currentTask = item.second;
            string status = "";
            if(currentTask.running == 1){
                status = "R";
            } else {
                status = "S";
            }
            // we get the amount of time that the task has been running
            int runTime = getRunTime(currentTask.pid);
            // print out the details as required by the assignment
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
    // so here we want to see how many tasks are active so we grab the size of the map
    printf("Processes = %u active \n", (int) getProcessesMap()->size());
    // here we print out the cpu usage times for formatting
    printf("Completed processes: \n");
    printCPUTime();
    printf("\n");
    return;
}

// here is the kill command to kill a process
void kill(vector<string> &tokens) {
    // check for the size and make sure it's correct
    if(tokens.size() != 2) {
        printf("Invalid argment size for kill. \n");
    } else {
        // get the pid and the item from the table
        pid_t pid = stoi(tokens.at(1));
        task *currentItem = getProcess(pid);
        // if the item exists, we send a sigkill signal to it and remove the item
        // from the map, otherwise print an error message
        // termination signals found from: https://www.gnu.org/software/libc/manual/html_node/Termination-Signals.html 
        if (currentItem != nullptr) {
            kill(pid, SIGKILL);
            getProcessesMap()->erase(pid);
        } else {
            printf("Can not find pid %d \n", pid);
        }

    }
}

// here we want to resume a process
void resume(vector<string> &tokens) {
    // check for the size and make sure it's correct
    if(tokens.size() != 2) {
        printf("Invalid argment size for resume. \n");
    } else {
        // get the pid and the item from the table
        pid_t pid = stoi(tokens.at(1));
        task *currentItem = getProcess(pid);
        // if the item exists, we send a sigcont signal to it and change the status of the item so that it's running again
        // from the map, otherwise print an error message
        // termination signals found from: https://www.gnu.org/software/libc/manual/html_node/Termination-Signals.html 
        if (currentItem != nullptr) {
            kill(pid, SIGCONT);
            currentItem->running = true;
        } else {
            printf("Can not find pid %d \n", pid);
        }

    }
}

// here we create a sleep function 
void sleep(vector<string> &tokens) {
    // makes sure the amount of arguments are enough
    if (tokens.size() < 2) {
        printf("Missing time argument. \n");
        return;
    }
    // calls the thread to sleep for inputted seconds https://man7.org/linux/man-pages/man3/sleep.3.html 
    sleep(stoi(tokens.at(1)));
    return;
}

// function to temporarily stop a process
void suspend(vector<string> &tokens) {
    // check if there's enough arguments
    if(tokens.size() != 2) {
        printf("Invalid argment size for suspend. \n");
    } else {
        // get the pid and the item from the table
        pid_t pid = stoi(tokens.at(1));
        task *currentItem = getProcess(pid);
        // if the item exists, we send a sigstop signal to it and change the status of the item so that it stops
        // from the map, otherwise print an error message
        // termination signals found from: https://www.gnu.org/software/libc/manual/html_node/Termination-Signals.html 
        if (currentItem != nullptr) {
            kill(pid, SIGSTOP);
            currentItem->running = false;
        } else {
            printf("Can not find pid %d \n", pid);
        }

    }
}

// function to wait for a process to finish.
void wait(vector<string> &tokens) {
    // check if there's enough arguments
    if(tokens.size() != 2) {
        printf("Invalid argment size for suspend. \n");
    } else {
        // get the pid and the item from the table
        pid_t pid = stoi(tokens.at(1));
        task *currentItem = getProcess(pid);
        // if the item is a valid item, we will wait for the given process of the given pid to be completed
        // then it will be removed from the map
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

// this function checks for the basic correctness of the token inputs
bool checkTokenSize(vector<string> &tokens){
    // checks if it doesnt exceed the max allowed arguments
    if (tokens.size() > MAX_ARGS) {
        printf("Too many arguments. \n");
        return false;
    }
    // checks if the arguments don't exceed the max length
    for (int i = 0; i < tokens.size(); i++) {
        string token = tokens.at(i);
        if (token.length() > MAX_LENGTH) {
            printf("Argument is too long. \n");
            return false;
        }
    }

    return true;
}

// this is the function where we process the input
void functions(string cmd) {
    // check if the line is under the max char length
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

    // if the token size is valid, we go through all the possible commands and do their function calls.
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