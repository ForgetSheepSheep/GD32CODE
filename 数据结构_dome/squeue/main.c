#include "squeue.h"

#include <stdio.h>

int main(void)
{
    pQueue_t q;
    datatype value;

    q = QueueCreate();
    if (q == NULL) {
        printf(" Queue Create Fail\r\n");
        return -1;
    }

    /*
     * 先入队几个测试数据。
     * 这里理论上应该检查 QueuePush 的返回值，
     * 因为如果队列已满，入队会失败。
     * 这里只是演示，所以直接写死几个数据。
     */
    QueuePush(q, 10);
    QueuePush(q, 100);
    QueuePush(q, 1000);
    QueuePush(q, 10000);

    /*
     * 你原来 while (!QueueEmpyt(q)) 这个思路本身没问题，
     * 真正错的是 QueueEmpyt() 函数内部把真假返回写反了，
     * 所以循环条件判断结果也跟着错了。
     */
    while (!QueueEmpty(q)) {
        if (QueuePop(q, &value)) {
            printf("QueueData:%d\r\n", value);
        }
    }

    if (QueueClear(q)) {
        printf("clear succeed\r\n");
    }

    QueueFree(&q);
    return 0;
}
