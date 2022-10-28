#include <iostream>
#include <unistd.h>

using namespace std;

int main(int argc, char *argv[]) {
    int x1, x2, x3, x4, x5;
    cout << "Hello! This is Test.cpp!" << endl;
    cout << "Input your values!" << endl;
    cin >> x1 >> x2 >> x3 >> x4 >> x5;
    int total = x1 + x2 + x3 + x4 + x5;
    cout << "Your inputs: " << x1 << " " << x2 << " " << x3 << " " << x4 << " " << x5 << endl;
    cout << "Now we'll use your command line arguments!" << endl;
    cout << "Your command line arguments: " << endl;
    for (int i = 1; i < argc; i++) {
        cout << argv[i];
        total += stoi(argv[i]);
        for (int j = 0; j < 3; j++) {
            cout << "." << flush;
            sleep(1);
        }
    }
    cout << "BING CHILLING! NOW YOU WILL WAIT FOR YOUR NUMBER!" << endl;
    long x = -2147483640;
    while (x < 21474836400) {
        x++;
    }
    cout << endl;
    cout << "Your total sum is: " << total << ". Thanks for using Test.cpp! " << endl;
}