#include "project_file_paths.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <sqlite3.h>
#include <uuid/uuid.h>
#include <unistd.h>

#include "project_file_paths.h"
#include "database.h"
#include "persistent_queue.h"

#define UUID_LEN 37

extern pthread_mutex_t qLock;

//static for signal handling
static sqlite3 *db;

void log_transaction(const char *tableName, const char *transactionType, const char *status, const char *details) {
    time_t t;
    time(&t);
    struct tm *tm_info = localtime(&t);

    char databaseLogFilePath[FILE_PATH_MAX_LEN];
    sprintf(databaseLogFilePath, "%s/%s/%s", baseDir, logsDir, databaseLogFile);
    FILE *logFile = fopen(databaseLogFilePath, "a");
    if (logFile != NULL) {
        fprintf(logFile, "%04d-%02d-%02d %02d:%02d:%02d %s %s %s - %s\n",
                tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
                tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
                tableName, transactionType, status, details);
        fclose(logFile);
    }
}

sqlite3 *open_database(const char *databaseFile) {
    int rc = sqlite3_open(databaseFile, &db);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        log_transaction(DATABASE_NAME, "OPEN", "ERROR", "Database open error");
        return NULL;
    }
    log_transaction(DATABASE_NAME, "OPEN", "ERROR", "Database opened successfully");
    return db;
}

void close_database() {
    sqlite3_close(db);
    log_transaction(DATABASE_NAME, "CLOSE", "SUCCESS", "Database closed");
}

int create_requests_table() {
    char *sql = "CREATE TABLE IF NOT EXISTS " REQUESTS_TABLE_NAME " ("
                "request_id VARCHAR(37) PRIMARY KEY, "
                "evaluation_status VARCHAR(10), "
                "received_on DATETIME, "
                "evaluated_on DATETIME, "
                "conveyed_on DATETIME);";

    // evaluation status can be: "in-process", "done", "pending"

    int rc = sqlite3_exec(db, sql, 0, 0, 0);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Table creation error: %s\n", sqlite3_errmsg(db));
        log_transaction(REQUESTS_TABLE_NAME, "CREATE TABLE", "ERROR", "Table creation error");
    } else {
        log_transaction(REQUESTS_TABLE_NAME, "CREATE TABLE", "SUCCESS", "Table created successfully");
    }

    return rc;
}

void generate_UUID(char *uuid_str) {
    uuid_t uuid;
    uuid_generate(uuid);
    uuid_unparse(uuid, uuid_str);
}

int execute_in_transaction(sqlite3 *db, const char *operationType, const char *sql, const char *tableName) {
    int rc = sqlite3_exec(db, "BEGIN TRANSACTION", 0, 0, 0);
    rc = sqlite3_exec(db, sql, 0, 0, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "%s error: %s\n", operationType, sqlite3_errmsg(db));
        log_transaction(tableName, operationType, "ERROR", "Operation error");
        sqlite3_exec(db, "ROLLBACK", 0, 0, 0);
    } else {
        log_transaction(tableName, operationType, "SUCCESS", "Operation successful");
        sqlite3_exec(db, "COMMIT", 0, 0, 0);
    }
    return rc;
}

int insert_data(char *requestID) {
    char insert_sql[SQL_LEN];
    snprintf(insert_sql, sizeof(insert_sql),
             "INSERT INTO " REQUESTS_TABLE_NAME " (request_id, evaluation_status, received_on) "
             "VALUES ('%s', 'pending', DATETIME('now'))",
             requestID);
    printf("Executing in transaction: %s\n", insert_sql);
    int rc = execute_in_transaction(db, "INSERT", insert_sql, REQUESTS_TABLE_NAME);
    return rc;
}

