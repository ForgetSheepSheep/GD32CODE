#include "linklist.h"

/* 创建一个带头结点的空链表。
 * 实现思路：申请头结点空间，头结点不存放有效数据，只把 next 置空作为统一操作入口。
 */
linklist ListCreate(void)
{
    linklist head = (linklist)malloc(sizeof(listnode));

    if (head == NULL) {
        printf("[ERROR] malloc failed in ListCreate\n");
        return NULL;
    }

    head->data = 0;
    head->next = NULL;
    return head;
}

/* 销毁整条链表并释放所有结点。
 * 实现思路：从当前结点开始逐个保存后继结点地址，释放当前结点后继续向后遍历。
 */
int ListDestroy(linklist H)
{
    linklist next = NULL;

    if (H == NULL) {
        printf("[ERROR] Linklist is NULL\n");
        return -1;
    }

    while (H != NULL) {
        next = H->next;
        free(H);
        H = next;
    }

    return 0;
}

/* 在头结点之后插入一个新结点。
 * 实现思路：先申请新结点，再让新结点指向原来的首元结点，最后由头结点接管新结点。
 */
int ListHeadInsert(linklist H, data_t value)
{
    linklist node = NULL;

    if (H == NULL) {
        printf("[ERROR] Linklist is NULL\n");
        return -1;
    }

    node = (linklist)malloc(sizeof(listnode));
    if (node == NULL) {
        printf("[ERROR] malloc failed in ListHeadInsert\n");
        return -1;
    }

    node->data = value;
    node->next = H->next;
    H->next = node;

    return 0;
}

/* 在链表尾部追加一个新结点。
 * 实现思路：先创建新结点，再从头结点开始找到最后一个结点，并把它的 next 指向新结点。
 */
int ListTailInsert(linklist H, data_t value)
{
    linklist node = NULL;
    linklist tail = NULL;

    if (H == NULL) {
        printf("[ERROR] Linklist is NULL\n");
        return -1;
    }

    node = (linklist)malloc(sizeof(listnode));
    if (node == NULL) {
        printf("[ERROR] malloc failed in ListTailInsert\n");
        return -1;
    }

    node->data = value;
    node->next = NULL;

    tail = H;
    while (tail->next != NULL) {
        tail = tail->next;
    }

    tail->next = node;
    return 0;
}

/* 按顺序打印所有数据结点。
 * 实现思路：从首元结点开始向后遍历，每访问一个结点就输出其 data，直到 next 为 NULL。
 */
int ListShow(linklist H)
{
    linklist node = NULL;

    if (H == NULL) {
        printf("[ERROR] Linklist is NULL\n");
        return -1;
    }

    node = H->next;
    while (node != NULL) {
        printf("%d ", node->data);
        node = node->next;
    }
    printf("\n");

    return 0;
}

/* 获取指定位置的结点指针。
 * 实现思路：约定 -1 返回头结点，其余非负位置从首元结点开始计数，顺序遍历到目标位置。
 */
linklist ListGet(linklist H, int pos)
{
    linklist node = NULL;
    int index = 0;

    if (H == NULL) {
        printf("[ERROR] Linklist is NULL\n");
        return NULL;
    }

    if (pos < -1) {
        printf("[ERROR] Invalid position: %d\n", pos);
        return NULL;
    }

    if (pos == -1) {
        return H;
    }

    node = H->next;
    while (node != NULL && index < pos) {
        node = node->next;
        index++;
    }

    if (node == NULL) {
        printf("[ERROR] Position %d is out of range\n", pos);
    }

    return node;
}

/* 在指定位置插入一个新结点。
 * 实现思路：先找到待插入位置的前驱结点，再把新结点插入到前驱结点和原后继结点之间。
 */
int ListInsert(linklist H, data_t value, int pos)
{
    linklist prev = NULL;
    linklist node = NULL;

    if (H == NULL) {
        printf("[ERROR] Linklist is NULL\n");
        return -1;
    }

    if (pos < 0) {
        printf("[ERROR] Invalid insert position: %d\n", pos);
        return -1;
    }

    prev = ListGet(H, pos - 1);
    if (prev == NULL) {
        printf("[ERROR] Cannot insert at position %d\n", pos);
        return -1;
    }

    node = (linklist)malloc(sizeof(listnode));
    if (node == NULL) {
        printf("[ERROR] malloc failed in ListInsert\n");
        return -1;
    }

    node->data = value;
    node->next = prev->next;
    prev->next = node;

    return 0;
}

/* 删除指定位置上的结点。
 * 实现思路：先定位目标位置的前驱结点，确认待删结点存在后断链，再释放该结点空间。
 */
int ListDelete(linklist H, data_t value, int pos)
{
    linklist prev = NULL;
    linklist node = NULL;

    (void)value;

    if (H == NULL) {
        printf("[ERROR] Linklist is NULL\n");
        return -1;
    }

    if (pos < 0) {
        printf("[ERROR] Invalid delete position: %d\n", pos);
        return -1;
    }

    prev = ListGet(H, pos - 1);
    if (prev == NULL) {
        printf("[ERROR] Cannot delete at position %d\n", pos);
        return -1;
    }

    /* 前驱结点后面没有数据结点，说明删除位置无效。 */
    if (prev->next == NULL) {
        printf("[ERROR] Delete position %d is out of range\n", pos);
        return -1;
    }

    /* 先修改指针关系，再释放待删结点。 */
    node = prev->next;
    prev->next = node->next;
    free(node);
    node = NULL;
    return 0;
}

/* 释放整条链表，并返回空指针。
 * 实现思路：逐个释放所有结点，最后统一返回 NULL，方便调用者直接把头指针置空。
 */
linklist ListFree(linklist H)
{
    linklist prev = NULL;

    if (H == NULL) {
        printf("[ERROR] Linklist is NULL\n");
        return NULL;
    }

    while (H != NULL) {
        prev = H;
        H = H->next;
        free(prev);
    }

    return NULL;
}
