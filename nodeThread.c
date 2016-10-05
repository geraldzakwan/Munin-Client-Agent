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
	struct sockaddr_in serverAddress;//, clientAddress;

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
	//serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	//Set server port
	serverAddress.sin_port = htons(portNumber);

	//Error if port already used
	if (bind(nodeSocketFD, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) < 0) {
		printError("Failed to bind socket to server");	
	}
	
	//Let server socket to listen up to 10 client
	listen(nodeSocketFD, 10);

	//Don't know
	//clientLength = sizeof(clientAddress);

	//Loop to always accept new connection
	//while (masterSocketFD = accept(nodeSocketFD, (struct sockaddr *) &clientAddress, &clientLength)) {
	while (1) {
		//Thread Gua
		masterSocketFD = accept(nodeSocketFD, (struct sockaddr*) NULL, NULL);
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
	char rawBuffer[256];
	char *buffer;

	//Make an empty buff
	bzero(rawBuffer, 256);

	//Loop controller
	int control = 1;
	//Write & Read return value
	int ret = -1;
	//Connection var
	//int* masterSocketFD = (int*) connection;
	int masterSocketFD = *(int*) connection;

	//Welcoming hostname message
	char hostName[1024];
	char* welcome;
    gethostname(hostName, 1023);
    asprintf(&welcome, "# munin node at %s\n", hostName);
	//ret = write(*masterSocketFD, welcome, strlen(welcome));
	ret = write(masterSocketFD, welcome, strlen(welcome));
    printf("%s\n", welcome);
	//Write error
	if (ret < 0) {
		printError("Failed to write to client socket");
	}

	//The routine communication between client server
	while(control==1) {
		//Read buff msg from client
		ret = read(masterSocketFD, &rawBuffer, sizeof(char)*256);
		//ret = read(*masterSocketFD, buffer, 255);
		//Read error
		if (ret < 0) {
			printError("Failed to read from client socket");
		} else {
			printf("%s",rawBuffer);
		}

        buffer = strtok(rawBuffer, "\t\n\r");

        int temp = 0;
        while (buffer[temp] != ' ' && temp < strlen(buffer)) {
            temp += 1;
        }

        buffer[temp] = '\0';

		char* respond;

		//The server tell the client that the message was accepted
		if(strcmp(buffer, "quit")==0){
			control = 0;
			break;
		} else 
		if (strcmp(buffer,"cap") == 0) {
            asprintf(&respond, "cap multigraph dirtyconfig\n");
            ret = write(masterSocketFD, respond, strlen(respond));
            printf("%s\n", respond);
        } else 
        if (strcmp(buffer,"nodes") == 0) {
            asprintf(&respond, "%s\n", hostName);
            ret = write(masterSocketFD, respond, strlen(respond));
            printf("%s\n", respond);
            asprintf(&respond, ".\n");
            ret = write(masterSocketFD, respond, strlen(respond));
            printf("%s\n", respond);
        } else 
        if (strcmp(buffer, "list") == 0) {
            if (rawBuffer[temp+1] == '\0' || rawBuffer[temp+1] == '\n') {
                asprintf(&respond, "\n");
                ret = write(masterSocketFD, respond, strlen(respond));
                printf("%s\n", respond);
            } else {
                int temp2 = 0;
                char *buffer2 = &buffer[temp+1];
                while (buffer2[temp2] != ' ' && buffer2[temp2]) {
                    temp2 += 1;
                }
                buffer2[temp2] = '\0';
                if (strcmp(buffer2,hostName) == 0) {
                    asprintf(&respond, "memory\n");
                    ret = write(masterSocketFD, respond, strlen(respond));
                    printf("%s\n", respond);
                } else {
                    asprintf(&respond, "\n");
                    ret = write(masterSocketFD, respond, strlen(respond));
                    printf("%s\n", respond);
                }
            }
        } else
        if (strcmp(buffer,"config") == 0) {
            int temp2 = 0;
            char *buffer2 = &buffer[temp+1];
            while (buffer2[temp2] != ' ' && temp2 < strlen(buffer2)) {
                temp2 += 1;
            }
            buffer2[temp2] = '\0';
            if (strcmp(buffer2,"memory") != 0) {
                asprintf(&respond, "# Unknown service\n");
                ret = write(masterSocketFD, respond, strlen(respond));
                printf("%s\n", respond);
                asprintf(&respond, ".\n");
                ret = write(masterSocketFD, respond, strlen(respond));
                printf("%s\n", respond);
            } else {    
                FILE *fp;
                char path[512];
                fp = popen("free", "r");
                if (fp == NULL) {
                    printf("Failed to run command\n" );
                } else {
                    char totalMem[256];
                    fgets(path, sizeof(path)-1, fp);
                    fgets(path, sizeof(path)-1, fp);
                    int i;
                    i = 0;
                    int counter = 0;
                    while (i < strlen(path)) {
                        if (path[i] >= '0' && path[i] <= '9') {
                            int j = i;
                            while (path[j] >= '0' && path[j] <= '9') {
                                totalMem[j-i] = path[j];
                                j += 1;
                            }
                            break;
                        }
                        i += 1;
                    }
                    pclose(fp);
                    long long ntotalMem = atoi(totalMem);
                    ntotalMem *= 1024;
                    asprintf(&respond, "graph_args --base 1024 -l 0 --upper-limit %llu\n", ntotalMem);
                    ret = write(masterSocketFD, respond, strlen(respond));
                    printf("%s\n", respond);
                }
                asprintf(&respond, "graph_vlabel Bytes\n");
                ret = write(masterSocketFD, respond, strlen(respond));
                printf("%s\n", respond);
                asprintf(&respond, "graph_title Memory usage\n");
                ret = write(masterSocketFD, respond, strlen(respond));
                printf("%s\n", respond);
                asprintf(&respond, "graph_category system\n");
                ret = write(masterSocketFD, respond, strlen(respond));
                printf("%s\n", respond);
                asprintf(&respond, "graph_info This graph shows this machine memory.\n");
                ret = write(masterSocketFD, respond, strlen(respond));
                printf("%s\n", respond);
                asprintf(&respond, "graph_order used free\n");
                ret = write(masterSocketFD, respond, strlen(respond));
                printf("%s\n", respond);
                asprintf(&respond, "used.label used\n");
                ret = write(masterSocketFD, respond, strlen(respond));
                printf("%s\n", respond);
                asprintf(&respond, "used.draw STACK\n");
                ret = write(masterSocketFD, respond, strlen(respond));
                printf("%s\n", respond);
                asprintf(&respond, "used.info Used memory.\n");
                ret = write(masterSocketFD, respond, strlen(respond));
                printf("%s\n", respond);
                asprintf(&respond, "free.label free\n");
                ret = write(masterSocketFD, respond, strlen(respond));
                printf("%s\n", respond);
                asprintf(&respond, "free.draw STACK\n");
                ret = write(masterSocketFD, respond, strlen(respond));
                printf("%s\n", respond);
                asprintf(&respond, "free.info Free memory.\n");
                ret = write(masterSocketFD, respond, strlen(respond));
                printf("%s\n", respond);
                asprintf(&respond, ".\n");
                ret = write(masterSocketFD, respond, strlen(respond));
                printf("%s\n", respond);
            }
        } else 
        if (strcmp(buffer,"fetch") == 0) {
            int temp2 = 0;
            char *buffer2 = &buffer[temp+1];
            while (buffer2[temp2] != ' ' && temp2 < strlen(buffer2)) {
                temp2 += 1;
            }
            buffer2[temp2] = '\0';
            if (strcmp(buffer2,"memory") != 0) {
                asprintf(&respond, "# Unknown service\n");
                ret = write(masterSocketFD, respond, strlen(respond));
                printf("%s\n", respond);
                asprintf(&respond, ".\n");
                ret = write(masterSocketFD, respond, strlen(respond));
                printf("%s\n", respond);
            } else {    
                FILE *fp;
                char path[512];
                fp = popen("free", "r");
                if (fp == NULL) {
                    printf("Failed to run command\n" );
                } else {
                    char usedMem[256];
                    char freeMem[256];
                    fgets(path, sizeof(path)-1, fp);
                    fgets(path, sizeof(path)-1, fp);
                    int i;
                    i = 0;
                    int counter = 0;
                    while (i < strlen(path)) {
                        if (path[i] >= '0' && path[i] <= '9') {
                            int j = i;
                            if (counter == 0) {
                                while (path[j] >= '0' && path[j] <= '9') {
                                    j += 1;
                                }
                                counter += 1;
                            } else if (counter == 1) {
                                while (path[j] >= '0' && path[j] <= '9') {
                                    usedMem[j-i] = path[j];
                                    j += 1;
                                }
                                counter += 1;
                            } else if (counter == 2) {
                                while (path[j] >= '0' && path[j] <= '9') {
                                    freeMem[j-i] = path[j];
                                    j += 1;
                                }
                                counter += 1;
                            } else if (counter == 3) {
                                break;
                            }
                            i = j;
                        }
                        i += 1;
                    }
                    pclose(fp);
                    long long nusedMem = atoi(usedMem);
                    nusedMem *= 1024;
                    asprintf(&respond, "used.value %lld\n", nusedMem);
                    ret = write(masterSocketFD, respond, strlen(respond));
                    printf("%s\n", respond);
                    long long nfreeMem = atoi(freeMem);
                    nfreeMem *= 1024;
                    asprintf(&respond, "free.value %lld\n", nfreeMem);
                    ret = write(masterSocketFD, respond, strlen(respond));
                    printf("%s\n", respond);
                    asprintf(&respond, ".\n");
                    ret = write(masterSocketFD, respond, strlen(respond));
                    printf("%s\n", respond);
                }
            }
        }else 
        if (strcmp(buffer,"version") == 0) {
            asprintf(&respond, "Bakwan node on %s version: 2.0.25-2\n", hostName);
            ret = write(masterSocketFD, respond, strlen(respond));
            printf("%s\n", respond);
        } else {
            asprintf(&respond, "# Unknown command. Try cap, list, nodes, config, fetch, version or quit\n");
            ret = write(masterSocketFD, respond, strlen(respond));
            printf("%s\n", respond);
        }
		//Write error
		if (ret < 0) {
			printError("Failed to write to client socket");
		}

		/*
		else {
			respond = messageHandler(buffer);
			ret = write(*masterSocketFD, respond, strlen(respond));

			//Write error
			if (ret < 0) {
				printError("Failed to write to client socket");
			}
		}
		*/

		//Reset the buff
		bzero(buffer, 256);
	}

	//Close the socket for client
	//close(*masterSocketFD);
	close(masterSocketFD);

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

