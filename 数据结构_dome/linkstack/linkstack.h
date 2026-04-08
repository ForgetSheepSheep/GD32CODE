#ifndef  __LINKSTACK_H
#define  __LINKSTACK_H
#include <stdio.h>
#include <stdlib.h>
typedef int data_t;
typedef struct node{
    data_t data;
    struct node *node;
}listnode, *linkstack;

linkstack StackCreate();
int StackPush(linkstack s, data_t value);
data_t StackPop(linkstack s);
int StackEmpyt(linkstack s);
data_t StackTop(linkstack s);
linkstack StackFree(linkstack s);

#endif /* __LINKSTACK_H */
