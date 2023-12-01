#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sqlite3.h>

#include "project_file_paths.h"
#include "socket-utils.h"
#include "grader-utils.h"
#include "database.h"
#include "persistent_queue.h"

char baseDir[1024];
char logsDir[] = "../logs";
char databaseLogFile[] = "database.log";
char publicDir[] = "../public";
char submissionsDir[] = "submissions";
char resultsDir[] = "results";
char sourceCodeFile[] = "program.c";
char compilerErrorFile[] = "compiler_error.txt";
char runtimeErrorFile[] = "runtime_error.txt";
char outputDiffFile[] = "output_diff.txt";
char programOutputFile[] = "program_output.txt";
char executableFile[] = "program";
char expectedOutputFile[] = "../tests/expected_output.txt";
char finalOutputFile[] = "final_output.txt";
char databaseFile[] = "../autograder.db";

pthread_mutex_t qLock;
pthread_cond_t qNotEmpty;
Queue *jobs;
int closepool;

void error(char *msg) {
    perror(msg);
    exit(1);
}

void evaluate_request(char *requestID) {

    puts("\n\nEVALUATE REQUEST");
    printf("Request ID: %s\n", requestID);

    update_data(requestID, "in-process", 0);
    puts("Status Updated to in-process");

    if (create_results_folder_n_files(requestID)) {
        return;
    }

    char *copyCommand = NULL;
    char sourceCodeFilePath[FILE_PATH_MAX_LEN];
    char executableFilePath[FILE_PATH_MAX_LEN];
    char compilerErrorFilePath[FILE_PATH_MAX_LEN];
    char finalOutputFilePath[FILE_PATH_MAX_LEN];

    sprintf(sourceCodeFilePath, "%s/%s/%s/%s/%s", baseDir, publicDir, requestID, submissionsDir, sourceCodeFile);
    sprintf(executableFilePath, "%s/%s/%s/%s/%s", baseDir, publicDir, requestID, submissionsDir, executableFile);
    sprintf(compilerErrorFilePath, "%s/%s/%s/%s/%s", baseDir, publicDir, requestID, resultsDir, compilerErrorFile);
    sprintf(finalOutputFilePath, "%s/%s/%s/%s/%s", baseDir, publicDir, requestID, resultsDir, finalOutputFile);

    // Compile and execute the file
    char *compileCommand = build_command(6, "gcc ", sourceCodeFilePath, " -o ", executableFilePath, " 2> ", compilerErrorFilePath);
    // printf("%s\n", compileCommand);

    int compilationStatus = system(compileCommand);
    if (compilationStatus == 0) {
        puts("Compilation Successful");
        char runtimeErrorFilePath[FILE_PATH_MAX_LEN];
        char programOutputFilePath[FILE_PATH_MAX_LEN];

        sprintf(runtimeErrorFilePath, "%s/%s/%s/%s/%s", baseDir, publicDir, requestID, resultsDir, runtimeErrorFile);
        sprintf(programOutputFilePath, "%s/%s/%s/%s/%s", baseDir, publicDir, requestID, resultsDir, programOutputFile);

        char *executeCommand = build_command(5, executableFilePath, " > ", programOutputFilePath, " 2> ", runtimeErrorFilePath);

        int executionStatus = system(executeCommand);
        if (executionStatus == 0) {
            puts("Execution Successful");
            char expectedOutputFilePath[FILE_PATH_MAX_LEN];
            char outputDiffFilePath[FILE_PATH_MAX_LEN];

            sprintf(expectedOutputFilePath, "%s/%s/%s", baseDir, publicDir, expectedOutputFile);
            sprintf(outputDiffFilePath, "%s/%s/%s/%s/%s", baseDir, publicDir, requestID, resultsDir, outputDiffFile);

            char *outputDiffCommand = build_command(6, "diff ", expectedOutputFilePath, " ", programOutputFilePath, " > ", outputDiffFilePath);

            int diffStatus = system(outputDiffCommand);
            if (diffStatus == 0) {
                puts("Diff: No mismatch");
                copyCommand = build_command(2, "echo PASS > ", finalOutputFilePath);
            } else {
                puts("Diff: Mismatch");
                copyCommand = build_command(4, "echo OUTPUT ERROR | cat - ", outputDiffFilePath, " > ", finalOutputFilePath);
            }
        } else {
            puts("Execution Failed");
            copyCommand = build_command(4, "echo RUNTIME ERROR | cat - ", runtimeErrorFilePath, " > ", finalOutputFilePath);
        }
    } else {
        puts("Compilation Failed");
        copyCommand = build_command(4, "echo COMPILER ERROR | cat - ", compilerErrorFilePath, " > ", finalOutputFilePath);
    }
    update_data(requestID, "done", 0);
    puts("Status Updated to done");
    while (system(copyCommand) != 0);
    return;
}

