#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "queue.h"
#define BUFFER_SIZE 4096
#define MAX_FILE_SIZE_BYTES 4
pthread_mutex_t qLock;
pthread_cond_t qNotEmpty;
Queue *jobs;
int closepool;

pthread_mutex_t serviceMutex;
double service_time;
int recv_file(int sockfd, char *file_path)
// Arguments: socket fd, file name (can include path) into which we will store the received file
{
    char buffer[BUFFER_SIZE];           // buffer into which we read  the received file chars
    bzero(buffer, BUFFER_SIZE);         // initialize buffer to all NULLs
    FILE *file = fopen(file_path, "w"); // Get a file descriptor for writing received data into file
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

void error(char *msg)
{
    perror(msg);
    exit(1);
}
char *getName(const char *base, const char *extension, size_t baselen, size_t extensionlen)
{
    char *fullname = (char *)malloc(sizeof(char) * (baselen + extensionlen));
    strncpy(fullname, base, baselen);
    strncat(fullname, extension, extensionlen);
    return fullname;
}
void deleteFiles(int numStrings, ...)
{
    va_list args;
    va_start(args, numStrings);
    for (int i = 0; i < numStrings; i++)
    {
        char *str = va_arg(args, char *);
        remove(str);
        free(str);
    }
}
char *buildCmd(int numStrings, ...)
{
    va_list args;
    va_start(args, numStrings);
    size_t total_len = 0;
    // calculate size of final string
    for (int i = 0; i < numStrings; i++)
    {
        char *str = va_arg(args, char *);
        total_len += strlen(str);
    }
    total_len++;
    char *cmd = (char *)malloc(sizeof(char) * total_len);
    if (cmd == NULL)
    {
        va_end(args);
        return NULL;
    }
    cmd[0] = '\0';
    va_start(args, numStrings);
    for (int i = 0; i < numStrings; i++)
    {
        char *str = va_arg(args, char *);
        strcat(cmd, str);
    }
    va_end(args);
    return cmd;
}
double handle_client(int arg)
{
    pthread_detach(pthread_self());
    int sockfd = arg;
    char buffer[4096];
    char basename[16];
    sprintf(basename, "%lu", pthread_self());
    // printf("%s\n", basename);
    char *sourceFile = getName(basename, ".c", 16, 2);
    char *compilerError = getName(basename, "_cerror.txt", 16, 11);
    char *runtimeError = getName(basename, "_rerror.txt", 16, 11);
    char *diffError = getName(basename, "_diff.txt", 16, 9);
    char *output = getName(basename, "_output.txt", 16, 11);
    char *solution = "solution";
    int n;
    struct timeval respo_start, respo_end;
    recv_file(sockfd, sourceFile);
    gettimeofday(&respo_start, NULL);
    char *compile_cmd = buildCmd(6, "gcc ", sourceFile, " -o ", basename, " 2> ", compilerError);
    int compile_status = system(compile_cmd);
    if (compile_status == 0)
    {
        char *exec_cmd = buildCmd(6, "./", basename, " > ", output, " 2> ", runtimeError);
        int execute_status = system(exec_cmd);
        if (execute_status == 0)
        {
            char *diff_cmd = buildCmd(6, "diff ", output, "  ", solution, " 2> ", diffError);
            int diff_status = system(diff_cmd);
            if (diff_status == 0)
            {
                char *msg = "PASS";
                // send msg
                n = write(sockfd, msg, strlen(msg));
                send_file(sockfd, output);
                if (n < 0)
                    error("ERROR writing to socket");
            }
            else
            {
                char *msg = "OUTPUT ERROR";
                // send msg
                n = write(sockfd, msg, strlen(msg));
                send_file(sockfd, diffError);
                if (n < 0)
                    error("ERROR writing to socket");
            }
        }
        else
        {
            char *msg = "RUNTIME ERROR";
            // send msg
             n = write(sockfd, msg, strlen(msg));
            send_file(sockfd, runtimeError);

            if (n < 0)
                error("ERROR writing to socket");
        }
        deleteFiles(1, exec_cmd);
    }
    else
    {
        char *msg = "COMPILE ERROR";
        // send msg
          n = write(sockfd, msg, strlen(msg));
        send_file(sockfd, compilerError);
        if (n < 0)
            error("ERROR writing to socket");
    }
    // service time calculation
    gettimeofday(&respo_end, NULL);
    double x = ((respo_end.tv_sec * 1000000 + respo_end.tv_usec) - (respo_start.tv_sec * 1000000 + respo_start.tv_usec)) / 1000000.0f;

    deleteFiles(5, sourceFile, compilerError, runtimeError, output, compile_cmd);
    remove(basename);
    close(sockfd);
    return x;
}
void *threadFunction(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&qLock);
        while (!closepool && isEmpty(jobs))
        {
            pthread_cond_wait(&qNotEmpty, &qLock);
        }
        if (closepool)
        {
            pthread_mutex_unlock(&qLock);
            pthread_exit(NULL);
        }
        int sockfd = dequeue(jobs);
        pthread_mutex_unlock(&qLock);
        if (sockfd != -1)
        {
            double serv = handle_client(sockfd);
            pthread_mutex_lock(&serviceMutex);
            service_time = serv;
            pthread_mutex_unlock(&serviceMutex);
        }
    }
    pthread_exit(NULL);
}
char *get_threads()
{
    // FILE *ps = popen("ps -T --no-headers -p $(pgrep 'server' | xargs echo ;) | wc -l", "r");
    FILE *ps = popen("cat /proc/self/stat | awk '{ print $20 }' |xargs echo -n", "r");
    char *buffer;
    if (ps)
    {
        buffer = (char *)malloc(10);
        if (fgets(buffer, sizeof(buffer), ps) != NULL)
        {
            pclose(ps);
            return buffer;
        }
    }
    pclose(ps);
    return NULL;
}
// Function to calculate CPU usage percentage
char *calculate_cpu_usage()
{

    FILE *cp = popen("top -bn 1 | grep '%Cpu' | awk '{print $2}'", "r");

    char *buffer;
    if (cp)
    {
        buffer = (char *)malloc(10);
        if (fgets(buffer, sizeof(buffer), cp) != NULL)
        {
            pclose(cp);
            return buffer;
        }
    }
    pclose(cp);
    return NULL;
}

