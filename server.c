/* 
TCP Server
Author : Geraldi Dzakwan
*/

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

void* acceptMaster(void* connection);

int main(int argc, char *argv[])
{
	pthread_t connectionThread[10];

	int portNumber = 4949, nodeSocketFD, masterSocketFD, connectionCount = 0, *n;
	*n = 0;

	struct sockaddr_in serverAddress;

	nodeSocketFD = socket(AF_INET, SOCK_STREAM, 0);

	bzero((char*) &serverAddress, sizeof(serverAddress));

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddress.sin_port = htons(portNumber);

	bind(nodeSocketFD, (struct sockaddr*) &serverAddress, sizeof(serverAddress));
	
	listen(nodeSocketFD, 5);

	//Loop to always accept new connection
	while (*n = accept(nodeSocketFD, (struct sockaddr*) NULL, NULL)) {
		//Thread
		pthread_create(&connectionThread[connectionCount], NULL, acceptMaster, (void*) n);
		connectionCount++;
	}

	close(nodeSocketFD);
	return 0; 
}

//Accept new client/master
void* acceptMaster(void* connection) {
	int masterSocketFD = *(int*) connection, ret = 0, control =1;
	char rawBuffer[256];

	//HostName
	char hostName[1024];
	char* welcome;
    gethostname(hostName, 1023);
    asprintf(&welcome, "# munin node at %s\n", hostName);
	ret = write(masterSocketFD, welcome, strlen(welcome));

	while(control==1) {
		ret = read(masterSocketFD, &rawBuffer, sizeof(char)*256);

		char* respond;

		if (strcmp(rawBuffer,"fetch") == 0) {
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
                long long nfreeMem = atoi(freeMem);
                nfreeMem *= 1024;
                asprintf(&respond, "free.value %lld\n", nfreeMem);
                ret = write(masterSocketFD, respond, strlen(respond));
                asprintf(&respond, ".\n");
                ret = write(masterSocketFD, respond, strlen(respond));
            }
        } else {
        	//message-message lain
        	break;
        }
	}

	close(masterSocketFD);

	return 0;
}

