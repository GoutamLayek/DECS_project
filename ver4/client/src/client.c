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
    return 1;
  }
  
  //receive response from server
  bzero(buffer,4096);
  n=read(sockfd,buffer,4095);

  if (n < 0) {
    perror("ERROR reading from socket\n");
      return 3;
  } else {
    printf("Your Request ID is: %s\n",buffer);
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

  if (argc < 4) {
    fprintf(stderr, "Usage:\n%s <new|status> <hostname> <portno> <filename|requestID>\n", argv[0]);
    exit(0);
  }
  
  portno = atoi(argv[3]);

  sockfd = socket(AF_INET, SOCK_STREAM, 0); //create the half socket

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
    
  close(sockfd);
  return 0;
}
 
