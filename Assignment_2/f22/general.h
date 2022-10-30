#ifndef GENERAL_H
#define GENERAL_H

#include <chrono>

using namespace std;

// https://en.cppreference.com/w/cpp/chrono/high_resolution_clock/now
chrono::time_point<std::chrono::high_resolution_clock> get_time ();

double get_time_difference (chrono::time_point<std::chrono::high_resolution_clock> start_time);


#endif 
