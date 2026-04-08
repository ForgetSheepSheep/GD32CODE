#include "squeue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

pQueue_t QueueCreate(void)
{
    pQueue_t q = (pQueue_t)malloc(sizeof(Queue_t));

    if (q == NULL) {
        printf(" Queue malloc fail\r\n");
        return NULL;
    }

    /*
     * 你原来的错误：
     *     memset(Q->data, 0, N * sizeof(Queue_t));
     *
     * 为什么错：
     * sizeof(Queue_t) 是整个队列结构体的大小，
     * 不是 data 数组单个元素的大小。
     * 你再乘以 N 之后，会把 data 数组后面的内存也一起乱写，
     * 这是典型的越界写内存错误，严重时会直接导致程序崩溃。
     *
     * 正确做法：
     * 直接把整个队列对象清零，一次完成 data/front/rear 初始化。
     */
    memset(q, 0, sizeof(Queue_t));

    /*
     * 你原来的错误：
     * 分配成功后仍然 return NULL。
     *
     * 为什么错：
     * main() 里会把 NULL 当成“创建失败”，
     * 所以即使 malloc 成功，主函数也会误判为失败。
     */
    return q;
}

bool QueuePush(pQueue_t q, datatype value)
{
    if (q == NULL) {
        printf("queue null\r\n");
        return false;
    }

    if (QueueFull(q)) {
        printf(" Queue Full Push fail\r\n");
        return false;
    }

    q->data[q->rear] = value;
    q->rear = (q->rear + 1) % N;
    return true;
}

bool QueuePop(pQueue_t q, datatype *value)
{
    if (q == NULL || value == NULL) {
        printf("queue null\r\n");
        return false;
    }

    /*
     * 你原来的错误：
     * 出队前没有先判断队列是否为空。
     *
     * 为什么错：
     * 当 front == rear 时，队列里根本没有有效元素。
     * 这时如果还去读 q->data[q->front]，读出来的只是旧数据，
     * 逻辑上就是错的。
     */
    if (QueueEmpty(q)) {
        printf("queue empty\r\n");
        return false;
    }

    *value = q->data[q->front];
    q->front = (q->front + 1) % N;
    return true;
}

bool QueueEmpty(pQueue_t q)
{
    if (q == NULL) {
        printf("queue null\r\n");
        return false;
    }

    /*
     * 你原来的错误：
     * rear == front 时返回 false。
     *
     * 为什么错：
     * 对循环队列来说，rear == front 的含义就是“队空”，
     * 所以这里必须返回 true。
     * 你原来的逻辑刚好写反了。
     */
    return q->rear == q->front;
}

bool QueueEmpyt(pQueue_t q)
{
    return QueueEmpty(q);
}

bool QueueFull(pQueue_t q)
{
    if (q == NULL) {
        printf("queue null\r\n");
        return false;
    }

    /*
     * 你原来的错误：
     * 队列真的满了时，你反而返回 false。
     *
     * 为什么错：
     * 函数名叫 QueueFull，语义就是“是否已满”。
     * 既然条件已经判断出队满，就应该返回 true，
     * 不能把真假含义写反。
     */
    return ((q->rear + 1) % N) == q->front;
}

bool QueueClear(pQueue_t q)
{
    if (q == NULL) {
        printf("queue null\r\n");
        return false;
    }

    q->front = q->rear = 0;
    return true;
}

void QueueFree(pQueue_t *q)
{
    if (q == NULL || *q == NULL) {
        printf("queue null\r\n");
        return;
    }

    /*
     * 你原来的错误：
     * free(q) 之后只把函数内部自己的 q 置成 NULL。
     *
     * 为什么错：
     * 这里的 q 只是形参，是外部指针的一份拷贝。
     * 你把这份拷贝改成 NULL，并不会影响 main() 里的那个指针变量。
     * 所以 main() 里的指针依然指向已经释放的内存，形成野指针。
     */
    free(*q);
    *q = NULL;
    printf("free succeed\r\n");
}
