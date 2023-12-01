#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/select.h>
#include <errno.h>
#include <time.h>

#include "socket-utils.h"

double total_looptime=0,total_response_time=0;
size_t n_req=0, n_err=0, n_timeouts=0, n_success=0;
// char **requestIDs;
// int requestIDCount = 0;

void error(char *msg) {
  perror(msg);
  exit(0);
}

int new(int sockfd, char *filePath) {
  int n;
  char buffer[4096];
  n=write(sockfd,"n",1);
  if(n<0){
    error("ERROR writing to socket");
    return 1;
  }// Handle request type: new
  n = send_file(sockfd, filePath);
  if(n<0){
    error("ERROR writing to socket");
    return 1;}
  n_req++;
  
  //receive response from server
  bzero(buffer,4096);
  n=read(sockfd,buffer,4095);

  if (n < 0) {
    if(errno == EWOULDBLOCK){
      perror("Recv timout:");
      n_timeouts++;
      return 2;
    }
    else if(errno == ECONNRESET){
      perror("ERROR reading from socket\n");
      n_err++;
      return 3;
    }
  } else {
    n_success++;
    // Message: uuid
    printf("Your Request ID is: %s\n",buffer);
    // requestIDs[requestIDCount] = malloc(37 * sizeof(char));
    // strncpy(requestIDs[requestIDCount++], buffer, 37);
  }
  return 0;
}

int status(int sockfd, char *requestID, int *submissionStatus) {
  int n;
  n=write(sockfd,"s", 1);
  if(n<0) {
    error("ERROR writing to socket");
    return 1;
  }
  puts("Request ID: ");
  puts(requestID);
  n=write(sockfd, requestID, 37);
  if(n<0) {
    error("ERROR writing to socket");
    return 1;
  }
  n_req++;
  puts("Sent");
  if(print_recv_file(sockfd, stdout, submissionStatus) < 0)
    error("ERROR reading message+file? from socket");
  return 0;
}

int main(int argc, char *argv[]) {
  int sockfd, portno;

  struct sockaddr_in serv_addr; //Socket address structure
  struct hostent *server; //return type of gethostbyname

  char buffer[4096]; //buffer for message

  if (argc < 8) {
    fprintf(stderr, "Usage:\n%s <new|status> <hostname> <portno> <filename|requestID> <loopnum> <sleep> <timeout>\n", argv[0]);
    exit(0);
  }
  
  portno = atoi(argv[3]); // 2nd argument of the command is port number
  int loopnum = atoi(argv[5]);
  int sleep_secs = atoi(argv[6]);

  
  struct timeval loopstart,loopend;
  gettimeofday(&loopstart,NULL);
  size_t timeout = atoi(argv[7]);
  for(int i=0;i<loopnum;i++) {
    puts("");
    puts("");

    struct timeval respo_start,respo_end;
    gettimeofday(&respo_start,NULL);
  
    sockfd = socket(AF_INET, SOCK_STREAM, 0); //create the half socket


    struct timeval time_out;
    time_out.tv_sec = timeout;
    time_out.tv_usec = 0; //This ensures that only the timeout in seconds (tv_sec) is considered, and there are no microseconds of timeout.
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &time_out, sizeof time_out) == -1){  // attempts to set a receive timeout on the socket referred to by sockfd
      perror("Set socket timeout failed");
      close(sockfd);
      exit(1);
    }
  
    if (sockfd < 0)
      error("ERROR opening socket");

    server = gethostbyname(argv[2]);

    if (server == NULL) {
      fprintf(stderr, "ERROR, no such host\n");
      exit(0);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr)); // set server address bytes to zero

    serv_addr.sin_family = AF_INET; // Address Family is IP

    bcopy((char *)server->h_addr_list[0], (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);

    serv_addr.sin_port = htons(portno);


    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
      error("ERROR connecting");
    }
    bzero(buffer,4096);

    int submissionStatus = 1;

    puts(argv[1]);
    puts(argv[4]);
    if(!strncmp(argv[1], "status", 6)) {
      puts("inside if");
      status(sockfd, argv[4], &submissionStatus);
    } else if (!strncmp(argv[1], "new", 3)) {
      new(sockfd, argv[4]);
    } else {
      error("Invalid Request Type\n");
    }
     
    
    //response time calculation
    gettimeofday(&respo_end,NULL);
    double x =((respo_end.tv_sec*1000000 + respo_end.tv_usec)-(respo_start.tv_sec*1000000 + respo_start.tv_usec))/1000000.0f;
    total_response_time+=x;
    
    close(sockfd);
    sleep(sleep_secs);
  }
 
  gettimeofday(&loopend,NULL);
  
  total_looptime=((loopend.tv_sec*1000000 + loopend.tv_usec)-(loopstart.tv_sec*1000000 + loopstart.tv_usec))/1000000.0f;
  
  
  printf("Total looptime: %f\n",total_looptime);
  printf("Avg response time: %f\n",total_response_time/loopnum);
  printf("Requests: %lu\n", n_req);
  printf("Success: %lu\n",n_success);
  printf("Error: %lu\n", n_err);
  printf("Timeouts: %lu\n", n_timeouts);
  return 0;
}
 
