/* USAGE:
   The benchmark is executed as "./road 198.2.34.12 9001"
   argv[1] = IP address of the 3PC/HGC slave
   argv[2] = Port number to bind the socket
   
   AFTER CONNECTION:
   1. The array length is received from the slave
   2. The array contents are received from the slave 
      FORMAT OF CONTENTS: comma separated string of numbers (Ex. 3,45,23,678,123,764)
   3. the delimiter 'A' is received to mark the end of input.
   
   AFTER THE TASK IS EXECUTED:
   1. Send the result of the task to the slave along with the delimiter 'A'
      FORMAT OF RESULT : comma separated string of numbers with 'A' being the last character (Ex. 3,45,23,678,123,764A)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#define SA struct sockaddr

#define false    0
#define true    !false

#define SPEEDO_SCALE_FACTOR 36000
#define NUM_TEETH           60
#define MIN_TOOTH_TIME      100
#define MAX_TOOTH_TIME      10000
#define MAX_VARIABLE 0x7FFF

int main(int argc, const char* argv[] ){
  printf("Task Name : ROAD\n");
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
  const int len = atoi(buff);

  int inpCounter[len];
  int outputArray[len];
  int index = 0;
  for(int i=0; i<len;  i++) inpCounter[i]=0;

  char full[900000];
  int ii=0;
  while(1){
    bzero(buff, sizeof(buff));
    read(sockfd, buff, sizeof(buff));
    // printf("Recived buffer len %ld \n",strlen(buff));
    if(strlen(buff)==1) break;
    for(int j=0; j<strlen(buff); j++){
      full[ii++] = buff[j];
    }
  }

  int length = strlen(full);
  int negative = 0,j=0;
  for(int i=0; i<length; i++){
    if (full[i] == ',') {
          if(negative==1){
            inpCounter[j]=-1*inpCounter[j];
            negative = 0;
          }
          j++;
    }
    else if(full[i] == '-'){
        negative = 1;
        continue;
    }
    else {
         inpCounter[j] = inpCounter[j] * 10 + (full[i] - 48);
         if(i == length-1 && negative==1) inpCounter[j]=-1*inpCounter[j];
    }
  }

  // printf("\nTask input received\n");
  // for(int i=0; i<len; i++) printf("%d ",inpCounter[i]);
  // printf("\n");

  int i ;
  static int roadSpeed1 ;
  static int toothDeltaTime1 ;
  static int toothDeltaTimeLast1 ;
  static int tonewheelCounterLast1 ;
  static int toothTimeAccum1 ;
  static int toothCount1 ;
  int tonewheelTeeth = NUM_TEETH;
  int tonewheelCounter;

  toothCount1 = 0 ;
  toothTimeAccum1 = 0 ;
  roadSpeed1 = 0 ;
  tonewheelCounterLast1 = 0 ;
  toothDeltaTimeLast1 = MAX_TOOTH_TIME ;

  for(int ii=0; ii<len; ii++){
      tonewheelCounter = inpCounter[ii];
      toothDeltaTime1 = (int) ( tonewheelCounter - tonewheelCounterLast1 ) ;
      if( tonewheelCounterLast1 > tonewheelCounter )  toothDeltaTime1 += ( int )( MAX_VARIABLE + 1 );

      if( toothDeltaTime1 < MIN_TOOTH_TIME )  toothDeltaTime1 = toothDeltaTimeLast1 ;
      if( toothDeltaTime1 > 4 * toothDeltaTimeLast1 )  toothDeltaTime1 = toothDeltaTimeLast1 ;

      tonewheelCounterLast1 = tonewheelCounter ;
      toothTimeAccum1 += toothDeltaTime1 ;
      toothCount1++ ;
      toothDeltaTimeLast1 = toothDeltaTime1 ;

      if( toothCount1 >= tonewheelTeeth / 2 ){
          if( toothTimeAccum1 > MAX_TOOTH_TIME *tonewheelTeeth / 2 ) roadSpeed1 = 0 ;
          else {
              roadSpeed1 = (int ) ( SPEEDO_SCALE_FACTOR  / ( toothTimeAccum1 / tonewheelTeeth * 2 ) ) ;
              toothCount1 = 0 ;
              toothTimeAccum1 = 0 ;
          }
      }
      outputArray[index++] = roadSpeed1;
  }//end of for

  char result[1000000];
  bzero(result, sizeof(result));
  int count = 0;
  for(int i=0; i<len; i++){
    char temp[20];
    sprintf(temp, "%d", outputArray[i]);
    for(int j=0; j<strlen(temp); j++){
      result[count] = temp[j];
      count++;
    }
    if(i<len-1){
      result[count]=',';
      count++;
    }
  }
  result[count]='A';

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
