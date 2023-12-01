#include "queue.h"

Node* createNode(int data){
    Node* newnode = (Node*)malloc(sizeof(Node));
    newnode->data = data;
    newnode->next = NULL;
    return newnode;
}
Queue* initializeQueue(){
    Queue* queue = (Queue*)malloc(sizeof(Queue));
    queue->front = queue->rear = NULL;
    queue->size = 0;
    return queue;
}

int isEmpty(Queue* queue){
    return (queue == NULL || queue->front == NULL);
}

void enqueue(Queue* queue, int data){
    Node* newnode = createNode(data);
    if(isEmpty(queue)){
        queue->front = queue->rear = newnode;
        return;
    }
    queue->rear->next = newnode;
    queue->rear = newnode;
    queue->size += 1;

}

int dequeue(Queue* queue){
    if(isEmpty(queue)){
        printf("Queue is empty!!\n");
        return -1;
    }
    Node* temp = queue->front;
    int data = temp->data;
    queue->front = queue->front->next;
    if(queue->front == NULL){
        queue->rear = NULL;
    }
    free(temp);
    queue->size -=1;
    return data;
}

int getFront(Queue* queue){
     if(isEmpty(queue)){
        printf("Queue is empty!!\n");
        return -1;
    }
    return queue->front->data;
}