int fetch_data(const char *request_id, FetchResult *result) {
    char select_sql[SQL_LEN];
    snprintf(select_sql, sizeof(select_sql),
             "SELECT * FROM " REQUESTS_TABLE_NAME " WHERE request_id = '%s'", request_id);

    printf("Executing in transaction: %s\n", select_sql);
    int rc = execute_in_transaction(db, "FETCH", select_sql, REQUESTS_TABLE_NAME);

    if (rc == SQLITE_OK) {
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(db, select_sql, -1, &stmt, 0) == SQLITE_OK) {
            rc = sqlite3_step(stmt);

            if (rc == SQLITE_ROW) {
                strcpy(result->request_id, (const char *)sqlite3_column_text(stmt, 0));

                strcpy(result->evaluation_status, (const char *)sqlite3_column_text(stmt, 1));

                const char *received_on_text = (const char *)sqlite3_column_text(stmt, 2);
                if (received_on_text != NULL) {
                    strcpy(result->evaluated_on, received_on_text);
                } else {
                    strcpy(result->evaluated_on, "");
                }

                const char *evaluated_on_text = (const char *)sqlite3_column_text(stmt, 3);
                if (evaluated_on_text != NULL) {
                    strcpy(result->evaluated_on, evaluated_on_text);
                } else {
                    strcpy(result->evaluated_on, "");
                }

                const char *conveyed_on_text = (const char *)sqlite3_column_text(stmt, 4);
                if (conveyed_on_text != NULL) {
                    strcpy(result->evaluated_on, conveyed_on_text);
                } else {
                    update_data(result->request_id, NULL, 1);
                }

                rc = SQLITE_OK;
            }
            else if (rc == SQLITE_DONE) {
                fprintf(stderr, "No data found for request_id: %s\n", request_id);
                rc = SQLITE_EMPTY;
            } else {
                fprintf(stderr, "Error fetching data: %s\n", sqlite3_errmsg(db));
            }
            sqlite3_finalize(stmt);
        }
    }
    return rc;
}

int update_data(const char *request_id, char *new_status, int is_conveyed) {
    char update_sql[SQL_LEN];
    if(is_conveyed && !new_status)
        snprintf(update_sql, sizeof(update_sql),
             "UPDATE " REQUESTS_TABLE_NAME " SET conveyed_on = DATETIME('now') WHERE request_id = '%s'", request_id);
    else if(!strcmp(new_status, "in-process"))
        snprintf(update_sql, sizeof(update_sql),
             "UPDATE " REQUESTS_TABLE_NAME " SET evaluation_status = '%s' WHERE request_id = '%s'", new_status, request_id);
    else if(!strcmp(new_status, "done"))
        snprintf(update_sql, sizeof(update_sql),
             "UPDATE " REQUESTS_TABLE_NAME " SET evaluation_status = '%s', evaluated_on = DATETIME('now') WHERE request_id = '%s'", new_status, request_id);

    printf("Executing in transaction: %s\n", update_sql);
    return execute_in_transaction(db, "UPDATE", update_sql, REQUESTS_TABLE_NAME);
}

void handle_signal(int signum) {
    printf("Received signal %d, Cleaning up...\n", signum);

    // Close the database gracefully
    close_database();

    exit(0);
}

void setup_signal_handler() {
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    // Set up a handler for SIGTERM and SIGINT signals
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
}

// Function to load data from the database into the queue
void load_queue_from_database(Queue *queue) {
    // Assuming that request_id and sockfd are the columns you want to retrieve
    char select_sql[SQL_LEN];
    snprintf(select_sql, sizeof(select_sql),
             "SELECT request_id FROM " REQUESTS_TABLE_NAME " WHERE evaluation_status = 'pending' OR evaluation_status = 'in-process'");

    printf("Executing in transaction: %s\n", select_sql);
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, select_sql, -1, &stmt, 0) == SQLITE_OK) {
        int rc = sqlite3_step(stmt);
        while (rc == SQLITE_ROW) {
            const char *request_id = (const char *)sqlite3_column_text(stmt, 0);

            QueueData *data = (QueueData *)malloc(sizeof(QueueData));
            if(request_id != NULL) {
                strncpy(data->requestID, request_id, sizeof(data->requestID));
                data->sockfd = -1;
            }
            // Enqueue the data into the queue
            pthread_mutex_lock(&qLock);
            enqueue(queue, (void *)data);
            pthread_mutex_unlock(&qLock);

            // Move to the next row
            rc = sqlite3_step(stmt);
        }
        sqlite3_finalize(stmt);
    } else {
        fprintf(stderr, "Error preparing SQL statement: %s\n", sqlite3_errmsg(db));
    }
}
