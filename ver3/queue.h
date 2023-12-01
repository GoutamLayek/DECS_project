#include <stdio.h>
#include <stdlib.h>

typedef struct Node{
    int data;
    struct Node* next;
} Node;

typedef struct Queue{
    Node* front;
    Node* rear;
    size_t size;
} Queue;

Node* createNode();
Queue *initializeQueue();
int isEmpty(Queue* queue);

void enqueue(Queue* queue, int data);
int dequeue(Queue* queue);
int getFront(Queue* queue);
