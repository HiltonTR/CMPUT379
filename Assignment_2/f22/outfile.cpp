#include "outfile.h"

void output_to_file (string output_file) {
    freopen(output_file.c_str(), "w", stdout);
}

void output_stats (int nthreads, map<int, int> no_of_completed_tasks, int work_count, int ask_count, int receive_count, int complete_count, int sleep_count, double tps) {
    cout << "Summary:" << endl;
    cout << "\t" << "Work" << "\t\t" << work_count << endl;
    cout << "\t" << "Ask" << "\t\t" << ask_count << endl;
    cout << "\t" << "Receive" << "\t\t" << receive_count << endl;
    cout << "\t" << "Complete" << "\t" << complete_count << endl;
    cout << "\t" << "Sleep" << "\t\t" << sleep_count << endl;

    for (int i = 1; i <= nthreads; i++){
        cout << "\t" << "Thread " << i << ":\t" << no_of_completed_tasks[i] << endl;
    }

    cout << "Transactions per second:" << "\t" << work_count/tps << endl;
}