//https://www.geeksforgeeks.org/socket-programming-cc/
//https://github.com/bozkurthan/Simple-TCP-Server-Client-CPP-Example
//Example code: A simple server side code, which echos back the received message.
//Handle multiple socket connections with select and fd_set on Linux
#include <stdio.h>
#include <string.h> //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h> //close
#include <arpa/inet.h> //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <string>
#include <iostream>
#include <chrono>
#include <ctime>

#include "tands.c"

using namespace std;

#define TRUE 1
#define FALSE 0

int main(int argc , char *argv[])
{
	//for the server, we only need to specify a port number
    if(argc != 2)
    {
        cerr << "Usage: port" << endl;
        exit(0);
    }

    //grab the port number
    int port = atoi(argv[1]);

	if (port < 5000 || port > 64000) {
		cerr << "Port must be between 5000 and 64000" << endl;
	}

	// total transactions 
	int transactionCount = 0;

    //buffer to send and receive messages with
    char buffer[1024];

	int opt = TRUE;
	int master_socket , addrlen , new_socket , client_socket[30] ,
		max_clients = 30 , activity, i , valread , sd;
	int max_sd;

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

	printf("Using port %d \n", port);

	//try to specify maximum of 3 pending connections for the master socket
	if (listen(master_socket, 3) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}
		
	//accept the incoming connection
	addrlen = sizeof(address);
		
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
	
		//wait for an activity on one of the sockets , timeout is NULL ,
		//so wait indefinitely
		activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
	
		if ((activity < 0) && (errno!=EINTR))
		{
			printf("select error");
		}
		//If something happened on the master socket ,
		//then its an incoming connection
		if (FD_ISSET(master_socket, &readfds))
		{
			if ((new_socket = accept(master_socket,
					(struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
			{
				perror("accept");
				exit(EXIT_FAILURE);
			}
			
			// //inform user of socket number - used in send and receive commands
			// printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , 
            //         new_socket , inet_ntoa(address.sin_addr) , ntohs
			// 	(address.sin_port));
					
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
		}
		
		//else its some IO operation on some other socket
		for (i = 0; i < max_clients; i++)
		{
			sd = client_socket[i];
				
			if (FD_ISSET( sd , &readfds))
			{
				if ((valread = read(sd, buffer, 1024)) == 0) {	
					//Somebody disconnected , get his details and print
					getpeername(sd , (struct sockaddr*)&address , \
						(socklen_t*)&addrlen);
					// printf("Host disconnected , ip %s , port %d \n" ,
					// 	inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
						
					//Close the socket and mark as 0 in list for reuse
					close( sd );
					client_socket[i] = 0;
				} else {
					transactionCount++;
					double recvTime = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
					string recvString = buffer;
					int delim = recvString.find(",");
					string hostNamePID = recvString.substr(0, delim);
					string transaction = recvString.substr(delim + 1);
					if (transaction[0] == 'T') {
						Trans(stoi(transaction.substr(1, transaction.size())));
					}
					char recString[128];
					sprintf(recString, "%.2f: # %i (T %i) from %s", recvTime/1000, transactionCount, stoi(recvString.substr(delim + 2)), hostNamePID.c_str()); 
					puts(recString);				

					memset(&buffer, 0, sizeof(buffer));	
					strcpy(buffer, to_string(transactionCount).c_str());
					double sendTime = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();		
					if( send(sd , buffer , strlen(buffer) , 0) < 0) {
						cerr << "Server send failure" << endl;
               			exit(EXIT_FAILURE);
					}

					char sendStr[128];
					sprintf(sendStr, "%.2f: # %i (T %i) from %s", sendTime/1000, transactionCount, stoi(recvString.substr(delim + 2)), hostNamePID.c_str()); 
					puts(sendStr);
				}
			}
		}
	}

	cout << endl;
	cout << "SUMMARY" << endl;
	cout << transactionCount << endl;
		
	return 0;
}
