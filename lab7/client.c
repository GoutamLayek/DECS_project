#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include<ctype.h>
#include <netdb.h>
#include <fcntl.h>
#include<sys/time.h>

void error(char *msg) {
  perror(msg);
  exit(0);
}

int main(int argc, char *argv[]) {
  int sockfd, portno, n;

  struct sockaddr_in serv_addr; //Socket address structure
  struct hostent *server; //return type of gethostbyname

  char buffer[4096]; //buffer for message

  if (argc < 6) {
    fprintf(stderr, "usage %s hostname port filename loopnum sleep\n", argv[0]);
    exit(0);
  }
  
  portno = atoi(argv[2]); // 2nd argument of the command is port number
  int loopnum=atoi(argv[4]);
  int sleep_time=atoi(argv[5]);
  
  
  struct timeval loopstart, loopend;
  gettimeofday(&loopstart,NULL);
  
  double total_response_time=0;
  int success = 0;
  
  for(int i=0;i<loopnum;i++)
  {
  
  struct timeval response_start, response_end;
  gettimeofday(&response_start,NULL);
  
  
  
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
     int fd=open(argv[3], O_RDONLY);
     n=read(fd,buffer,4095);
     close(fd);
     n=write(sockfd,buffer,4095);
     if(n<0)
        error("ERROR writing to socket");
     //printf("File sent sucessfully\n");
     
     
     
     
     bzero(buffer,4096);
     n=read(sockfd,buffer,4095);
     
     if (n < 0)
        error("ERROR reading from socket");
     else
     	success++;
     printf("-->%s\n", buffer);
     
     
     gettimeofday(&response_end,NULL);
     
     double x=((response_end.tv_sec *1000000 + response_end.tv_usec)-(response_start.tv_sec *1000000 + response_start.tv_usec))/1000000.0f;
     
     total_response_time+=x;
     
     
     
     
     close(sockfd);
     
     sleep(sleep_time);
     
     
     }
     
  gettimeofday(&loopend,NULL);
  
   printf("Loop_time: %f second\n",((loopend.tv_sec *1000000 + loopend.tv_usec)-(loopstart.tv_sec *1000000 + loopstart.tv_usec))/1000000.0f);
   printf("Average response time: %f second\n",total_response_time/loopnum);
   printf("Number of successful responses: %d\n", success);
  
  
  
  
     
     return 0;
     }
 
