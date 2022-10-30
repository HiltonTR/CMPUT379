#include "general.h"

chrono::time_point<std::chrono::high_resolution_clock> get_time () {
    chrono::time_point<std::chrono::high_resolution_clock> time = chrono::high_resolution_clock::now();
    return time;
}

double get_time_difference (chrono::time_point<std::chrono::high_resolution_clock> start_time) {
    double time_difference;
    time_difference = (double)chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - start_time).count()/(double)1000000;
    return time_difference;
}
