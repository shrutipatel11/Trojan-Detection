/* USAGE:
   The benchmark is executed as "./angle 198.2.34.12 9001"
   argv[1] = IP address of the 3PC/HGC slave
   argv[2] = Port number to bind the socket
   
   AFTER CONNECTION:
   1. The array length is sent by the slave
   2. The array contents are sent by the slave 
      FORMAT OF CONTENTS: comma separated string of numbers (Ex. 3,45,23,678,123,764)
   3. the delimiter 'A' is sent to mark the end of input.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#define SA struct sockaddr

#define CYLINDERS       8       /* We're simulating an eight cylinder engine */
#define TENTH_DEGREES   3600    /* Number of 1/10-degrees in a circle */

#define FIRE1_ANGLE (TENTH_DEGREES/CYLINDERS*1)
#define FIRE2_ANGLE (TENTH_DEGREES/CYLINDERS*2)
#define FIRE3_ANGLE (TENTH_DEGREES/CYLINDERS*3)
#define FIRE4_ANGLE (TENTH_DEGREES/CYLINDERS*4)
#define FIRE5_ANGLE (TENTH_DEGREES/CYLINDERS*5)
#define FIRE6_ANGLE (TENTH_DEGREES/CYLINDERS*6)
#define FIRE7_ANGLE (TENTH_DEGREES/CYLINDERS*7)
#define FIRE8_ANGLE (TENTH_DEGREES/CYLINDERS*8)

#define CYL1            1       /* Cylinder #1 firing window */
#define CYL2            2       /* Cylinder #2 firing window */
#define CYL3            3       /* Cylinder #3 firing window */
#define CYL4            4       /* Cylinder #4 firing window */
#define CYL5            5       /* Cylinder #5 firing window */
#define CYL6            6       /* Cylinder #6 firing window */
#define CYL7            7       /* Cylinder #7 firing window */
#define CYL8            8       /* Cylinder #8 firing window */

#define TDC_TEETH       2       /* Number of missing teeth (=1) at TDC */
#define TDC_MARGIN      0.9     /* Discrimination window for TDC teeth */
#define NUM_TEETH       60      /* Number of teeth on tonewheel */

#define RPM_SCALE_FACTOR          3600000
#define MAX_VARIABLE 0x7FFF

