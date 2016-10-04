/* 
TCP Server, port served as an argument 
*/

//All the directive header needed
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

//Buffer message for client-server communication
char buffer[256];

//Preparation
void initialize();

//Set buff char with 0
void clearBuff();

//Print error and exit
void printError(const char *message);

//Returning appropriate message based on input
char* messageHandler(const char* message);

int main(int argc, char *argv[])
{
	//Preparation
	initialize();

	//Variable for socket and port
	int portNumber, sockFD, newsockFD;

	//Struct server & client
	struct sockaddr_in serverAddress, clientAddress;

	//Length of socket client
	socklen_t clientLength;

	//Error if no port specified
	if (argc < 2) {
		printError("No port specified");
	} else {
		//Set port number
		portNumber = atoi(argv[1]);
	}

	//Create socket
	sockFD = socket(AF_INET, SOCK_STREAM, 0);

	//Error if socket creation fail
	if (sockFD < 0) {
		printError("Failed to create/open socket");
	}

	//Make serverAddress, clear the buff
	bzero((char *) &serverAddress, sizeof(serverAddress));

	//Set server family
	serverAddress.sin_family = AF_INET;
	//Set server address
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	//Set server port
	serverAddress.sin_port = htons(portNumber);

	//Error if ...
	if (bind(sockFD, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
		printError("Failed tp bind socket to server");	
	}
	
	//Let server socket to listen up to 10 client
	listen(sockFD,10);

	//Don't know
	clientLength = sizeof(clientAddress);

	//Create socket for client
	newsockFD = accept(sockFD, (struct sockaddr *) &clientAddress, &clientLength);

	//Error if ...
	if (newsockFD < 0) {
		printError("Failed to accept");
	}
	
	//Clear the char/msg buff
	clearBuff();

	int control = 1;
	int n = -1;
	while (control==1) {
		//Read buff msg from client
		n = read(newsockFD,buffer,255);

		//Error if ...
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
			n = write(newsockFD, respond, strlen(respond));

			//Error if ...
			if (n < 0) {
				printError("Failed to write to client socket");
			}
		}

		//Reset the buff
		clearBuff();
	}

	//Close socket for client
	close(newsockFD);

	//Close socket for server
	close(sockFD);
	return 0; 
}

//Preparation
void initialize() {

}

//Set buff char with 0
void clearBuff() {
	bzero(buffer, 256);
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