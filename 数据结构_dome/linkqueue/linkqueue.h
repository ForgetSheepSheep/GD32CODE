#ifndef  __LINKQUEUE_H
#define  __LINKQUEUE_H
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef int ElemType;

/* 结点 */
typedef struct QueueNode
{
    ElemType data;
    struct QueueNode *next;
} QueueNode;

/* 链式队列 */
typedef struct
{
    QueueNode *front;
    QueueNode *rear;
} LinkQueue;


#endif /* __LINKQUEUE_H */
