#include <sys/types.h>

#ifndef GRADER_UTILS_H
#define GRADER_UTILS_H

char* get_name(const char* base, const char* extension, size_t baselen, size_t extensionlen);
void delete_files(int numStrings, ...);
char* build_command(int numStrings, ...);
int create_submission_folder(const char *folderName);
int create_results_folder_n_files(const char *folderName);

#endif