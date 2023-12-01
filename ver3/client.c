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
#include <sys/stat.h>
void error(char *msg)
{
  perror(msg);
  exit(0);
}
#define BUFFER_SIZE 4096
#define MAX_FILE_SIZE_BYTES 4
// Utility Function to receive a file of any size to the grading server
int recv_file(int sockfd, FILE *file)
// Arguments: socket fd, file name (can include path) into which we will store the received file
{
  char buffer[BUFFER_SIZE];   // buffer into which we read  the received file chars
  bzero(buffer, BUFFER_SIZE); // initialize buffer to all NULLs
  if (!file)
  {
    perror("Error opening file");
    return -1;
  }

  puts("\nReceiving File\n");

  // buffer for getting file size as bytes
  char file_size_bytes[MAX_FILE_SIZE_BYTES];
  // first receive  file size bytes
  if (recv(sockfd, file_size_bytes, sizeof(file_size_bytes), 0) == -1)
  {
    perror("Error receiving file size");
    fclose(file);
    return -1;
  }

  int file_size;
  // copy bytes received into the file size integer variable
  memcpy(&file_size, file_size_bytes, sizeof(file_size_bytes));

  // some local printing for debugging
  printf("File Size=%d\n", file_size);

  // now start receiving file data
  size_t bytes_read = 0, total_bytes_read = 0;
  while (1)
  {
    // read max BUFFER_SIZE amount of file data
    bytes_read = recv(sockfd, buffer, BUFFER_SIZE, 0);

    // total number of bytes read so far
    total_bytes_read += bytes_read;
    if (bytes_read <= 0)
    {
      perror("Error receiving file data");
      fclose(file);
      return -1;
    }

    // write the buffer to the file
    fwrite(buffer, 1, bytes_read, file);

    // reset buffer
    bzero(buffer, BUFFER_SIZE);

    // break out of the reading loop if read file_size number of bytes
    if (total_bytes_read >= file_size)
      break;
  }
  fclose(file);
  puts("Received File\n\n");
  return 0;
}
// Utility Function to send a file of any size to the grading server
int send_file(int sockfd, char *file_path)
{
  // Arguments: socket fd, file name (can include path)
  char buffer[BUFFER_SIZE];           // buffer to read  from  file
  bzero(buffer, BUFFER_SIZE);         // initialize buffer to all NULLs
  FILE *file = fopen(file_path, "r"); // open the file for reading, get file descriptor
  if (!file)
  {
    perror("Error opening file");
    return -1;
  }

  // for finding file size in bytes
  fseek(file, 0L, SEEK_END);
  int file_size = ftell(file);
  printf("\nSending File\nFile size=%d\n", file_size);

  // Reset file descriptor to beginning of file
  fseek(file, 0L, SEEK_SET);

  // buffer to send file size to server
  char file_size_bytes[MAX_FILE_SIZE_BYTES];
  // copy the bytes of the file size integer into the char buffer
  memcpy(file_size_bytes, &file_size, sizeof(file_size));

  // send file size to server, return -1 if error
  if (send(sockfd, &file_size_bytes, sizeof(file_size_bytes), 0) == -1)
  {
    perror("Error sending file size");
    fclose(file);
    return -1;
  }
  puts("Sent File Size");
  // now send the source code file
  while (!feof(file))
  { // while not reached end of file

    // read buffer from file
    size_t bytes_read = fread(buffer, 1, BUFFER_SIZE - 1, file);

    // send to server
    if (send(sockfd, buffer, bytes_read + 1, 0) == -1)
    {
      perror("Error sending file data");
      fclose(file);
      return -1;
    }

    // clean out buffer before reading into it again
    bzero(buffer, BUFFER_SIZE);
  }
  puts("Sent File\n\n");
  // close file
  fclose(file);
  return 0;
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

    // send file to server
    send_file(sockfd, argv[3]);
    n_req++;

    // receive response from server
    gettimeofday(&respo_start, NULL);
    recv_file(sockfd, stdout);
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
