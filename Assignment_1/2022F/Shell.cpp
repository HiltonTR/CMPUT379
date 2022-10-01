#include "Shell.h"


int main(int argc, char *argv[]) {

    for (string cmd; cout << "Shell379: ";){
        getline(cin, cmd);
        if (cmd.empty()){
            cout << "empty command" << endl;
        } else {
            functions(cmd);
        }
    }

    return 0;
}