void handle_new_request(int sockfd, char *requestID) {
    int dbStatus = 0, n = 0;
    char sourceCodeFilePath[FILE_PATH_MAX_LEN];

    generate_UUID(requestID);
    printf("\n\nNEW REQUEST\nRequest ID generated: %s\n", requestID);

    if (create_submission_folder(requestID)) {
        close(sockfd);
        return;
    }

    sprintf(sourceCodeFilePath, "%s/%s/%s/%s/%s", baseDir, publicDir, requestID, submissionsDir, sourceCodeFile);
    n = recv_file(sockfd, sourceCodeFilePath);
    if (n < 0) {
        printf("Error reading from socket\n");
        close(sockfd);
        return;
    }

    // Add the request to the database
    dbStatus = insert_data(requestID);
    if (dbStatus != SQLITE_OK) {
        printf("Error inserting data into database\n");
        close(sockfd);
        return;
    }
    printf("Request ID added to database\n");

    // What if this message is not sent?
    n = write(sockfd, requestID, 37);
    if (n < 0) {
        printf("Error reading from socket\n");
        close(sockfd);
        return;
    }
    close(sockfd);
    printf("Request ID sent\n\n");

    QueueData *data = (QueueData *)malloc(sizeof(QueueData));
    data->sockfd = -1;
    strcpy(data->requestID, requestID);
    pthread_mutex_lock(&qLock);
    enqueue(jobs, (void *)data);
    pthread_mutex_unlock(&qLock);
    printf("EVALUATE Request for %s added to queue\n\n", requestID);
}

void handle_status_request(int sockfd, char *requestID) {
    char message[200];
    int n = 0, dbStatus = 0, queuePosition = 0;

    puts("\n\nSTATUS REQUEST");

    n = read(sockfd, requestID, 37);
    if (n < 0) {
        printf("Error reading from socket\n");
        close(sockfd);
        return;
    }
    printf("Request ID received: %s\n", requestID);

    FetchResult result;
    dbStatus = fetch_data(requestID, &result);

    if(dbStatus == SQLITE_EMPTY) {
        sprintf(message, "\001Grading request %s not found. Please check and resend your request ID or re-send your original grading request.\n", requestID);
        if(send_message(sockfd, message) == -1) {
            printf("Error writing to socket\n");
            close(sockfd);
            return;
        }
        printf("Sent grading request not found message\n");
    }
    
    if (dbStatus != SQLITE_OK) {
        printf("Error fetching data from database\n");
        close(sockfd);
        return;
    }

    // Request is processed
    if (!strcmp(result.evaluation_status, "done")) {
        sprintf(message, "\002Your grading request ID %s processing is done, here are the results:\n", requestID);
        if(send_message(sockfd, message) == -1) {
            printf("Error writing to socket\n");
            close(sockfd);
            return;
        }
        puts("Sent evaluation done message");
        char finalOutputFilePath[FILE_PATH_MAX_LEN];
        sprintf(finalOutputFilePath, "%s/%s/%s/%s/%s", baseDir, publicDir, requestID, resultsDir, finalOutputFile);
        n = send_file(sockfd, finalOutputFilePath);
        if (n < 0) {
            printf("Error writing from socket\n");
            close(sockfd);
            return;
        }
        puts("Sent results file");
    }

    if (!strcmp(result.evaluation_status, "in-process")) {
        // Request is processing
        sprintf(message, "\001Your grading request ID %s has been accepted and is currently being processed.\n", requestID);
        if(send_message(sockfd, message) == -1) {
            printf("Error writing to socket\n");
            close(sockfd);
            return;
        }
        puts("Sent evaluation in-process message");
    }

    if(!strcmp(result.evaluation_status, "pending")) {
        // Request is in the queue
        queuePosition = find_position(jobs, requestID);
        if(queuePosition == -1) {
            sprintf(message, "\001Your grading request %s has been accepted. Please try after sometime with this request ID.\n", requestID);
        } else {
            sprintf(message, "\001Your grading request ID %s has been accepted. It is currently at position %d in the queue.\n", requestID, queuePosition);
        }
        n = write(sockfd, message, strlen(message));
        if(send_message(sockfd, message) == -1) {
            printf("Error writing to socket\n");
            close(sockfd);
            return;
        }
        printf("Sent queue position %d message\n", queuePosition);
    }
    close(sockfd);
}

