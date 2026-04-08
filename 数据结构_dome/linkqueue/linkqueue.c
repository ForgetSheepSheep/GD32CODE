#include "linkqueue.h"


bool QueueCreate(LinkQueue *q)
{
   QueueNode *node  = (QueueNode *)malloc(sizeof(QueueNode));
    if (node == NULL)
    {
        printf("node malloc fail\r\n");
        return false;
    }
    node->next = NULL;
    q->front = node;
    q->rear = node;
    return true;
}
bool QueueEmpty(LinkQueue *q)
{
    return q->front == q->rear;
}