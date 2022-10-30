#include "outfile.h"

void output_to_file (string output_file) {
    freopen(output_file.c_str(), "w", stdout);
}

void output_stats (vector<int> no_of_completed_tasks, int work_count, int ask_count, int receive_count, int complete_count, int sleep_count, double tps) {
    cout << "Summary:" << endl;
    cout << "\t" << "Work" << "\t\t" << work_count << endl;
    cout << "\t" << "Ask" << "\t\t\t" << ask_count << endl;
    cout << "\t" << "Receive" << "\t\t" << receive_count << endl;
    cout << "\t" << "Complete" << "\t" << complete_count << endl;
    cout << "\t" << "Sleep" << "\t\t" << sleep_count << endl;

    int temp_index = 1;
    for (auto i: no_of_completed_tasks){
        cout << "\t" << "Thread " << temp_index << ":\t" << i << endl;
        temp_index++;
    }

    cout << "Transactions per second:" << "\t" << work_count/tps << endl;
}