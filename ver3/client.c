#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/select.h>
#include <errno.h>
#include <time.h>

void error(char *msg)
{
  perror(msg);
  exit(0);
}

int main(int argc, char *argv[])
{
  int sockfd, portno, n;

  struct sockaddr_in serv_addr; // Socket address structure
  struct hostent *server;       // return type of gethostbyname

  char buffer[4096]; // buffer for message

  if (argc < 7)
  {
    fprintf(stderr, "usage %s hostname portno filename loopnum sleep timeout\n", argv[0]);
    exit(0);
  }

  portno = atoi(argv[2]); // 2nd argument of the command is port number
  int loopnum = atoi(argv[4]);
  int sleep_secs = atoi(argv[5]);

  double total_looptime = 0, total_response_time = 0;

  struct timeval loopstart, loopend;
  gettimeofday(&loopstart, NULL);
  size_t timeout = atoi(argv[6]);
  size_t n_req = 0, n_err = 0, n_timeouts = 0, n_success = 0;
  for (int i = 0; i < loopnum; i++)
  {

    struct timeval respo_start, respo_end;
    sockfd = socket(AF_INET, SOCK_STREAM, 0); // create the half socket

    struct timeval time_out;
    time_out.tv_sec = timeout;
    time_out.tv_usec = 0; // This ensures that only the timeout in seconds (tv_sec) is considered, and there are no microseconds of timeout.
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &time_out, sizeof time_out) == -1)
    { // attempts to set a receive timeout on the socket referred to by sockfd
      perror("Set socket timeout failed");
      close(sockfd);
      exit(1);
    }

    if (sockfd < 0)
      error("ERROR opening socket");

    server = gethostbyname(argv[1]);

    if (server == NULL)
    {
      fprintf(stderr, "ERROR, no such host\n");
      exit(0);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr)); // set server address bytes to zero

    serv_addr.sin_family = AF_INET; // Address Family is IP

    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);

    serv_addr.sin_port = htons(portno);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
      error("ERROR connecting");
    }
    bzero(buffer, 4096);
    int fd = open(argv[3], O_RDONLY);
    n = read(fd, buffer, 4094);
    close(fd);
    // send file to server
    n = write(sockfd, buffer, n);
    if (n < 0)
      error("ERROR writing to socket");
    else
      n_req++;

    // receive response from server
    bzero(buffer, 4096);
    gettimeofday(&respo_start, NULL);
    n = read(sockfd, buffer, 4095);
    // response time calculation
    gettimeofday(&respo_end, NULL);
    double x = ((respo_end.tv_sec * 1000000 + respo_end.tv_usec) - (respo_start.tv_sec * 1000000 + respo_start.tv_usec)) / 1000000.0f;
    total_response_time += x;
    if (n < 0)
    {
      if (errno == EWOULDBLOCK)
      {
        perror("Recv timout:");
        n_timeouts++;
      }
      else if (errno == ECONNRESET)
      {
        perror("ERROR reading from socket\n");
        n_err++;
      }
    }
    else
    {
      n_success++;
      // printf("%s\n",buffer);
    }

    close(sockfd);
    sleep(sleep_secs);
  }

  gettimeofday(&loopend, NULL);

  total_looptime = ((loopend.tv_sec * 1000000 + loopend.tv_usec) - (loopstart.tv_sec * 1000000 + loopstart.tv_usec)) / 1000000.0f;

  printf("Total looptime: %f\n", total_looptime);
  printf("Total response time: %f\n", total_response_time);
  printf("Number of requests: %lu\n", n_req);
  printf("Successful responses: %lu\n", n_success);
  printf("Error: %lu\n", n_err);
  printf("Timeouts: %lu\n", n_timeouts);

  return 0;
}
