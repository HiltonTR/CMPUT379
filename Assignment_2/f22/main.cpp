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
int thread_count = 1;
int consumerID[1];


void* producer(void* args) {
    string command;
    while (getline(cin, command)) {
        cout << command[0] << " " << command[1] << endl;

        while ((int)buffer.size() >= maxbuffer) {
            pthread_cond_wait(&buffer_control.decrement, &buffer_control.mutex);
        }

        // check if it doesn't exceed max buffer, if it does wait
        if (command[0] == 'T'){
            // lock the buffer
            pthread_mutex_lock(&buffer_control.mutex);
            
            // log information here
            char task[128];
            sprintf (task, "%.3f ID= %2d Q= %2ld %-10s %d\n",
                (double)chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - start_time).count()/(double)1000000, 
                0, buffer.size(), "Work", command[1]);
            output.append(task);

            // push to queue
            buffer.push(command);

            // signal to thread that the buffer has an item
            pthread_cond_signal(&buffer_control.increment);

            // free buffer
            pthread_mutex_unlock(&buffer_control.mutex);


        } else if (command[0] == 'S') {
            // lock the buffer
            pthread_mutex_lock(&buffer_control.mutex);

            // log information here
            char task[128];
            sprintf (task, "%.3f ID= %2d Q= %2ld %-10s %d\n",
                (double)chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - start_time).count()/(double)1000000, 
                0, buffer.size(), "Sleep", command[1]);
            output.append(task);

            // sleep
                Sleep(command[1]);

            // free buffer
            pthread_mutex_unlock(&buffer_control.mutex);

        }


    }
    // print output


    return 0;
}

// Consumer Section
void* consume(void* args) {
    // Log asked
    char ask[128];
    sprintf (ask, "%.3f ID= %2d      %-10s\n",
        (double)chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - start_time).count()/(double)1000000, 
        0, "Ask");
    output.append(ask);

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

    // Signal to producer that something was removed from the buffer
    pthread_cond_signal(&buffer_control.decrement);

    // Unlock buffer
    pthread_mutex_unlock(&buffer_control.mutex);

    // Log received
    char receive[128];
    sprintf (receive, "%.3f ID= %2d      %-10s %d\n",
        (double)chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - start_time).count()/(double)1000000, 
        0, "Receive", command[1]);
    output.append(receive);

    // Perform the work
    Trans(command[1]);

    // Log complete
    char complete[128];
    sprintf (complete, "%.3f ID= %2d      %-10s %d\n",
        (double)chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - start_time).count()/(double)1000000, 
        0, "Complete", command[1]);
    output.append(complete);

    // increment completed task counter 

    return 0;
}

int main(int argc, char* argv[]) {
    // defines the default amount of threads
    int nthreads = 0;
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

    // spawn producer here
    pthread_create(&threads[0], NULL, producer, 0);
    // spawn consumer here
    for (int i = 0; i < nthreads; ++i) {
        consumerID[i] = i + 1;
        pthread_create(&threads[i], NULL, consume, &consumerID[i]);
    }
    
    return 0;
}