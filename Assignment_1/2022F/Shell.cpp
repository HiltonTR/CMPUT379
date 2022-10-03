#include "Shell.h"

// here is the main function that will take in the input and pass it to the function
// that will read the input and check it.
int main(int argc, char *argv[]) {

    for (;;){
        string cmd; 
        cout << "Shell379: ";
        // get the input line
        getline(cin, cmd);
        if (cmd.empty()){
            cout << "Empty command" << endl;
        } else {
            // pass it to the functions which will determine the command
            functions(cmd);
        }
    }

    return 0;
}