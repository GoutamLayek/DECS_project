#include "persistent_queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

// Function to create a new node
static Node *createNode(void *data) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->data = data;
    newNode->next = NULL;
    return newNode;
}

// Function to initialize the queue
Queue *initialize_queue() {
    Queue *queue = (Queue *)malloc(sizeof(Queue));
    queue->front = queue->rear = NULL;
    queue->length = 0;
    puts("Initialized queue");
    return queue;
}

// Function to enqueue data into the queue
void enqueue(Queue *queue, void *data) {
    Node *newNode = createNode(data);
    newNode->data = data;
    if (queue->rear == NULL) {
        queue->front = queue->rear = newNode;
    } else {
        queue->rear->next = newNode;
        queue->rear = newNode;
    }
    queue->length++;
    printf("Enqueued %s [Queue Length: %ld]\n", ((QueueData *)data)->requestID, queue->length);
}

// Function to dequeue data from the queue
void *dequeue(Queue *queue) {
    if (queue->front == NULL) {
        printf("Queue is empty\n");
        return NULL;
    }

    Node *temp = queue->front;
    QueueData *data = temp->data;
    queue->front = queue->front->next;
    if (queue->front == NULL) {
        queue->rear = NULL;
    }
    queue->length--;
    free(temp);
    printf("Dequeued %s [Queue Length: %ld]\n", data->requestID, queue->length);
    return (void*) data;
}

bool is_empty(Queue *queue) {
    return (queue == NULL || queue->front == NULL);
}

// Function to freea the memory allocated for the queue
void free_queue(Queue *queue) {
    while (queue->front != NULL) {
        Node *temp = queue->front;
        queue->front = queue->front->next;
        free(temp->data);
        free(temp);
    }
}

void *get_front(Queue *queue) {
    if (is_empty(queue))
    {
        printf("Queue is empty!!\n");
        return NULL;
    }
    return queue->front->data;
}

int find_position(Queue *queue, char *requestID) {
    Node *current = queue->front;
    int position = 0;

    while (current != NULL) {
        char *queuedRequestID = ((QueueData *)current->data)->requestID;
        if (strncmp(queuedRequestID, requestID, (unsigned long)37)) {
            printf("Queue[%s]=%d\n", queuedRequestID, position);
            return position;
        }
        current = current->next;
        position++;
    }
    return -1;
}
