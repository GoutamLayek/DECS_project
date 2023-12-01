/**
 * 
 * Credits: Prof. Varsha Apte for the socket utility functions: send_file, recv_file
*/

#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "socket-utils.h"

int send_message(int sockfd, char *message) {
    int n;
    int msgLength = strlen(message);

    puts("\nSending Message");
    printf("Message length=%d\n", msgLength);
    //buffer to send file size to server
    char message_size_bytes[MAX_FILE_SIZE_BYTES];
    //copy the bytes of the file size integer into the char buffer
    memcpy(message_size_bytes, &msgLength, sizeof(msgLength));

    n = write(sockfd, message_size_bytes, sizeof(message_size_bytes));
    if (n < 0) {
        perror("ERROR writing to socket");
        return -1;
    }
    printf("Message size sent\n");
    n = write(sockfd, message, msgLength);
    if (n < 0) {
        perror("ERROR writing to socket");
        return -1;
    }
    printf("Message sent\n\n");
    return 0;
}

//Utility Function to send a file of any size to the grading server
int send_file(int sockfd, char* file_path) {
//Arguments: socket fd, file name (can include path)
    char buffer[BUFFER_SIZE]; //buffer to read  from  file
    bzero(buffer, BUFFER_SIZE); //initialize buffer to all NULLs
    FILE *file = fopen(file_path, "r"); //open the file for reading, get file descriptor 
    if (!file) {
        perror("Error opening file");
        return -1;
    }

	//for finding file size in bytes
    fseek(file, 0L, SEEK_END); 
    int file_size = ftell(file);
    printf("\nSending File\nFile size=%d\n", file_size);
    
    //Reset file descriptor to beginning of file
    fseek(file, 0L, SEEK_SET);
		
    //buffer to send file size to server
    char file_size_bytes[MAX_FILE_SIZE_BYTES];
    //copy the bytes of the file size integer into the char buffer
    memcpy(file_size_bytes, &file_size, sizeof(file_size));
    
    //send file size to server, return -1 if error
    if (send(sockfd, &file_size_bytes, sizeof(file_size_bytes), 0) == -1) {
        perror("Error sending file size");
        fclose(file);
        return -1;
    }
    puts("Sent File Size");
	//now send the source code file 
    while (!feof(file)) { //while not reached end of file
    
    		//read buffer from file
        size_t bytes_read = fread(buffer, 1, BUFFER_SIZE -1, file);
        
     		//send to server
        if (send(sockfd, buffer, bytes_read+1, 0) == -1) {
            perror("Error sending file data");
            fclose(file);
            return -1;
        }
        
        //clean out buffer before reading into it again
        bzero(buffer, BUFFER_SIZE);
    }
    puts("Sent File\n\n");
    //close file
    fclose(file);
    return 0;
}

//Utility Function to receive a file of any size to the grading server
int recv_file(int sockfd, char* file_path)
//Arguments: socket fd, file name (can include path) into which we will store the received file
{
    char buffer[BUFFER_SIZE]; //buffer into which we read  the received file chars
    bzero(buffer, BUFFER_SIZE); //initialize buffer to all NULLs
    FILE *file = fopen(file_path, "w");  //Get a file descriptor for writing received data into file
    if (!file)
    {
        perror("Error opening file");
        return -1;
    }

    puts("\nReceiving File\n");
	
	//buffer for getting file size as bytes
    char file_size_bytes[MAX_FILE_SIZE_BYTES];
    //first receive  file size bytes
    if (recv(sockfd, file_size_bytes, sizeof(file_size_bytes), 0) == -1)
    {
        perror("Error receiving file size");
        fclose(file);
        return -1;
    }
   
    int file_size;
    //copy bytes received into the file size integer variable
    memcpy(&file_size, file_size_bytes, sizeof(file_size_bytes));
    
    //some local printing for debugging
    printf("File Size=%d\n", file_size);
    
    //now start receiving file data
    size_t bytes_read = 0, total_bytes_read =0;
    while (true)
    {
    	  //read max BUFFER_SIZE amount of file data
        bytes_read = recv(sockfd, buffer, BUFFER_SIZE, 0);

        //total number of bytes read so far
        total_bytes_read += bytes_read;
        if (bytes_read <= 0)
        {
            perror("Error receiving file data");
            fclose(file);
            return -1;
        }

		//write the buffer to the file
        fwrite(buffer, 1, bytes_read, file);

	// reset buffer
        bzero(buffer, BUFFER_SIZE);
        
       //break out of the reading loop if read file_size number of bytes
        if (total_bytes_read >= file_size)
            break;
    }
    fclose(file);
    puts("Received File\n\n");
    return 0;
}

// Utility Function to receive a file of any size to the grading server
// File is not closed here
int print_recv_file(int sockfd, FILE *file, int *submissionStatus) {
    if (!file) {
        perror("Error opening file");
        return -1;
    }

    char buffer[BUFFER_SIZE]; //buffer into which we read  the received file chars
    bzero(buffer, BUFFER_SIZE); //initialize buffer to all NULLs
    char message_size_bytes[MAX_FILE_SIZE_BYTES];

    if (recv(sockfd, message_size_bytes, sizeof(message_size_bytes), 0) == -1) {
        perror("Error receiving file size");
        return -1;
    }
    puts("\nReceiving File\n");
    int message_size;
    memcpy(&message_size, message_size_bytes, sizeof(message_size_bytes));

    printf("Message Size=%d\n", message_size);

    int n = recv(sockfd, buffer, message_size, 0);
    char isFileToBeSent = buffer[0];
    fwrite(buffer+1, 1, n-1, file);

    bzero(buffer, BUFFER_SIZE);
    
    *submissionStatus = isFileToBeSent;
    if(isFileToBeSent == '\001') return 0;
    if(isFileToBeSent != '\002') return -1;
    // STARTING THE FILE OPERATIONS

    char file_size_bytes[MAX_FILE_SIZE_BYTES];
	//buffer for getting file size as bytes
    //first receive  file size bytes
    if (recv(sockfd, file_size_bytes, sizeof(file_size_bytes), 0) == -1) {
        perror("Error receiving file size");
        return -1;
    }

    int file_size;
    //copy bytes received into the file size integer variable
    memcpy(&file_size, file_size_bytes, sizeof(file_size_bytes));
    
    //some local printing for debugging
    // printf("File size=%d\n", file_size);
    
    //now start receiving file data
    size_t bytes_read = 0, total_bytes_read =0;;
    while (true)
    {
    	  //read max BUFFER_SIZE amount of file data
        bytes_read = recv(sockfd, buffer, BUFFER_SIZE, 0);

        //total number of bytes read so far
        total_bytes_read += bytes_read;

        if (bytes_read <= 0)
        {
            perror("Error receiving file data");
            return -1;
        }

		//write the buffer to the file
        fwrite(buffer, 1, bytes_read, file);

	// reset buffer
        bzero(buffer, BUFFER_SIZE);
        
       //break out of the reading loop if read file_size number of bytes
        if (total_bytes_read >= file_size)
            break;
    }
    // printf("File End\n\n");
    return 0;
}