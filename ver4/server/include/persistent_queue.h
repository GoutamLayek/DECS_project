#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifndef PERSISTENT_QUEUE_H
#define PERSISTENT_QUEUE_H


typedef struct QueueData {
    int sockfd;
    char requestID[37];
} QueueData;

typedef struct Node {
    void *data;
    struct Node* next;
} Node;

typedef struct Queue {
    long length;
    Node* front;
    Node* rear;
} Queue;

Node* create_node(void *data);
Queue *initialize_queue();
bool is_empty(Queue* queue);
void enqueue(Queue* queue, void* data);
void *dequeue(Queue *queue);
void free_queue(Queue *queue);
void *get_front(Queue *queue);
int find_position(Queue* queue, char *requestID);


#endif