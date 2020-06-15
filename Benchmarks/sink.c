#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#define SA struct sockaddr

int main(int argc, const char* argv[] )
{
    printf("Task Name : SINK\n");
    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;

    int port;
    port = atoi(argv[2]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        // printf("Socket creation failed...\n");
        exit(0);
    }
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    servaddr.sin_port = htons(port);
    bzero(&(servaddr.sin_zero),8);

    // printf("Port number : %d\n",port);
    // connect the client socket to server socket
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        // printf("Connection with the server failed...\n");
        exit(0);
    }

    char buff[60000];
    bzero(buff, sizeof(buff));
    read(sockfd, buff, sizeof(buff));
    // printf("array length received ");
    // for(int i=0;i<strlen(buff);i++) printf("%c",buff[i]);
    printf("\n");

    char result[1000000];
    int ii=0;
    while(1){
      bzero(buff, sizeof(buff));
      read(sockfd, buff, sizeof(buff));
      // printf("Recived buffer len %ld \n",strlen(buff));
      if(strlen(buff)==1) break;
      for(int j=0; j<strlen(buff); j++){
        result[ii++] = buff[j];
      }
    }

    result[ii]='A';

    int n = strlen(result);
    // printf("This length should be received %d \n",n);
    char sendi[60000];
    int send_index = 0;
    while(send_index < n){
      bzero(sendi, sizeof(sendi));
      for(int j=0; j<sizeof(send); j++){
        sendi[j] = result[send_index++];
        if(send_index == n) break;
      }
      send(sockfd, sendi, strlen(sendi), 0);
      if(send_index == n) break;
    }
    // close the socket
    close(sockfd);
}
