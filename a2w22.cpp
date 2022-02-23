#include <string>
#include "a2w22.h"
//#include "controller.h"
//#include "switch.h"
#include <sstream>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <vector>

using namespace std;

int main(int argc, char *argv[]) {
      if (argc < 3 || argc > 6) {
        printf("ERROR: invalid argument format:\n"
               "\t'a2w22 master nSwitch'\n"
               "\t'a2w22 pswi dataFile (null|pswj) (null|pswk) IPlow-IPhigh'\n");
        exit(EINVAL);
    }

    if (argv[1] == string("master") && argc == 3 && stoi(argv[2]) <= MAX_SWITCH) {
        //controller(stoi(argv[2]));
    } else if (string(argv[1]).find("pswi") != string::npos && argc == 6) {
        IPRange ip_range = ip_range_split(argv[5]);

        int switch_num = stoi(string(argv[1]).substr(2));
        int left_switch_num = -1;
        int right_switch_num = -1;

        if (argv[3] != string("null")) left_switch_num = stoi(string(argv[3]).substr(2));
        if (argv[4] != string("null")) right_switch_num = stoi(string(argv[4]).substr(2));

        //switch(switch_num, left_switch_num, right_switch_num, argv[2], ip_range);
    } else {
        printf("Invalid argument\n");
        exit(EXIT_FAILURE);
    }
    return 0;

}

IPRange ip_range_split(string input) {
  stringstream ss(input);
  string item;
  vector<int> temp;
  IPRange ip_range;

  while (getline(ss, item, '-')) {
    temp.push_back(stoi(item));
  }

  ip_range.low = temp[0];
  ip_range.high = temp[1];

  return ip_range;
}