#include <stdio.h>

#ifndef SOCKET_UTILS_H

#define SOCKET_UTILS_H

#define BUFFER_SIZE 4096
#define MAX_FILE_SIZE_BYTES 4

int send_message(int sockfd, char *message);
int send_file(int sockfd, char* file_path);
int recv_file(int sockfd, char* file_path);
int print_recv_file(int sockfd, FILE* filePtr, int* submissionStatus);

#endif