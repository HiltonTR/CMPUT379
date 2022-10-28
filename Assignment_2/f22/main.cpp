#include <chrono>
#include <iostream>
#include <string>
#include <pthread.h>
#include <thread>
#include <queue>
#include <mutex>
#include "tands.c"
#include <vector>

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
auto start_time = chrono::high_resolution_clock::now();

// define id
int consumeThreads = 1;
int consumerID[1];
bool in_progress = false;

void* producer(void* args) {
    string command;
    in_progress = true;
    while (getline(cin, command)) {
        //cout << command[0] << " " << command[1] << endl;
        int n = (int)command[1] - 48;
        while ((int)buffer.size() >= maxbuffer) {
            pthread_cond_wait(&buffer_control.decrement, &buffer_control.mutex);
        }

        // check if it doesn't exceed max buffer, if it does wait
        if (command[0] == 'T'){
            // lock the buffer
            pthread_mutex_lock(&buffer_control.mutex);
            
            // log information here
            char task[128];

            sprintf (task, "%.3f ID= %2d Q=%2ld %-10s %i\n",
                (double)chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - start_time).count()/(double)1000000, 
                0, buffer.size(), "Work", n);
            output.append(task);
            cout << task;

            // push to queue
            buffer.push(command);

            // signal to thread that the buffer has an item
            pthread_cond_signal(&buffer_control.increment);

            // free buffer
            pthread_mutex_unlock(&buffer_control.mutex);


        } else if (command[0] == 'S') {
            // log information here
            char task[128];
            sprintf (task, "%.3f ID= %2d Q=%2ld %-10s %i\n",
                (double)chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - start_time).count()/(double)1000000, 
                0, buffer.size(), "Sleep", n);
            output.append(task);
            cout << task;

            // sleep
            Sleep(command[1]);
        }


    }
    in_progress = false;
    // print output


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
        sprintf (ask, "%.3f ID= %2d      %-10s\n",
            (double)chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - start_time).count()/(double)1000000, 
            id, "Ask");
        output.append(ask);
        cout << ask;

        // Lock buffer
        pthread_mutex_lock(&buffer_control.mutex);

        // Wait for work
        while (buffer.empty()) {
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
        sprintf (receive, "%.3f ID= %2d      %-10s %i\n",
            (double)chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - start_time).count()/(double)1000000, 
            id, "Receive", n);
        output.append(receive);
        cout << receive;

        // Perform the work
        Trans(command[1]);

        // Log complete
        char complete[128];
        sprintf (complete, "%.3f ID= %2d      %-10s %i\n",
            (double)chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - start_time).count()/(double)1000000, 
            id, "Complete", n);
        output.append(complete);
        cout << complete;

        // increment completed task counter 
        jobs_complete++;
    }
    return 0;
}

int main(int argc, char* argv[]) {
    start_time = chrono::high_resolution_clock::now();
    // defines the default amount of threads
    int nthreads;
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
    cout << output_file << endl;

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
    
    return 0;
}