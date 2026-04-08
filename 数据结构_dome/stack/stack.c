#include "stack.h"

/*
 * 顺序栈的实现思路：
 * 1. 用 data 指向一段连续空间保存元素。
 * 2. top 初始为 -1，表示空栈。
 * 3. 入栈时先判断是否已满，再让 top 加 1，把新值放到 data[top]。
 * 4. 出栈时先判断是否为空，取出 data[top] 后再让 top 减 1。
 * 5. 这里 data_t 定义为 int，所以出错时暂时返回 -1。
 *    如果后面希望支持任意整数，实际项目里更推荐“返回状态码 + 输出参数”。
 */

sqstack *StackCreate(int len)
{
    sqstack *s;

    if (len <= 0)
    {
        printf("stack len is invalid\r\n");
        return NULL;
    }

    s = (sqstack *)malloc(sizeof(sqstack));
    if (s == NULL)
    {
        printf("malloc sqstack failed\r\n");
        return NULL;
    }

    s->data = (data_t *)malloc(len * sizeof(data_t));
    if (s->data == NULL)
    {
        printf("malloc stack data failed\r\n");
        free(s);
        return NULL;
    }

    memset(s->data, 0, len * sizeof(data_t));
    s->maxlen = len;
    s->top = -1;

    return s;
}

int StackEmpty(sqstack *s)
{
    if (s == NULL)
    {
        printf("stack is NULL\r\n");
        return -1;
    }

    return (s->top == -1);
}

int StackFull(sqstack *s)
{
    if (s == NULL)
    {
        printf("stack is NULL\r\n");
        return -1;
    }

    return (s->top == s->maxlen - 1);
}

int StackPush(sqstack *s, data_t value)
{
    if (s == NULL)
    {
        printf("stack is NULL\r\n");
        return -1;
    }

    if (StackFull(s))
    {
        printf("stack is full\r\n");
        return -1;
    }

    s->top++;
    s->data[s->top] = value;
    return 0;
}

data_t StackPop(sqstack *s)
{
    data_t value;

    if (s == NULL)
    {
        printf("stack is NULL\r\n");
        return -1;
    }

    if (StackEmpty(s))
    {
        printf("stack is empty\r\n");
        return -1;
    }

    value = s->data[s->top];
    s->top--;
    return value;
}

data_t StackTop(sqstack *s)
{
    if (s == NULL)
    {
        printf("stack is NULL\r\n");
        return -1;
    }

    if (StackEmpty(s))
    {
        printf("stack is empty\r\n");
        return -1;
    }

    return s->data[s->top];
}

int StackClear(sqstack *s)
{
    if (s == NULL)
    {
        printf("stack is NULL\r\n");
        return -1;
    }

    s->top = -1;
    return 0;
}

int StackFree(sqstack *s)
{
    if (s == NULL)
    {
        printf("stack is NULL\r\n");
        return -1;
    }

    if (s->data != NULL)
    {
        free(s->data);
        s->data = NULL;
    }

    free(s);
    return 0;
}
