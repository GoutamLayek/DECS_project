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
pthread_mutex_t qLock;
pthread_cond_t qNotEmpty;
Queue *jobs;
int closepool;

pthread_mutex_t serviceMutex;
double service_time;

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
    char *output = getName(basename, "_output.txt", 16, 11);
    int n;
    bzero(buffer, 4096);
    struct timeval respo_start, respo_end;
    gettimeofday(&respo_start, NULL);

    n = read(sockfd, buffer, 4095);
    if (n < 0)
    {
        printf("Error reading from socket\n");
    }

    int fd = open(sourceFile, O_RDWR | O_CREAT, 0660);
    n = write(fd, buffer, n);
    close(fd);
    char *compile_cmd = buildCmd(6, "gcc ", sourceFile, " -o ", basename, " 2> ", compilerError);
    // printf("%s\n", compile_cmd);
    int compile_status = system(compile_cmd);
    if (compile_status == 0)
    {
        char *exec_cmd = buildCmd(6, "./", basename, " > ", output, " 2> ", runtimeError);
        // printf("%s\n", exec_cmd);
        int execute_status = system(exec_cmd);
        if (execute_status == 0)
        {
            bzero(buffer, 4096);
            int fd = open(output, O_RDONLY);
            int n = read(fd, buffer, 4095);
            close(fd);
            n = write(sockfd, buffer, n);
            if (n < 0)
                error("ERROR writing to socket");
        }
        else
        {
            bzero(buffer, 4096);
            int fd = open(runtimeError, O_RDONLY);
            int n = read(fd, buffer, 4095);
            close(fd);
            n = write(sockfd, buffer, n);
            if (n < 0)
                error("ERROR writing to socket");
        }
        deleteFiles(1, exec_cmd);
    }
    else
    {
        bzero(buffer, 4096);
        int fd = open(compilerError, O_RDONLY);
        int n = read(fd, buffer, 4095);
        close(fd);
        n = write(sockfd, buffer, n);
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
char *get_threads() {
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
char *calculate_cpu_usage() {
    //FILE *cp = popen("top -bn 1 | grep '%Cpu' | awk '{print $2}'| xargs echo -n;", "r");
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
    FILE* fd =  fopen("logs.txt", "w+");
    char header[] = "cpu,threads,queue,service_time\n";
    printf("here\n");
    fwrite(header, sizeof(char), sizeof(header), fd);
    while (1)
    {
        if(closepool) {
            break;
        }
        if(system("top -bn 1 | grep '%Cpu' | awk '{print $2}' >> cpu_usage.txt")) {
            puts("Error: top");
        }
        char toExecute[200], cmd[] = "cat /proc/%d/stat | awk '{ print $20 }' >> active_threads.txt";
        sprintf(toExecute, cmd, getpid());
        if(system(toExecute)) {
            puts("Error: cat");
        }

        // char *cpu = calculate_cpu_usage();
        // char *th = get_threads();
        // // Write the values as a comma-separated string to the file
        // fprintf(fd, "%s,%s,%ld,%f\n", cpu, th, jobs->size, service_time);
        fprintf(fd, "%lu,%f\n", jobs->size, service_time);
        fflush(fd);
        // free(cpu);
        // free(th);
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
