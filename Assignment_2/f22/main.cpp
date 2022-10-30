#include <chrono>
#include <iostream>
#include <string>
#include <pthread.h>
#include <thread>
#include <queue>
#include <mutex>
#include <vector>
#include <stdio.h>
#include <cstdio>
#include <fstream>

#include "tands.c"
#include "general.h"
#include "outfile.h"

using namespace std;

// global variables:

// defines max buffer size
int maxbuffer;

// creates a global queue to add tasks
queue<string> buffer;

// defines the thread mutex
typedef struct {
    pthread_cond_t increment;
    pthread_cond_t decrement;
    pthread_mutex_t mutex;
} pthread_buffer;

pthread_buffer buffer_control;

// creates a string to store the output
string output;

// defines start time
chrono::time_point<std::chrono::high_resolution_clock> start_time;

// defines count for summary
int work_count = 0;
int ask_count = 0;
int receive_count = 0;
int complete_count = 0;
int sleep_count = 0;
map<int, int> thread_completed_tasks;

// define id
int nthreads;
int consumeThreads = 1;
int consumerID[1];
bool in_progress = false;

bool done = false;

void* producer(void* args) {
    string command;
    in_progress = true;
    while (getline(cin, command)) {
        //cout << command[0] << " " << command[1] << endl;
        int n = (int)command[1] - 48;

        // check if it doesn't exceed max buffer, if it does wait
        if (command[0] == 'T'){
            // log information here
            char task[128];
            sprintf (task, "\t%.3f ID= %2d Q=%2ld %-10s %i\n",
                get_time_difference(start_time), 
                0, buffer.size(), "Work", n);
            output.append(task);
            cout << task;

            // increase work count
            work_count++;

            // lock the buffer
            pthread_mutex_lock(&buffer_control.mutex);

            while ((int)buffer.size() >= maxbuffer) {
                pthread_cond_wait(&buffer_control.decrement, &buffer_control.mutex);
            }            

            // push to queue
            buffer.push(command);

            // signal to thread that the buffer has an item
            pthread_cond_signal(&buffer_control.increment);

            // free buffer
            pthread_mutex_unlock(&buffer_control.mutex);


        } else if (command[0] == 'S') {
            // log information here
            char task[128];
            sprintf (task, "\t%.3f ID= %2d Q=%2ld %-10s %i\n",
                get_time_difference(start_time), 
                0, buffer.size(), "Sleep", n);
            output.append(task);
            cout << task;

            // sleep
            Sleep(command[1]);

            // increase sleep count
            sleep_count++;
        }


    }
    done = true;
    pthread_mutex_lock(&buffer_control.mutex);
    pthread_cond_broadcast(&buffer_control.increment);
    pthread_mutex_unlock(&buffer_control.mutex);
    in_progress = false;
    return 0;
}

//Consumer Section
void* consume(void* args) {
    int id = consumeThreads;
    int jobs_complete = 0;
    consumeThreads++;
    while(in_progress) {
        // Log asked
        char ask[128];
        sprintf (ask, "\t%.3f ID= %2d      %-10s\n",
            get_time_difference(start_time), 
            id, "Ask");
        output.append(ask);
        cout << ask;

        // increase ask count
        ask_count++;

        // Lock buffer
        pthread_mutex_lock(&buffer_control.mutex);

        // Wait for work
        while (buffer.empty()) {
            if (done) {
                pthread_mutex_unlock(&buffer_control.mutex);
                return 0;
            }
            // Wait until signal that buffer is not empty
            pthread_cond_wait(&buffer_control.increment, &buffer_control.mutex);
        }

        // Pop from queue
        string command = buffer.front();
        buffer.pop();
        int n = (int)command[1] - 48;

        // Signal to producer that something was removed from the buffer
        pthread_cond_signal(&buffer_control.decrement);

        // Unlock buffer
        pthread_mutex_unlock(&buffer_control.mutex);

        // Log received
        char receive[128];
        sprintf (receive, "\t%.3f ID= %2d      %-10s %i\n",
            get_time_difference(start_time), 
            id, "Receive", n);
        output.append(receive);
        cout << receive;

        // increase receive count
        receive_count++;

        // Perform the work
        Trans(command[1]);

        // Log complete
        char complete[128];
        sprintf (complete, "\t%.3f ID= %2d      %-10s %i\n",
            get_time_difference(start_time), 
            id, "Complete", n);
        output.append(complete);
        cout << complete;

        // increase complete count
        complete_count++;

        // increment completed task counter 
        jobs_complete++;
        thread_completed_tasks[id] = jobs_complete;
    }
    return 0;
}

int main(int argc, char* argv[]) {
    start_time = get_time();
    // defines the default amount of threads
    int thread_id = 0;
    char output_file[64];

    // not enough arguments will end program
    if (argc < 2) {
        fprintf(stderr, "Not enough arguments");
        exit(1);
    }

    // get the amount of threads and the output file id
    nthreads = atoi(argv[1]);

    if (argc > 2) {
        thread_id = atoi(argv[2]);
    }
    
    maxbuffer = nthreads * 2;

    if (thread_id == 0 ) {
        sprintf(output_file, "prodcon.log");
    } else {
        sprintf(output_file, "prodcon.%d.log", thread_id);
    }
    cout << output_file <<endl;
    output_to_file(output_file);
    pthread_t threads[nthreads];


    //spawn producer here
    pthread_create(&threads[0], NULL, producer, 0);
    //spawn consumer here
    for (int i = 0; i < nthreads; ++i) {
        pthread_create(&threads[i], NULL, consume, 0);
    }

    for (int i = 0; i < nthreads; i++) {
        pthread_join(threads[i], NULL);
    }

    output_stats(nthreads, thread_completed_tasks, work_count, ask_count, receive_count, complete_count, sleep_count, get_time_difference(start_time));

    return 0;
}