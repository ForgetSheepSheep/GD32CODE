#ifndef SQUEUE_H
#define SQUEUE_H

#include <stdbool.h>
#include <stddef.h>

typedef int datatype;

#define N 64

/*
 * 这是一个循环顺序队列。
 * 由于队满条件定义为：
 *     (rear + 1) % N == front
 * 所以必须故意空出一个位置，用来区分“队空”和“队满”。
 * 因此这个队列虽然数组长度是 N，但实际最多只能存 N - 1 个元素。
 */
typedef struct {
    datatype data[N];
    int front;
    int rear;
} Queue_t, *pQueue_t;

/*
 * 在堆上创建一个队列，并完成初始化。
 */
pQueue_t QueueCreate(void);

/*
 * 在队尾插入一个元素。
 * 成功返回 true，失败返回 false。
 * 失败的原因通常是：
 * 1. 队列指针无效
 * 2. 队列已满
 */
bool QueuePush(pQueue_t q, datatype value);

/*
 * 这里特意改成“返回 bool + 输出参数”的形式。
 *
 * 你原来的错误：
 * 原函数直接返回 datatype。
 *
 * 为什么错：
 * 如果函数失败时返回 0，而队列里本来就可能合法存放 0，
 * 调用者就无法区分“这是弹出的真实数据 0”，还是“函数出错了”。
 *
 * 正确做法：
 * 用 bool 表示函数是否成功，用 value 把真正的数据带出去。
 */
bool QueuePop(pQueue_t q, datatype *value);

/*
 * 队列为空时返回 true，否则返回 false。
 */
bool QueueEmpty(pQueue_t q);

/*
 * 你原来把 Empty 拼成了 Empyt。
 * 这里保留旧名字，只是为了兼容你原来的调用方式。
 * 真正推荐使用的名字是 QueueEmpty。
 */
bool QueueEmpyt(pQueue_t q);

/*
 * 队列满时返回 true，否则返回 false。
 */
bool QueueFull(pQueue_t q);

/*
 * 清空队列。
 * 这里只是把 front 和 rear 重新置零，
 * 并不会释放队列对象本身。
 */
bool QueueClear(pQueue_t q);

/*
 * 这里改成二级指针释放。
 *
 * 你原来的错误：
 * 原函数 free 之后返回 NULL，期待调用者写成：
 *     q = QueueFree(q);
 *
 * 为什么错：
 * 如果调用者只是写 QueueFree(q)，那么主函数里的 q 不会自动变成 NULL，
 * 它仍然保存着已经被释放的地址，变成野指针。
 *
 * 正确做法：
 * 传入 pQueue_t *，这样函数内部可以直接把调用者手里的指针也置空。
 */
void QueueFree(pQueue_t *q);

#endif
