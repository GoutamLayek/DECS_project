#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "project_file_paths.h"
#include "grader-utils.h"

char *get_name(const char *base, const char *extension, size_t baselen, size_t extensionlen)
{
    char *fullname = (char *)malloc(sizeof(char) * (baselen + extensionlen));
    strncpy(fullname, base, baselen);
    strncat(fullname, extension, extensionlen);
    return fullname;
}

void delete_files(int numStrings, ...)
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

char *build_command(int numStrings, ...)
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

int create_submission_folder(const char *folderName) {
    char requestIDDirPath[FILE_PATH_MAX_LEN];
    char submissionDirPath[FILE_PATH_MAX_LEN];
    char sourceCodeFilePath[FILE_PATH_MAX_LEN];

    sprintf(requestIDDirPath, "%s/%s/%s", baseDir, publicDir, folderName);
    sprintf(submissionDirPath, "%s/%s/%s/%s", baseDir, publicDir, folderName, submissionsDir);
    sprintf(sourceCodeFilePath, "%s/%s/%s/%s/%s", baseDir, publicDir, folderName, submissionsDir, sourceCodeFile);

    if (mkdir(requestIDDirPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
        perror("Error creating  folder");
        return 1;
    }
    printf("Created request ID folder: %s\n", requestIDDirPath);

    if (mkdir(submissionDirPath, 0777) == -1) {
        perror("Error creating submission folder");
        return 1;
    }
    printf("Created submission folder: %s\n", submissionDirPath);

    int programCodeFd = creat(sourceCodeFilePath, 0666);
    if (programCodeFd == -1) {
        perror("Error creating files");
        return 2;
    }
    return 0;
}

int create_results_folder_n_files(const char *folderName)
{
    // Create the 'results' folder
    char resultsDirPath[FILE_PATH_MAX_LEN];
    char compilerErrorFilePath[FILE_PATH_MAX_LEN];
    char runtimeErrorFilePath[FILE_PATH_MAX_LEN];
    char programOutputFilePath[FILE_PATH_MAX_LEN];
    char outputDiffFilePath[FILE_PATH_MAX_LEN];
    char finalOutputFilePath[FILE_PATH_MAX_LEN];
    sprintf(resultsDirPath, "%s/%s/%s/%s", baseDir, publicDir, folderName, resultsDir);

    if (mkdir(resultsDirPath, 0777) == -1)
    {
        perror("Error creating results folder");
        return 1;
    }
    printf("Created results folder: %s\n", resultsDirPath);

    // Create files within the 'results' folder using creat
    sprintf(compilerErrorFilePath, "%s/%s/%s/%s/%s", baseDir, publicDir, folderName, resultsDir, compilerErrorFile);
    sprintf(runtimeErrorFilePath, "%s/%s/%s/%s/%s", baseDir, publicDir, folderName, resultsDir, runtimeErrorFile);
    sprintf(programOutputFilePath, "%s/%s/%s/%s/%s", baseDir, publicDir, folderName, resultsDir, programOutputFile);
    sprintf(outputDiffFilePath, "%s/%s/%s/%s/%s", baseDir, publicDir, folderName, resultsDir, outputDiffFile);
    sprintf(finalOutputFilePath, "%s/%s/%s/%s/%s", baseDir, publicDir, folderName, resultsDir, finalOutputFile);

    int compilerErrFd = creat(compilerErrorFilePath, 0666);
    int runtimeErrFd = creat(runtimeErrorFilePath, 0666);
    int diffOutputFd = creat(outputDiffFilePath, 0666);
    int programOutputFd = creat(programOutputFilePath, 0666);
    int finalOutputFd = creat(finalOutputFilePath, 0666);

    if (compilerErrFd == -1 || runtimeErrFd == -1 || diffOutputFd == -1 || programOutputFd == -1 || finalOutputFd == -1)
    {
        perror("Error creating result files");
        return 2;
    }

    printf("Created empty result files\n");

    // Close the file descriptors
    close(compilerErrFd);
    close(runtimeErrFd);
    close(diffOutputFd);
    close(programOutputFd);
    close(finalOutputFd);
    return 0;
}