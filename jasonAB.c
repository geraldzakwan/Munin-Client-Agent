//JASON ABE

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

int sockFD, newsockFD, portNumber;
socklen_t clientLength;
struct sockaddr_in serverAddress, clientAddress;
pthread_t threads;

void *newserver(void *arg); //int sock, struct sockaddr_in clientAddress
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argv, char *argc[])
{
     sockFD = socket(AF_INET, SOCK_STREAM, 0);
     if (sockFD < 0) 
        error("ERROR opening socket");
     bzero((char *) &serverAddress, sizeof(serverAddress));
     portNumber = 4949;
     serverAddress.sin_family = AF_INET;
     //serverAddress.sin_addr.s_addr = INADDR_ANY;
     serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
     serverAddress.sin_port = htons(portNumber);
     if (bind(sockFD, (struct sockaddr *) &serverAddress,
              sizeof(serverAddress)) < 0) 
              error("ERROR on binding");
     listen(sockFD,5);
     clientLength = sizeof(clientAddress);
     while((newsockFD = accept(sockFD, (struct sockaddr *) &clientAddress, &clientLength)))
    {    
        if(pthread_create(&threads , NULL ,  newserver , (void*) &newsockFD) < 0)
        {
            perror("could not create thread");
            return 1;
        }
    }
     close(sockFD);
     return 0;
}

void *newserver (void *arg)
{
    int files[16];
    int nf = 0;
    int n, input, port;
    char *buffer;
    char line[256];
    int sock = *(int*) arg;
    char *output;
    char hostname[1024];
    char *list;
    gethostname(hostname, 1023);
    asprintf(&output, "# munin node at %s\n", hostname);
    n = write(sock, output, strlen(output));
    do {
        n = read(sock,&line,sizeof(char)*256);
        buffer = strtok(line, "\t\n\r");
        printf("%s\n",line);
        int temp = 0;
        while (buffer[temp] != ' ' && temp < strlen(buffer)) {
            temp += 1;
        }
        buffer[temp] = '\0';
        if (strcmp(buffer,"cap") == 0) {
            asprintf(&output, "cap multigraph dirtyconfig\n");
            n = write(sock, output, strlen(output));
        } else if (strcmp(buffer,"nodes") == 0) {
            asprintf(&output, "%s\n", hostname);
            n = write(sock, output, strlen(output));
            asprintf(&output, ".\n");
            n = write(sock, output, strlen(output));
        } else if (strcmp(buffer, "list") == 0) {
            if (line[temp+1] == '\0' || line[temp+1] == '\n') {
                asprintf(&output, "memory\n");
                n = write(sock, output, strlen(output));
            } else {
                int temp2 = 0;
                char *buffer2 = &buffer[temp+1];
                while (buffer2[temp2] != ' ' && buffer2[temp2]) {
                    temp2 += 1;
                }
                buffer2[temp2] = '\0';
                if (strcmp(buffer2,hostname) == 0) {
                    asprintf(&output, "memory\n");
                    n = write(sock, output, strlen(output));
                } else {
                    asprintf(&output, "\n");
                    n = write(sock, output, strlen(output));
                }
            }
        } else if (strcmp(buffer,"config") == 0) {
            int temp2 = 0;
            char *buffer2 = &buffer[temp+1];
            while (buffer2[temp2] != ' ' && temp2 < strlen(buffer2)) {
                temp2 += 1;
            }
            buffer2[temp2] = '\0';
            if (strcmp(buffer2,"memory") != 0) {
                asprintf(&output, "# Unknown service\n");
                n = write(sock, output, strlen(output));
                asprintf(&output, ".\n");
                n = write(sock, output, strlen(output));
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
                    asprintf(&output, "graph_args --base 1024 -l 0 --upper-limit %llu\n", ntotalMem);
                    n = write(sock, output, strlen(output));
                }
                asprintf(&output, "graph_vlabel Bytes\n");
                n = write(sock, output, strlen(output));
                asprintf(&output, "graph_title Memory usage\n");
                n = write(sock, output, strlen(output));
                asprintf(&output, "graph_category system\n");
                n = write(sock, output, strlen(output));
                asprintf(&output, "graph_info This graph shows this machine memory.\n");
                n = write(sock, output, strlen(output));
                asprintf(&output, "graph_order used free\n");
                n = write(sock, output, strlen(output));
                asprintf(&output, "used.label used\n");
                n = write(sock, output, strlen(output));
                asprintf(&output, "used.draw STACK\n");
                n = write(sock, output, strlen(output));
                asprintf(&output, "used.info Used memory.\n");
                n = write(sock, output, strlen(output));
                asprintf(&output, "free.label free\n");
                n = write(sock, output, strlen(output));
                asprintf(&output, "free.draw STACK\n");
                n = write(sock, output, strlen(output));
                asprintf(&output, "free.info Free memory.\n");
                n = write(sock, output, strlen(output));
                asprintf(&output, ".\n");
                n = write(sock, output, strlen(output));
            }
        } else if (strcmp(buffer,"fetch") == 0) {
            int temp2 = 0;
            char *buffer2 = &buffer[temp+1];
            while (buffer2[temp2] != ' ' && temp2 < strlen(buffer2)) {
                temp2 += 1;
            }
            buffer2[temp2] = '\0';
            if (strcmp(buffer2,"memory") != 0) {
                asprintf(&output, "# Unknown service\n");
                n = write(sock, output, strlen(output));
                asprintf(&output, ".\n");
                n = write(sock, output, strlen(output));
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
                    asprintf(&output, "used.value %lld\n", nusedMem);
                    n = write(sock, output, strlen(output));
                    long long nfreeMem = atoi(freeMem);
                    nfreeMem *= 1024;
                    asprintf(&output, "free.value %lld\n", nfreeMem);
                    n = write(sock, output, strlen(output));
                    asprintf(&output, ".\n");
                    n = write(sock, output, strlen(output));
                }
            }
        } else if (strcmp(buffer,"version") == 0) {
            asprintf(&output, "h3h3 node on MyComputer version: 3.21\n");
            n = write(sock, output, strlen(output));
        } else if (strcmp(buffer,"quit") == 0) {
            break;
        } else {
            asprintf(&output, "# Unknown command. Try cap, list, nodes, config, fetch, version or quit\n");
            n = write(sock, output, strlen(output));
        }
    } while (1);
    close(sock);
    return NULL;
}