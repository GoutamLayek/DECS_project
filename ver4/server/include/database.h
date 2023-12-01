#include <sqlite3.h>
#include <stdbool.h>
#include "persistent_queue.h"

#ifndef HELPER_H
#define HELPER_H

#define DATABASE_NAME "AUTOGRADER"
#define REQUESTS_TABLE_NAME "requests"
#define QUEUE_TABLE_NAME "queue"
#define SQL_LEN 1024

extern char databaseFile[];

typedef struct FetchResult {
    char request_id[37];
    char evaluation_status[11];
    char received_on[20];
    char evaluated_on[20];
    char conveyed_on[20];
} FetchResult;

void log_transaction(const char *tableName, const char *transactionType, const char *status, const char *details);
sqlite3 *open_database(const char *databaseFile);
void close_database();
int create_requests_table();
void generate_UUID(char *uuid_str);
int execute_in_transaction(sqlite3 *db, const char *operationType, const char *sql, const char *tableName);
int insert_data(char *request_uuid);
int fetch_data(const char *request_id, FetchResult *result);
int update_data(const char *request_id, char *new_status, int is_conveyed);
void handle_signal(int signum);
void setup_signal_handler();

int create_queue_table();
int insert_data_enqueue(Queue *queue, const char *request_id, int sockfd);
int delete_data_dequeue(Queue *queue, char *request_id, int *sockfd);
void load_queue_from_database(Queue *queue);

#endif