void *controlFunction(void *args)
{
    // Creates or opens log file
    FILE *fd = fopen("logs.txt", "w+");
    char header[] = "cpu,threads,queue,service_time\n";
    printf("here\n");
    fwrite(header, sizeof(char), sizeof(header), fd);
    while (1)
    {
        if (closepool)
        {
            break;
        }
        if (system("top -bn 1 | grep '%Cpu' | awk '{print $2}' >> cpu_usage.txt"))
        {
            puts("Error: top");
        }
        char toExecute[200], cmd[] = "cat /proc/%d/stat | awk '{ print $20 }' >> active_threads.txt";
        sprintf(toExecute, cmd, getpid());
        if (system(toExecute))
        {
            puts("Error: cat");
        }
        fprintf(fd, "%lu,%f\n", jobs->size, service_time);
        fflush(fd);
        sleep(0.5);
    }
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    if (argc != 3)
    {
        fprintf(stderr, "Usage: server.c <port> <num of threads>");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
        error("ERROR opening socket");

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    portno = atoi(argv[1]);
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    size_t n_threads = atoi(argv[2]);
    jobs = initializeQueue();
    pthread_mutex_init(&qLock, NULL);
    pthread_mutex_init(&serviceMutex, NULL);
    pthread_cond_init(&qNotEmpty, NULL);
    pthread_t *threadPool = (pthread_t *)malloc(n_threads * sizeof(pthread_t));

    pthread_t controlThread;
    for (size_t i = 0; i < n_threads; i++)
    {
        pthread_create(&threadPool[i], NULL, threadFunction, NULL);
    }
    pthread_create(&controlThread, NULL, controlFunction, NULL);
    listen(sockfd, 2);
    while (1)
    {
        clilen = sizeof(cli_addr);

        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");

        pthread_mutex_lock(&qLock);
        enqueue(jobs, newsockfd);
        pthread_cond_signal(&qNotEmpty);
        pthread_mutex_unlock(&qLock);
    }

    close(sockfd);
    return 0;
}