float main(int argc, const char* argv[] ){
  printf("Task Name : ANGLE\n");
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

  int inputArray[len];
  for(int i=0; i<len;  i++) inputArray[i]=0;
  int outputArray[len];
  int index = 0;

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

  float angleCounter ;      /* Current 'angleCounter' pulled from test data */
  float *inpAngleCount ;    /* Array of 'angleCounter' test data values */
  int tonewheelTeeth ;    /* Number of teeth on the tonewheel */

  float window ;
  static float pulseDeltaTime1 ;
  static float angle1 ;
  static float angleCounterLast1 ;
  static int toothCount1 ;
  static float deltaTimeAccum1 ;
  static float deltaTimeAvg1 ;
  static float firingTime1 ;
  static long tdcTime1 ;
  static long engineSpeed1 ;
  static long rotationTime1 ;
  static float isTopDeadCenter1 ; /* TRUE/FALSE flag when TDC occurs */

  toothCount1 = 0 ;       /* Don't know which pulse we start on */
  deltaTimeAccum1 = 0 ;   /* ...and haven't accumulated for filter... */
  deltaTimeAvg1 = 32767 ; /* ...and not gotten an average... */
  tdcTime1 = 0 ;          /* ...and don't know when TDC occurs */
  angleCounterLast1 = 0 ;
  engineSpeed1 = 0 ;
  rotationTime1 = 0 ;
  firingTime1 = 0 ;

  // angleCounter	= 123.90;   /* Current 'angleCounter' pulled from  data */
	// // inpAngleCount	= NULL; /* Array of 'angleCounter' test data values */
	tonewheelTeeth	= 10; /* Number of teeth on the tonewheel */
  window = TENTH_DEGREES / tonewheelTeeth ; /* Only need to do this once */

  for(int i=0; i<len; i++){
    angleCounter = inputArray[i];
    if( angleCounterLast1 > angleCounter ) pulseDeltaTime1 = angleCounter + ( (int)MAX_VARIABLE - angleCounterLast1 + 1 ) ;
    else pulseDeltaTime1 = angleCounter - angleCounterLast1 ;

    angleCounterLast1 = angleCounter ;
    rotationTime1 += pulseDeltaTime1 ;

    if( pulseDeltaTime1 > ( TDC_TEETH *deltaTimeAvg1 *TDC_MARGIN ) ) {
        isTopDeadCenter1 = 1 ;
        pulseDeltaTime1 /= TDC_TEETH ;
        tdcTime1 = rotationTime1 ;
        rotationTime1 = 0 ;
        engineSpeed1 = RPM_SCALE_FACTOR / tdcTime1 ;
        toothCount1 = 0 ;
    }
    else{
        toothCount1++ ;
        isTopDeadCenter1 = 0 ;
    }

    deltaTimeAccum1 += pulseDeltaTime1 ;
    if( ( toothCount1 > 0 ) && ( toothCount1 %( tonewheelTeeth / CYLINDERS ) == 0 ) ) {
        deltaTimeAvg1 = deltaTimeAccum1 / ( tonewheelTeeth / CYLINDERS ) ;
        deltaTimeAccum1 = 0 ;
    }
    angle1 = ( TENTH_DEGREES * toothCount1 / tonewheelTeeth ) ;

    /* CYLINDER 1 */
    if( ( angle1 >= ( ( CYL1 * TENTH_DEGREES / CYLINDERS ) - window ) ) &&
        ( angle1 < ( CYL1 * TENTH_DEGREES / CYLINDERS ) ) )
    {
        firingTime1 = ( ( FIRE1_ANGLE - angle1 ) * tdcTime1 / TENTH_DEGREES ) + angleCounter ;
    }

    /* CYLINDER 2 */
    if( ( angle1 >= ( ( CYL2 * TENTH_DEGREES / CYLINDERS ) - window ) ) &&
        ( angle1 < ( CYL2 * TENTH_DEGREES / CYLINDERS ) ) )
    {
        firingTime1 = ( ( FIRE2_ANGLE - angle1 ) * tdcTime1 / TENTH_DEGREES ) + angleCounter ;
    }

    /* CYLINDER 3 */
    if( ( angle1 >= ( ( CYL3 * TENTH_DEGREES / CYLINDERS ) - window ) ) &&
        ( angle1 < ( CYL3 * TENTH_DEGREES / CYLINDERS ) ) )
    {
        firingTime1 = ( ( FIRE3_ANGLE - angle1 ) * tdcTime1 / TENTH_DEGREES ) + angleCounter ;
    }

    /* CYLINDER 4 */
    if( ( angle1 >= ( ( CYL4 * TENTH_DEGREES / CYLINDERS ) - window ) ) &&
        ( angle1 < ( CYL4 * TENTH_DEGREES / CYLINDERS ) ) )
    {
        firingTime1 = ( ( FIRE4_ANGLE - angle1 ) * tdcTime1 / TENTH_DEGREES ) + angleCounter ;
    }

  #if( CYLINDERS > 4 )
    /* CYLINDER 5 */
    if( ( angle1 >= ( ( CYL5 * TENTH_DEGREES / CYLINDERS ) - window ) ) &&
        ( angle1 < ( CYL5 * TENTH_DEGREES / CYLINDERS ) ) )
    {
        firingTime1 = ( ( FIRE5_ANGLE - angle1 ) * tdcTime1 / TENTH_DEGREES ) + angleCounter ;
    }

    /* CYLINDER 6 */
    if( ( angle1 >= ( ( CYL6 * TENTH_DEGREES / CYLINDERS ) - window ) ) &&
        ( angle1 < ( CYL6 * TENTH_DEGREES / CYLINDERS ) ) )
    {
        firingTime1 = ( ( FIRE6_ANGLE - angle1 ) * tdcTime1 / TENTH_DEGREES ) + angleCounter ;
    }

  #if( CYLINDERS > 6 )
    /* CYLINDER 7 */
    if( ( angle1 >= ( ( CYL7 * TENTH_DEGREES / CYLINDERS ) - window ) ) &&
        ( angle1 < ( CYL7 * TENTH_DEGREES / CYLINDERS ) ) )
    {
        firingTime1 = ( ( FIRE7_ANGLE - angle1 ) * tdcTime1 / TENTH_DEGREES ) + angleCounter ;
    }

    /* CYLINDER 8 */
    if( ( angle1 >= ( ( CYL8 * TENTH_DEGREES / CYLINDERS ) - window ) ) &&
        ( angle1 < ( CYL8 * TENTH_DEGREES / CYLINDERS ) ) )
    {
        firingTime1 = ( ( FIRE8_ANGLE - angle1 ) * tdcTime1 / TENTH_DEGREES ) +angleCounter ;
    }
  #endif /* 6 cylinders */
  #endif /* 4 cylinders */

    if( firingTime1 > MAX_VARIABLE ) firingTime1 -= MAX_VARIABLE ;

    /* Output the 'firingTime result */
    // printf("%f \n",firingTime1);
    outputArray[index++] = firingTime1;
  }//end of for

  int output[len];
  for(int i=0; i<len; i++) output[i] = (int)outputArray[i];

  char result[1000000];
  bzero(result, sizeof(result));
  int count = 0;
  for(int i=0; i<len; i++){
    char temp[20];
    sprintf(temp, "%d", output[i]);
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
