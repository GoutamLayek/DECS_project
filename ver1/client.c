#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include<ctype.h>
#include <netdb.h>
#include <fcntl.h>
void error(char *msg) {
  perror(msg);
  exit(0);
}

int main(int argc, char *argv[]) {
  int sockfd, portno, n;

  struct sockaddr_in serv_addr; //Socket address structure
  struct hostent *server; //return type of gethostbyname

  char buffer[4096]; //buffer for message

  if (argc < 2) {
    fprintf(stderr, "usage %s hostname port\n", argv[0]);
    exit(0);
  }
  
  portno = atoi(argv[2]); // 2nd argument of the command is port number

  sockfd = socket(AF_INET, SOCK_STREAM, 0); //create the half socket. 
  
  if (sockfd < 0)
    error("ERROR opening socket");


  server = gethostbyname(argv[1]);

  if (server == NULL) {
    fprintf(stderr, "ERROR, no such host\n");
    exit(0);
  }

  bzero((char *)&serv_addr, sizeof(serv_addr)); // set server address bytes to zero

  serv_addr.sin_family = AF_INET; // Address Family is IP

  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
        server->h_length);

  serv_addr.sin_port = htons(portno);

  

  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR connecting");
    
     bzero(buffer,4096);
     int fd=open("code.c", O_RDONLY);
     n=read(fd,buffer,4095);
     close(fd);
     n=write(sockfd,buffer,4095);
     if(n<0)
        error("ERROR writing to socket");
     printf("File sent sucessfully\n");
     
     
     
     
     bzero(buffer,4096);
     n=read(sockfd,buffer,4095);
     
     if (n < 0)
        error("ERROR reading from socket");
     printf("-->%s\n", buffer);
     
     
     close(sockfd);
     return 0;
     }
 
