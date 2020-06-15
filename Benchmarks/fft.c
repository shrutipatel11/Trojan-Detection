#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#define SA struct sockaddr

/* TRIG_SCALE_FACTOR is used for greatest twiddle factor precision */
#define TRIG_SCALE_FACTOR 32768

/* BUTTERFLY_SCALE_FACTOR = log2( TRIG_SCALE_FACTOR )is used to
accomodate accumulator size limit of 32 bits */
#define BUTTERFLY_SCALE_FACTOR  15          /* Divide by 32768 */

/* STAGE_SCALE_FACTOR is used to prevent overflow from
accumulation at each processing stage */
#define STAGE_SCALE_FACTOR      1           /* Divide by 2 */

#ifdef PI
#undef PI
#endif
#define PI 3.141592654

// #define NUM_POINTS   16   /* Number of points in FFT */
// #define FFT_LENGTH   5     /* length = log2( NUM_POINTS )*/

int main(int argc, const char* argv[] )
{
    printf("Task Name : FFT\n");
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
    const int NUM_POINTS = atoi(buff);
    const int FFT_LENGTH = (int)log2(NUM_POINTS);
    // printf("Length %d\n",NUM_POINTS);

    int realData_1[NUM_POINTS] ;    /* Points to real part of data */
    int imagData_1[NUM_POINTS] ;    /* Points to imaginary part of data */
    for(int i=0; i<NUM_POINTS;  i++) realData_1[i]=0;

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
              realData_1[j]=-1*realData_1[j];
              negative = 0;
            }
			      j++;
      }
      else if(full[i] == '-'){
          negative = 1;
          continue;
      }
		  else {
			     realData_1[j] = realData_1[j] * 10 + (full[i] - 48);
           // printf("%d",buff[i]-48);
           imagData_1[j] = 0;
              if(i == length-1 && negative==1) realData_1[j]=-1*realData_1[j];
		  }
    }
    // printf("Assigned input Array\n");

    // printf("\nTask input received\n");
    // for(int i=0; i<NUM_POINTS; i++) printf("%d ",realData_1[i]);
    // printf("\n\n");

    int bitRevInd[NUM_POINTS] ;
    double trigArg ;
    int index ;
    int brIndex ;
    int sineV[NUM_POINTS / 2] ;
    int cosineV[NUM_POINTS / 2] ;
    int realBitRevData_1[NUM_POINTS] ;
    int imagBitRevData_1[NUM_POINTS] ;
    static int wReal_1 ;
    static int wImag_1 ;
    static int tRealData_1 ;
    static int tImagData_1 ;
    static int *realLow_1 ;
    static int *imagLow_1 ;
    static int *realHi_1 ;
    static int *imagHi_1 ;
    static long argIndex_1 ;
    static long deltaIndex_1 ;
    static int n1_1 ;
    static int n2_1 ;
    static int l_1 ;
    static int i_1 ;
    static int j_1 ;
    static int k_1 ;
    // printf("Initialised the variables\n");

    /* Populate the sine & cosine tables -- used by all instances */
    for( i_1 = 0 ; i_1 < ( NUM_POINTS / 2 ) ; i_1++ ){
        trigArg = (double)( i_1 * PI / ( NUM_POINTS / 2 ) ) ;
        if( sin( trigArg ) == 1.0 ) sineV[i_1] = (long)( TRIG_SCALE_FACTOR - 1 ) ;
        else sineV[i_1] = (long)( sin( trigArg ) * TRIG_SCALE_FACTOR ) ;

        if( cos( trigArg ) == 1.0 ) cosineV[i_1] = (long)( TRIG_SCALE_FACTOR - 1 ) ;
        else cosineV[i_1] = (long)( cos( trigArg ) * TRIG_SCALE_FACTOR ) ;
    }
    // printf("Populated sine and cosine tables\n");


    /* Compute the bit reversal indicies  -- used by all the instances */
    for( i_1 = 0 ; i_1 < NUM_POINTS ; i_1++ ){
        index = i_1 ;
        brIndex = 0 ;
        for( j_1 = 0 ; j_1 < FFT_LENGTH ; j_1++ ){
            brIndex <<= 1 ;
            if( 0x01 &index ) brIndex |= 0x01 ;
            index >>= 1 ;
        }
        bitRevInd[i_1] = brIndex ;
    }

    /* Bit Reversal */
    for( i_1 = 0 ; i_1 < NUM_POINTS ; i_1++ ){
        realBitRevData_1[i_1] = realData_1[bitRevInd[i_1]] ;
        imagBitRevData_1[i_1] = imagData_1[bitRevInd[i_1]] ;
    }

    /* Return bit reversed data to input arrays */
    for( i_1 = 0 ; i_1 < NUM_POINTS ; i_1++ ){
        realData_1[i_1] = realBitRevData_1[i_1] ;
        imagData_1[i_1] = imagBitRevData_1[i_1] ;
    }
    // printf("Computed bit reversal indices\n");

    /* FFT Computation */

    /* Step through the stages */
    for( int passCount_1 = 0, k_1 = 1 ; k_1 <= FFT_LENGTH ; k_1++, passCount_1++ ){
        n1_1 = 1 << k_1 ;
        n2_1 = n1_1 >> 1 ;

        /* Initialize twiddle factor lookup indicies */
        argIndex_1 = 0 ;
        deltaIndex_1 = ( NUM_POINTS / 2 ) / n2_1 ;
        // printf("\n Twiddle : %d\n",passCount_1);

        /* Step through the butterflies */
        for( j_1 = 0 ; j_1 < n2_1 ; j_1++, passCount_1++ ){
            /* Lookup twiddle factors */
            wReal_1 = cosineV[argIndex_1] ;
            wImag_1 = sineV[argIndex_1] ;

            /* Process butterflies with the same twiddle factors */
            for( i_1 = j_1 ; i_1 < NUM_POINTS ; i_1 += n1_1, passCount_1++ ) {
                l_1 = i_1 + n2_1 ;
                // printf("l_1 i_1 n2_1 %d %d %d\n",l_1,i_1,n2_1);
                realLow_1 = &realData_1[l_1] ;
                imagLow_1 = &imagData_1[l_1] ;
                realHi_1 = &realData_1[i_1] ;
                imagHi_1 = &imagData_1[i_1] ;

                /* Scale each stage to prevent overflow */
                *realLow_1 >>= STAGE_SCALE_FACTOR ;
                *imagLow_1 >>= STAGE_SCALE_FACTOR ;
                *realHi_1 >>= STAGE_SCALE_FACTOR ;
                *imagHi_1 >>= STAGE_SCALE_FACTOR ;

                tRealData_1 = *realLow_1 * wReal_1 - *imagLow_1 * wImag_1 ;
                tImagData_1 = *imagLow_1 * wReal_1 + *realLow_1 * wImag_1 ;

                /* Scale twiddle products to accomodate
                 * 32bit accumulator */
                tRealData_1 >>= BUTTERFLY_SCALE_FACTOR ;
                tImagData_1 >>= BUTTERFLY_SCALE_FACTOR ;

                realData_1[l_1] = *realHi_1 - tRealData_1 ;
                imagData_1[l_1] = *imagHi_1 - tImagData_1 ;
                realData_1[i_1] += tRealData_1 ;
                imagData_1[i_1] += tImagData_1 ;
            }
            argIndex_1 += deltaIndex_1 ;
        }
        // printf("Step through the butterflies\n");
    } /* End of FFT loop */
    // printf("End of FFT loop\n");
    // printf("Length of outputArray : %ld \n",sizeof(realData_1)/sizeof(realData_1[0]));

    char result[5000000];
    bzero(result, sizeof(result));
    int count = 0;
  	for(int i=0; i<NUM_POINTS; i++){
      char temp[20];
      sprintf(temp, "%d", realData_1[i]);
      for(int j=0; j<strlen(temp); j++){
        result[count] = temp[j];
        count++;
      }
      if(i<NUM_POINTS-1){
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
