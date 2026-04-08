#ifndef __LINKLIST_H__
#define __LINKLIST_H__

#include <stdio.h>
#include <stdlib.h>

typedef int data_t;

/* 单链表结点定义。
 * 实现思路：使用带头结点的结构，头结点只负责统一管理链表入口，不存放用户有效数据。
 */
typedef struct node {
    data_t data;
    struct node *next;
} listnode, *linklist;

/* 创建空链表。
 * 实现思路：动态申请一个头结点，并将 next 置空，表示当前没有任何数据结点。
 */
linklist ListCreate(void);

/* 销毁整条链表。
 * 实现思路：从头结点开始逐个释放所有结点，包含头结点本身。
 */
int ListDestroy(linklist H);

/* 头插法插入结点。
 * 实现思路：把新结点插到头结点之后，这样不需要遍历链表即可完成插入。
 */
int ListHeadInsert(linklist H, data_t value);

/* 尾插法插入结点。
 * 实现思路：先遍历到尾结点，再把新结点挂接到尾部。
 */
int ListTailInsert(linklist H, data_t value);

/* 打印链表中的所有数据。
 * 实现思路：从首元结点开始顺序遍历，按链表先后顺序输出每个数据域。
 */
int ListShow(linklist H);

/* 获取指定位置的结点。
 * 实现思路：约定 -1 表示头结点，其余位置从 0 开始，按顺序遍历到目标结点。
 */
linklist ListGet(linklist H, int pos);

/* 按位置插入结点。
 * 实现思路：先找到目标位置的前驱结点，再完成指针重连，把新结点插入进去。
 */
int ListInsert(linklist H, data_t value, int pos);

/* 按位置删除结点。
 * 实现思路：先找到待删结点的前驱结点，再断开目标结点并释放其空间。
 */
int ListDelete(linklist H, data_t value, int pos);

/* 释放整条链表，并返回空指针，便于调用者直接回收头指针。 */
linklist ListFree(linklist H);

#endif /* __LINKLIST_H__ */
