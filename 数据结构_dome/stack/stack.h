#ifndef __STACK_H__
#define __STACK_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int data_t;

/* 顺序栈：用一段连续内存保存栈元素，top 指向当前栈顶下标。 */
typedef struct
{
    data_t *data;
    int maxlen;
    int top;
} sqstack;

/* 基本操作接口 */
sqstack *StackCreate(int len);
int StackEmpty(sqstack *s);
int StackFull(sqstack *s);
int StackPush(sqstack *s, data_t value);
data_t StackPop(sqstack *s);
data_t StackTop(sqstack *s);
int StackClear(sqstack *s);
int StackFree(sqstack *s);

#endif /* __STACK_H__ */