void handle_request(int sockfd) {
    int n = 0;
    char requestType;
    char requestID[37];

    // Extract the Request Type
    n = read(sockfd, &requestType, 1);
    if (n < 0) {
        printf("Error reading from socket\n");
        close(sockfd);
        return;
    }

    if (requestType == 'n') {
        // Request Type: new
        handle_new_request(sockfd, requestID);
    } else if (requestType == 's') {
        // Request Type: status
        handle_status_request(sockfd, requestID);
    } else {
        // Request Type: Invalid
        printf("Invalid request type\n");
    }

    close(sockfd);
    return;
}

void *main_function_of_thread() {
    pthread_detach(pthread_self());
    while (1) {
        pthread_mutex_lock(&qLock);
        while (!closepool && is_empty(jobs)) {
            pthread_cond_wait(&qNotEmpty, &qLock);
        }
        if (closepool) {
            pthread_mutex_unlock(&qLock);
            pthread_exit(NULL);
        }
        QueueData* job = (QueueData*) dequeue(jobs);

        pthread_mutex_unlock(&qLock);
        // If requestID is not empty, then we have a request to process
        if (strlen(job->requestID) == 36 && job->sockfd == -1) {
            evaluate_request(job->requestID);
        } else if (job->sockfd >= 0) {
            handle_request(job->sockfd);
        }
        free(job);
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (getcwd(baseDir, FILE_PATH_MAX_LEN) == NULL) {
        perror("Error getting current working directory");
        return 1;
    }
    // Server Resilience
    char databaseFilePath[FILE_PATH_MAX_LEN];
    sprintf(databaseFilePath, "%s/%s", baseDir, databaseFile);
    open_database(databaseFilePath);
    setup_signal_handler();
    create_requests_table();
    jobs = initialize_queue();
    pthread_mutex_init(&qLock, NULL);
    pthread_cond_init(&qNotEmpty, NULL);
    load_queue_from_database(jobs);

    // Server Setup
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    if (argc < 3) {
        fprintf(stderr, "Usage: ./server.c <port> <numOfThreads>");
        // exit(1);
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

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    size_t n_threads = atoi(argv[2]);
    pthread_t *threadPool = (pthread_t *)malloc(n_threads * sizeof(pthread_t));
    for (size_t i = 0; i < n_threads; i++)
    {
        pthread_create(&threadPool[i], NULL, main_function_of_thread, NULL);
    }
    listen(sockfd, 2);
    printf("Listening on port: %d\n", portno);
    while (1)
    {
        clilen = sizeof(cli_addr);

        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        printf("newsockfd: %d\n", newsockfd);
        if (newsockfd < 0)
            error("ERROR on accept");

        // Move locks to some other place
        pthread_mutex_lock(&qLock);
        QueueData *data = (QueueData *)malloc(sizeof(QueueData));
        data->sockfd = newsockfd;
        strcpy(data->requestID, "");
        enqueue(jobs, data);
        pthread_cond_signal(&qNotEmpty);
        pthread_mutex_unlock(&qLock);
    }

    close(sockfd);
    close_database();
    return 0;
}
