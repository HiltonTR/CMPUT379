#include "general.h"

chrono::time_point<std::chrono::high_resolution_clock> get_time () {
    // return the current time
    chrono::time_point<std::chrono::high_resolution_clock> time = chrono::high_resolution_clock::now();
    return time;
}

double get_time_difference (chrono::time_point<std::chrono::high_resolution_clock> start_time) {
    // cast the time difference to double
    double time_difference;
    // referenced https://omegaup.com/docs/cpp/en/cpp/chrono/duration/duration_cast.html to get the time in ms
    time_difference = (double)chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - start_time).count()/(double)1000000;
    return time_difference;
}
