#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/stat.h>

void error(char *msg) {
    perror(msg);
    exit(1);
}
char* getName(const char* base, const char* extension, size_t baselen, size_t extensionlen){
    char* fullname = (char*)malloc(sizeof(char)*(baselen+extensionlen));
    strncpy(fullname, base, baselen);
    strncat(fullname, extension, extensionlen);
    return fullname;
}
void deleteFiles(int numStrings, ...){
    va_list args;
    va_start(args, numStrings);
    for(int i = 0; i < numStrings; i++){
        char* str = va_arg(args, char*);
        remove(str);
        free(str);
    }

}
char* buildCmd(int numStrings, ...){
    va_list args;
    va_start(args, numStrings);
    size_t total_len = 0;
    //calculate size of final string
    for(int i = 0; i < numStrings; i++){
        char* str = va_arg(args, char*);
        total_len += strlen(str);
    }
    total_len++;
    char* cmd = (char*)malloc(sizeof(char)*total_len);
    if(cmd == NULL){
        va_end(args);
        return NULL;
    }
    cmd[0] = '\0';
    va_start(args, numStrings);
    for(int i = 0; i < numStrings;i++){
        char* str = va_arg(args, char*);
        strcat(cmd, str);
    }
    va_end(args);
    return cmd;
}
void* handle_client(void *arg){
    pthread_detach(pthread_self());  //A detached thread is one that will clean up its resources automatically when it exits, 
                                      //without the need for other threads to call pthread_join() to collect its exit status.
    int sockfd = (int)arg;
    char buffer[4096];
    char basename[16];
    sprintf(basename, "%lu", pthread_self());  //sprintf function to format and store the thread ID of the current thread as a string in the character array basename
    //printf("%s\n", basename);
    char* sourceFile = getName(basename, ".c", 16, 2);
    char* compilerError = getName(basename, "_cerror.txt", 16, 11);
    char* runtimeError = getName(basename, "_rerror.txt", 16, 11);
    char* output = getName(basename, "_output.txt", 16, 11);
    int n;
    bzero(buffer, 4096);
    n = read(sockfd, buffer, 4095);
    if (n < 0) {
        printf("Error reading from socket\n");
    }
    
    int fd = open(sourceFile, O_RDWR  | O_CREAT, 0660);
    n = write(fd, buffer, n);
    close(fd);
    char* compile_cmd = buildCmd(6, "gcc ", sourceFile, " -o ", basename, " 2> ", compilerError);
    //printf("%s\n", compile_cmd);
    int compile_status = system(compile_cmd);
    if (compile_status == 0) {
        char* exec_cmd = buildCmd(6, "./", basename, " > ", output, " 2> ", runtimeError);
        //printf("%s\n", exec_cmd);
        int execute_status = system(exec_cmd);
        if (execute_status == 0) {
            bzero(buffer, 4096);
            int fd = open(output, O_RDONLY);
            int n = read(fd, buffer,4095);
            close(fd);
            n = write(sockfd, buffer,n);
            if (n < 0)
                error("ERROR writing to socket");
        } else {
            bzero(buffer, 4096);
            int fd = open(runtimeError, O_RDONLY);
            int n = read(fd, buffer, 4095);
            close(fd);
            n = write(sockfd, buffer,n);
            if (n < 0)
                error("ERROR writing to socket");
        }
        deleteFiles(1, exec_cmd);
    } else {
        bzero(buffer, 4096);
        int fd = open(compilerError, O_RDONLY);
        int n = read(fd, buffer, 4095);
        close(fd);
        n = write(sockfd, buffer,n);
        if (n < 0)
            error("ERROR writing to socket");
    }

   
    deleteFiles(6, sourceFile, compilerError, runtimeError, output, compile_cmd);
    remove(basename);
    close(sockfd);
  //  close(sockfd);
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
   
 

    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        //exit(1);
        argv[1] = "9000";
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
        error("ERROR opening socket");

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    portno = atoi(argv[1]);
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr) )< 0)
        error("ERROR on binding");

while(1)
{

    listen(sockfd, 2);

    clilen = sizeof(cli_addr);

    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0)
            error("ERROR on accept");

    
    pthread_t thread;
    if(pthread_create(&thread, NULL, handle_client, (void*)newsockfd)){
        close(newsockfd);
    }
    
}
    
    close(sockfd);
    return 0;
}

