#include "client.h"
/**
 * This is the client that sends transactions to the server to process. A lot of the code here is based off of and remain unchanged
 * https://www.geeksforgeeks.org/socket-programming-cc/
 * https://www.geeksforgeeks.org/socket-programming-in-cc-handling-multiple-clients-on-server-without-multi-threading/?ref=lbp
 * https://simpledevcode.wordpress.com/2016/06/16/client-server-chat-in-c-using-sockets/
 */
int client(char *argv[]) {
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
    // print out the headers needed for the client output file
    cout << "Using port " << port << endl;
    cout << "Using server address " << serverIp << endl;
    cout << "Host " << hostnamePID << endl;

    // keep track of the total transactions
    string data;
    int totalTransactions = 0;

    while (getline(cin, data)) {
        if (data[0] == 'T'){
            totalTransactions++;
            // transaction
            memset(&msg, 0, sizeof(msg));//clear the buffer
            sprintf(msg, "%s.%d,%s", hostname, (int)pid, data.c_str());
            // attempt to send data
            if(send(clientSd, (char*)&msg, strlen(msg), 0) < 0) {
                cerr << "Client send failure" << endl;
                exit(EXIT_FAILURE);
            }
            // getting time found from https://www.techiedelight.com/get-current-timestamp-in-milliseconds-since-epoch-in-cpp/  
            double sendTime = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
            char sendStr[128];
            sprintf(sendStr, "%.2f: Send (T %i)", sendTime/1000, stoi(data.substr(1,data.size())));
            puts(sendStr);

            memset(&msg, 0, sizeof(msg));//clear the buffer
            // attempt to receive data
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
    // close the client after the file is done
    close(clientSd);
    cout << "Sent " << totalTransactions << " transactions" << endl;
    return 0; 

}
/**
 * main function to call the client
 */
int main(int argc, char *argv[]) {
    //we need 2 things: ip address and port number, in that order
    if(argc != 3) {
        cerr << "Usage: ip_address port" << endl; exit(0); 
    } 
    // calls the client to run
    client(argv);

    return 0;    
}