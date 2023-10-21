#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <netinet/in.h>

void error(char *msg)
{
	perror(msg);
	exit(1);
}

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno;
	socklen_t clilen;
	char buffer[4096];
	struct sockaddr_in serv_addr, cli_addr;
	int n;

	if(argc < 2)
	{
		fprintf(stderr, "ERROR, no port provided\n");
		exit(1);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if(sockfd < 0)
		error("ERROR opening socket");

	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	portno = atoi(argv[1]);
	serv_addr.sin_port = htons(portno);

	if( bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR on binding");


while(1){

	listen(sockfd, 2);

	clilen = sizeof(cli_addr);

	

        newsockfd=accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
	if(newsockfd < 0)
	error("ERROR on accept");
	

	//fprintf(stdout, "Connected to Client\n");
	
	
	bzero(buffer,4096);
	
        n=read(newsockfd,buffer,4095);
        if(n<0)
         printf("Error reading from socket\n");
        
        int fd=open("received.c",O_WRONLY | O_CREAT);
        n=write(fd,buffer,n);
        close(fd);
        
        //printf("File received sucessfully\n");
        
        int compile_status=system("gcc received.c -o output 2>error.txt");
        if (compile_status ==0)
        {
           
           int execute_status=system("./output >output.txt 2>runtime.txt");
           
           if(execute_status==0)
           {
              bzero(buffer,4096);
              int fd=open("output.txt",O_RDONLY);
              int n=read(fd,buffer,4095);
              close(fd);
              n=write(newsockfd,buffer,4095);
              if(n<0)
               error("ERROR writing to socket");
              
           }
           else
           {
              bzero(buffer,4096);
              int fd=open("runtime.txt",O_RDONLY);
              int n=read(fd,buffer,4095);
              close(fd);
              n=write(newsockfd,buffer,4095);
              if(n<0)
               error("ERROR writing to socket");
           }
           
        }
        else
        {
              bzero(buffer,4096);
              int fd=open("error.txt",O_RDONLY);
              int n=read(fd,buffer,4095);
              close(fd);
              n=write(newsockfd,buffer,4095);
              if(n<0)
               error("ERROR writing to socket");
        }
        
        
        
        
        close(newsockfd);
    }
        close(sockfd);
        
        return 0;
    }		
