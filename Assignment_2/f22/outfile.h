#ifndef OUTFILE_H
#define OUTFILE_H

#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <bits/stdc++.h>

using namespace std;

void output_to_file(string output_file);

void output_stats(vector<int> no_of_completed_tasks, int work_count, int ask_count, int receive_count, int complete_count, int sleep_count, double tps);

#endif 
