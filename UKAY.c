//UKAY

#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 

void *acceptclient(void *conn){
    int *c = (int *) conn;
    char Cinput[20];
    char Coutput[256];
    while(1)
        {    
            //ticks = time(NULL);
            read(*c, Cinput, sizeof(Cinput));
            //printf("%d\n", (int)sizeof(Cinput));
            if (strcmp(Cinput,"quit")){
                printf("selamat\n");
            }
            printf("Server: received %s", Cinput);
            /*printf("Please enter the message: ");
            fgets(Coutput,255,stdin);
            write(connfd, Coutput, sizeof(Coutput));*/
            sleep(1);
         }
    close(*c);
}

int main(int argc, char *argv[])
{
    pthread_t pth[55];
    int i = 0;
    int listenfd = 0;
    struct sockaddr_in serverAddress; 

    char sendBuff[1025];
    time_t ticks; 

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serverAddress, '0', sizeof(serverAddress));
    memset(sendBuff, '0', sizeof(sendBuff)); 

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(4950); 

    bind(listenfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)); 

    listen(listenfd, 10); 
    while(1) {
        //connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
        int *connfd;
        *connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
        pthread_create(&pth[i], NULL,  acceptclient, (void *) connfd) ; 
        i++;       
    }
}