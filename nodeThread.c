/* 
TCP Server, port served as an argument 
*/

//All the directive header needed
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

//Preparation
void initialize();

//Print error and exit
void printError(const char *message);

//Returning appropriate message based on input
char* messageHandler(const char* message);

//Accept new client/master
void* acceptMaster(void* connection);

int main(int argc, char *argv[])
{
	//Preparation
	initialize();

	//Thread for establishing connection and do monitoring routines
	//listenfd -> nodeSocketFD 
	//Thread gua/ukay
	pthread_t connectionThread[10];
	//Thread jason
	pthread_t threads;

	//Node & master socket, munin port
	int portNumber = 4949, nodeSocketFD, masterSocketFD, connectionCount = 0;

	//Struct server & client (node, master)
	struct sockaddr_in serverAddress, clientAddress;

	//Length of socket client
	socklen_t clientLength;

	//Create socket
	nodeSocketFD = socket(AF_INET, SOCK_STREAM, 0);

	//Error if socket creation fail
	if (nodeSocketFD < 0) {
		printError("Failed to create/open socket");
	}

	//Make serverAddress, clear the buff
	bzero((char*) &serverAddress, sizeof(serverAddress));

	//Set server family
	serverAddress.sin_family = AF_INET;
	//Set server address
	//GATAU PAKE YG MANA
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	//serverAddress.sin_addr.s_addr = INADDR_ANY;
	//Set server port
	serverAddress.sin_port = htons(portNumber);

	//Error if port already used
	if (bind(nodeSocketFD, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) < 0) {
		printError("Failed to bind socket to server");	
	}
	
	//Let server socket to listen up to 5 client
	listen(nodeSocketFD,5);

	//Don't know
	clientLength = sizeof(clientAddress);

	//Loop to always accept new connection
	while (masterSocketFD = accept(nodeSocketFD, (struct sockaddr *) &clientAddress, &clientLength)) {
		//Thread Gua
		if (pthread_create(&connectionThread[connectionCount], NULL, acceptMaster, (void*) &masterSocketFD) < 0 ) {
			printError("Failed to create a thread for new connection");
			break;
		} else {
			connectionCount++;
		}

		//Thread Ukay
		/*
		int* connectionFD;
		*connectionFD = accept(nodeSocketFD, (struct sockaddr*)NULL, NULL);
		pthread_create(&connectionThread[connectionCount], NULL, acceptMaster, (void*) connectionFD);
		connectionCount++;

		//Thread Jason
		if(pthread_create(&threads , NULL ,  newserver , (void*) &newsockFD) < 0)
        {
            perror("could not create thread");
            return 1;
        }
		*/
	}

	//Close socket for server
	close(nodeSocketFD);
	return 0; 
}

//Accept new client/master
void* acceptMaster(void* connection) {

	//Buffer message for client-server communication
	char buffer[256];

	//Make an empty buff
	bzero(buffer, 256);

	//Loop controller
	int control = 1;
	//Write & Read return value
	int n = -1;
	//Connection var
	int* masterSocketFD = (int*) connection;

	//Welcoming hostname message
	char hostName[1024];
	char* welcome;
    gethostname(hostName, 1023);
    asprintf(&welcome, "# munin node at %s\n", hostName);
	n = write(*masterSocketFD, welcome, strlen(welcome));

	//Write error
	if (n < 0) {
		printError("Failed to write to client socket");
	}

	//The routine communication between client server
	while(control==1) {
		//Read buff msg from client
		n = read(*masterSocketFD, buffer, 255);

		//Read error
		if (n < 0) {
			printError("Failed to read from client socket");
		}

		char* respond;

		//The server tell the client that the message was accepted
		if(strcmp(buffer, "quit\n")==0){
			control = 0;
			break;
		} else {
			respond = messageHandler(buffer);
			n = write(*masterSocketFD, respond, strlen(respond));

			//Write error
			if (n < 0) {
				printError("Failed to write to client socket");
			}
		}

		//Reset the buff
		bzero(buffer, 256);
	}

	//Close the socket for client
	close(*masterSocketFD);

	//exit
	return 0;
}

//Preparation
void initialize() {

}

//Print error and exit
void printError(const char *message) {
    printf("%s\n",message);
    exit(1);
}

//Returning appropriate message based on input
char* messageHandler(const char* message) {
	if (strcmp(message, "cap\n")==0) {
		return ("cap multigraph dirtyconfig\n");
	} else
	if (strcmp(message, "list\n")==0) {
		return("memory\n");
	} else	 
	if (strcmp(message, "nodes\n")==0) {
		return("[hostname]\n");
	} else
	if (strcmp(message, "config\n")==0) {
		return("graph\n");
	} else
	if (strcmp(message, "fetch\n")==0) {
		return("used.value\n");
	} else
	if (strcmp(message, "version\n")==0) {
		return("Node on [hostname] version: 16.04\n");
	} else 
	if (strcmp(message, "quit\n")==0) {
		return ("Closing connection\n");
	} else {
		return ("# Unknown command. Try cap, list, nodes, config, fetch, version, or quit\n");
	}
}

/*
//
	
*/

