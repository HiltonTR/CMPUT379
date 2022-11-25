#include "server.h"
// For the general structure of the code, the following links were used. The second url was used for the select implementation.
//https://www.geeksforgeeks.org/socket-programming-cc/
//https://www.geeksforgeeks.org/socket-programming-in-cc-handling-multiple-clients-on-server-without-multi-threading/?ref=lbp
//https://simpledevcode.wordpress.com/2016/06/16/client-server-chat-in-c-using-sockets/

/**
 * This is the server which takes the transactions from the client and processes them
 */
int server(char *argv[]) {
	//grab the port number and limit it
    int port = atoi(argv[1]);

	if (port < 5000 || port > 64000) {
		cerr << "Port must be between 5000 and 64000" << endl;
	}

	// total transactions 
	int transactionCount = 0;

    // https://stackoverflow.com/questions/504810/how-do-i-find-the-current-machines-full-hostname-in-c-hostname-and-domain-info
    char hostname[128];
    if(gethostname(hostname, 128) < 0) {
        cerr << "Failed to get hostname" << endl;
        exit(EXIT_FAILURE);
    }
	// get the pid
	pid_t pid = getpid();
	
	// create the hostname and pid string so we can redirect the stdout later
    string hostnameString = hostname;
    string serverOutput = hostnameString+ '.' + to_string(pid);

    //buffer to send and receive messages with
    char buffer[1024];
	
	// define some variables we may need later. This was taken from the links above
	int opt = TRUE;
	int master_socket , addrlen , new_socket , client_socket[30] ,
		max_clients = 30 , activity, i , valread , sd;
	int max_sd;

	// map to store the transactions that each client has done
	map<string, int> client_completed_tasks;

	// these are to check for the first and last transaction
	bool first = true;
	double firstTime, lastTime;

	struct sockaddr_in address;		
		
	//set of socket descriptors
	fd_set readfds;
		
	//type of socket created
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl ( INADDR_ANY );
	address.sin_port = htons( port );

	//initialise all client_socket[] to 0 so not checked
	for (i = 0; i < max_clients; i++){
		client_socket[i] = 0;
	}
		
	//create a master socket
	if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0){
		perror("socket failed");
		exit(EXIT_FAILURE);
	}
	
	//set master socket to allow multiple connections ,
	//this is just a good habit, it will work without this
	if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ){
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	//set the socket to be non blocking
	if(ioctl(master_socket, FIONBIO, (char *)&opt) < 0) {
		perror("ioctl");
		exit(EXIT_FAILURE);
	}

	//bind the socket to its local address
    if(bind(master_socket, (struct sockaddr*) &address, sizeof(address)) < 0){
        cerr << "Error binding socket to local address" << endl;
        exit(0);
    }

	//try to specify maximum of 3 pending connections for the master socket
	if (listen(master_socket, 3) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}
		
	//accept the incoming connection
	addrlen = sizeof(address);
	
	// declare timeout 
	struct timeval timeout;

	// redirect output to file
	freopen(serverOutput.c_str(), "w", stdout);

	printf("Using port %d \n", port);

	while(TRUE)
	{
		//clear the socket set
		FD_ZERO(&readfds);
	
		//add master socket to set
		FD_SET(master_socket, &readfds);
		max_sd = master_socket;
			
		//add child sockets to set
		for ( i = 0 ; i < max_clients ; i++)
		{
			//socket descriptor
			sd = client_socket[i];
				
			//if valid socket descriptor then add to read list
			if(sd > 0)
				FD_SET( sd , &readfds);
				
			//highest file descriptor number, need it for the select function
			if(sd > max_sd)
				max_sd = sd;
		}
		// Timeout time of 30s
		timeout.tv_sec = 30;
		timeout.tv_usec = 0;

		//wait for an activity on one of the sockets , timeout is 30
		activity = select( max_sd + 1 , &readfds , NULL , NULL , &timeout);
	
		if ((activity < 0) && (errno!=EINTR))
		{
			printf("select error");
		}
		//If something happened on the master socket ,
		//then its an incoming connection
		if (FD_ISSET(master_socket, &readfds))
		{
			if ((new_socket = accept(master_socket,
					(struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
				perror("accept");
				exit(EXIT_FAILURE);
			}
								
			//add new socket to array of sockets
			for (i = 0; i < max_clients; i++)
			{
				//if position is empty
				if( client_socket[i] == 0 )
				{
					client_socket[i] = new_socket;
					// printf("Adding to list of sockets as %d\n" , i);
					break;
				}
			}
		} else if (activity == 0) {
			// server timeout so close socket
			close(master_socket);
			break;
		}

		//else its some IO operation on some other socket
		for (i = 0; i < max_clients; i++)
		{
			sd = client_socket[i];
				
			if (FD_ISSET( sd , &readfds))
			{
				if ((valread = read(sd, buffer, 1024)) == 0) {	
					//Somebody disconnected , get details
					getpeername(sd , (struct sockaddr*)&address , \
						(socklen_t*)&addrlen);
						
					//Close the socket and mark as 0 in list for reuse
					close( sd );
					client_socket[i] = 0;
				} else {
					// here we increment the transaction count
					transactionCount++;
					// to get the time: https://www.techiedelight.com/get-current-timestamp-in-milliseconds-since-epoch-in-cpp/ 
					double recvTime = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
					// check if this is the first transaction on the server
					if (first) {
						firstTime = recvTime;
						first = false;
					}
					string recvString = buffer;
					// process the data received 
					int delim = recvString.find(",");
					string hostNamePID = recvString.substr(0, delim);
					string transaction = recvString.substr(delim + 1);
					client_completed_tasks[hostNamePID]++;
					// if it's a transaction
					if (transaction[0] == 'T') {
						// call trans for # and print out the T and then send the data back and print out the Done
						Trans(stoi(transaction.substr(1, transaction.size())));
						char recString[128];
						sprintf(recString, "%.2f: # %i (T %i)\tfrom %s", recvTime/1000, transactionCount, stoi(recvString.substr(delim + 2)), hostNamePID.c_str()); 
						puts(recString);				

						memset(&buffer, 0, sizeof(buffer));	
						strcpy(buffer, to_string(transactionCount).c_str());
						double sendTime = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
						// attempt to send data		
						if( send(sd , buffer , strlen(buffer) , 0) < 0) {
							cerr << "Server send failure" << endl;
							exit(EXIT_FAILURE);
						}
						// update the last item sent time to be the most recent send time
						lastTime = sendTime;
						char sendStr[128];
						sprintf(sendStr, "%.2f: # %i (Done)\tfrom %s", sendTime/1000, transactionCount, hostNamePID.c_str()); 
						puts(sendStr);
					}
				}
			}
		}
	}

	cout << endl;
	cout << "SUMMARY" << endl;
	// https://stackoverflow.com/questions/26281979/c-loop-through-map 
	for (auto const& x : client_completed_tasks) {
		cout << "\t" << x.second << " transactions from " << x.first << endl;  
	}
	cout <<transactionCount/((lastTime-firstTime)/1000)\
		<< " transactions/sec\t(" << transactionCount << "/" << (lastTime-firstTime)/1000 << ")" << endl;

	return 0;
}

/**
 * This is the main function that takes the input and calls server.
 */
int main(int argc , char *argv[]) {
	//for the server, we only need to specify a port number
    if(argc != 2)
    {
        cerr << "Usage: port" << endl;
        exit(0);
    }

	server(argv);

	return 0;
}
