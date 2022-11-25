#include <iostream>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>


#include "tands.c"

using namespace std;
//Client side
int main(int argc, char *argv[])
{
    //we need 2 things: ip address and port number, in that order
    if(argc != 3) {
        cerr << "Usage: ip_address port" << endl; exit(0); 
    } 
    //grab the IP address and port number 
    char *serverIp = argv[2]; 
    int port = atoi(argv[1]); 
 
    // https://stackoverflow.com/questions/504810/how-do-i-find-the-current-machines-full-hostname-in-c-hostname-and-domain-info
    char hostname[128];
    if(gethostname(hostname, 128) < 0) {
        cerr << "Failed to get hostname" << endl;
        exit(EXIT_FAILURE);
    }
	pid_t pid = getpid();

    //create a message buffer 
    char msg[1024]; 

    //setup a socket and connection tools 
    struct hostent* host = gethostbyname(serverIp); 
    sockaddr_in sendSockAddr;   
    bzero((char*)&sendSockAddr, sizeof(sendSockAddr)); 
    sendSockAddr.sin_family = AF_INET; 
    sendSockAddr.sin_addr.s_addr = 
        inet_addr(inet_ntoa(*(struct in_addr*)*host->h_addr_list));
    sendSockAddr.sin_port = htons(port);
    int clientSd = socket(AF_INET, SOCK_STREAM, 0);

    //try to connect...
    if(connect(clientSd, (sockaddr*) &sendSockAddr, sizeof(sendSockAddr)) < 0){
        cout<<"Error connecting to socket!"<<endl;
        return -1;
    }

    string hostnameString = hostname;
    string hostnamePID = hostnameString+ '.' + to_string(pid);

    // redirect output to file
    freopen(hostnamePID.c_str(), "w", stdout);

    cout << "Using port " << port << endl;
    cout << "Using server address " << serverIp << endl;
    cout << "Host " << hostnamePID << endl;

    string data;
    int totalTransactions = 0;

    while (getline(cin, data)) {
        // int n = (int)command[1] - 48;
        if (data[0] == 'T'){
            totalTransactions++;
            // transaction
            memset(&msg, 0, sizeof(msg));//clear the buffer
            // strcpy(msg, data.c_str());
            sprintf(msg, "%s.%d,%s", hostname, (int)pid, data.c_str());
            if(send(clientSd, (char*)&msg, strlen(msg), 0) < 0) {
                cerr << "Client send failure" << endl;
                exit(EXIT_FAILURE);
            }
            double sendTime = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
            char sendStr[128];
            sprintf(sendStr, "%.2f: Send (T %i)", sendTime/1000, stoi(data.substr(1,data.size())));
            puts(sendStr);

            memset(&msg, 0, sizeof(msg));//clear the buffer
            if(recv(clientSd, (char*)&msg, sizeof(msg), 0) < 0) {
                cerr << "Client receive failure" << endl;
                exit(EXIT_FAILURE);
            }
            double recTime = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
            string doneString = msg;
            char recString[128];
            sprintf(recString, "%.2f: Recv (D %i)", recTime/1000, stoi(doneString));
            puts(recString);

        } else if (data[0] == 'S') {
            // sleep
            cout << "Sleep " << data.substr(1,data.size()) << " units" << endl;
            Sleep(stoi(data.substr(1,data.size())));
        }
    }

    close(clientSd);
    cout << "Sent " << totalTransactions << " transactions" << endl;
    return 0;    
}