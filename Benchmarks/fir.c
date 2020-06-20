/* USAGE:
   The benchmark is executed as "./fir 198.2.34.12 9001"
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

#define COEF_SCALE 13       /* Filter coefficients are scaled */
#define DATA_SCALE 1        /* ...and so is the input data */
#define THRESHOLD  1000     /* Threshold for binary output */

#define	FIRLOW1_SECTIONS	35
#define FIRHI1_SECTIONS		35

#define HYSTERISIS (0.01*THRESHOLD) /* Hysterisis band around THRESHOLD */

typedef struct FILTER_DEF {
    int *coef ;     /* Pointer to coefficients of filter */
    int *history ;  /* Pointer to history for filter */
    int sections ;      /* Number of filter sections */
} FILTER_DEF ;


int main(int argc, const char* argv[] ) {
    printf("Task Name : FIR\n");
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
    // printf("Buffer : ");
    // for(int i=0; i<strlen(buff); i++) printf("%c",buff[i]);
    // printf("\n");
    const int len = atoi(buff);
    // printf("len %d\n",len);

    int inputArray[len];
    for(int i=0; i<len; i++) inputArray[i]=0;

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
              inputArray[j]=-1*inputArray[j];
              negative = 0;
            }
			      j++;
      }
      else if(full[i] == '-'){
          negative = 1;
          continue;
      }
		  else {
			     inputArray[j] = inputArray[j] * 10 + (full[i] - 48);
           if(i == length-1 && negative==1) inputArray[j]=-1*inputArray[j];
		  }
    }
    // printf("Assigned input Array\n");

    // printf("\nTask input received\n");
    // for(int i=0; i<len; i++) printf("%d ",inputArray[i]);
    // printf("\n\n");

    int coefHiPass[] = { 69, 20, -131, 110, 67, -132, -68, 229, -8, -315, 139, 406, -384, -479, 897, 529, -3127, 4454, -3127, 529, 897, -479, -384, 406, 139, -315, -8, 229, -68, -132, 67, 110, -131, 20, 69 };
    int coefLowPass[] = { -69, 20, 131, 110, -67, -132, 68, 229, 8, -315, -139, 406, 384, -479, -897, 529, 3127, 4454, 3127, 529, -897, -479, 384, 406, -139, -315, 8, 229, 68, -132, -67, 110, 131, 20, -69 };

    int outputArray[len*2];
    int index = 0;

    FILTER_DEF firHi1 = {
        coefHiPass,     /* Points to filter coefficients */
        NULL,           /* Placeholder for history pointer */
        35,             /* 35 sections */
    } ;

    FILTER_DEF firLow1 = {
        coefLowPass,    /* Points to filter coefficients */
        NULL,           /* Placeholder for history pointer */
        35,             /* 35 sections */
    } ;

    static int *coefficient1 ;
    static int *history1Low1 ;
    static int *history1Low2 ;
    static int *history1Hi1 ;
    static int *history1Hi2 ;
    static int signalOutLow1 ;
    static int signalOutHi1 ;
    int i1 ;
    int signal_in;   /* The input signal to be filtered */

    /* Allocate history arrays */
    firLow1.history = (int*)malloc( firLow1.sections * sizeof(int) ) ;
    firHi1.history = (int*)malloc( firHi1.sections * sizeof(int) ) ;

    /* Must clear out the history */

    /* ...for the low-pass filter */
    history1Low1 = firLow1.history ;
    for( i1 = 0 ; i1 < firLow1.sections ; i1++ ) *history1Low1++ = 0 ;

    /* ...and for the high-pass filter */
    history1Low1 = firHi1.history ;
    for( i1 = 0 ; i1 < firHi1.sections ; i1++ ) *history1Low1++ = 0 ;

    /* LOW-PASS FIR FILTER */
    for(int i=0; i<len; i++){
      signal_in = inputArray[i];
      coefficient1 = firLow1.coef + firLow1.sections - 1 ;
      history1Low1 = history1Low2 = firLow1.history ;
      signalOutLow1 = (int)( ( *coefficient1-- ) * ( *history1Low1++ ) ) ;
      signalOutLow1 += 1 << ( COEF_SCALE - 1 ) ;
      signalOutLow1 >>= COEF_SCALE ;

      /* Ripple through the filter history */
      for( i1 = 2 ; i1 < firLow1.sections ; i1++ ){
          *history1Low2++ = *history1Low1 ;
          signalOutLow1 += (int)
              ( ( *coefficient1-- ) * ( *history1Low1++ ) ) ;

          signalOutLow1 += 1 <<( COEF_SCALE - 1 ) ;
          signalOutLow1 >>= COEF_SCALE ;
      }

      *history1Low1 = signal_in ;
      signalOutLow1 += (int)( ( *coefficient1 ) * signal_in ) ;
      signalOutLow1 += 1 << ( COEF_SCALE - 1 ) ;
      signalOutLow1 >>= COEF_SCALE ;
      /* End of LOW-PASS FILTER */


      /* HIGH-PASS FIR FILTER */
      coefficient1 = firHi1.coef + firHi1.sections - 1 ;
      history1Hi1 = history1Hi2 = firHi1.history ;
      signalOutHi1 = (int)( ( *coefficient1-- ) * ( *history1Hi1++ ) ) ;
      signalOutHi1 += 1 << ( COEF_SCALE - 1 ) ;
      signalOutHi1 >>= COEF_SCALE ;

      for( i1 = 2 ; i1 < firHi1.sections ; i1++ ) {
          *history1Hi2++ = *history1Hi1 ;
          signalOutHi1 += (int)
              ( ( *coefficient1-- ) * ( *history1Hi1++ ) ) ;

          signalOutHi1 += 1 << ( COEF_SCALE - 1 ) ;
          signalOutHi1 >>= COEF_SCALE ;
      }

      *history1Hi1 = signal_in ;
      signalOutHi1 += (int)( ( *coefficient1 ) * signal_in ) ;
      signalOutHi1 += 1 << ( COEF_SCALE - 1 ) ;
      signalOutHi1 >>= COEF_SCALE ;
      /* End of HIGH-PASS FILTER */

      outputArray[index++] = signalOutHi1;
      outputArray[index++] = signalOutLow1;
      // printf( "%d \n",signalOutHi1 ) ;
      // printf( "%d \n",signalOutLow1 ) ;
    }

    char result[1000000];
    bzero(result, sizeof(result));
    int count = 0;
    for(int i=0; i<2*len; i++){
      char temp[20];
      sprintf(temp, "%d", outputArray[i]);
      for(int j=0; j<strlen(temp); j++){
        result[count] = temp[j];
        count++;
      }
      if(i<(2*len)-1){
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
}
