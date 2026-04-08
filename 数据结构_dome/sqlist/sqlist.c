#include "sqlist.h"
#include "sqlist.h"

/**
 * @brief 创建一个顺序表
 * @return 成功返回顺序表句柄，失败返回 NULL
 */
pSqList_t ListCreate(void)
{
    pSqList_t L;

    printf("[DEBUG] Enter ListCreate\r\n");

    L = (pSqList_t)malloc(sizeof(SqList_t));
    if (L == NULL)
    {
        printf("[ERROR] list malloc failed\r\n");
        return NULL;
    }

    memset(L, 0, sizeof(SqList_t));
    L->last = -1;

    printf("[DEBUG] ListCreate success, L = %p, last = %d\r\n", L, L->last);
    return L;
}

/**
 * @brief 销毁顺序表
 * @param list 顺序表句柄
 * @return true 销毁成功
 * @return false 销毁失败
 */
bool ListDelete(pSqList_t list)
{
    printf("[DEBUG] Enter ListDelete, list = %p\r\n", list);

    if (list == NULL)
    {
        printf("[ERROR] List not found\r\n");
        return false;
    }

    free(list);
    list = NULL;

    printf("[DEBUG] ListDelete success\r\n");
    return true;
}

/**
 * @brief 清空顺序表中的所有数据
 * @param list 顺序表句柄
 * @return true 清空成功
 * @return false 清空失败
 */
bool ListClear(pSqList_t list)
{
    printf("[DEBUG] Enter ListClear, list = %p\r\n", list);

    if (list == NULL)
    {
        printf("[ERROR] List is NULL\r\n");
        return false;
    }

    memset(list, 0, sizeof(SqList_t));
    list->last = -1;

    printf("[DEBUG] ListClear success, last = %d\r\n", list->last);
    return true;
}

/**
 * @brief 判断顺序表是否为空
 * @param list 顺序表句柄
 * @return true 顺序表为空
 * @return false 顺序表非空
 */
bool ListEmpty(pSqList_t list)
{
    printf("[DEBUG] Enter ListEmpty, list = %p\r\n", list);

    if (list == NULL)
    {
        printf("[ERROR] List is NULL\r\n");
        return true;
    }

    if (list->last == -1)
    {
        printf("[DEBUG] List is empty\r\n");
        return true;
    }
    else
    {
        printf("[DEBUG] List is not empty, last = %d\r\n", list->last);
        return false;
    }
}

/**
 * @brief 获取顺序表当前长度
 * @param list 顺序表句柄
 * @return 顺序表中元素的个数
 */
uint8_t ListLength(pSqList_t list)
{
    uint8_t length;

    printf("[DEBUG] Enter ListLength, list = %p\r\n", list);

    if (list == NULL)
    {
        printf("[ERROR] List is NULL\r\n");
        return 0;
    }

    length = list->last + 1;
    printf("[DEBUG] ListLength = %d\r\n", length);

    return length;
}

/**
 * @brief 查找顺序表中是否存在指定元素
 * @param list 顺序表句柄
 * @param value 要查找的元素值
 * @return 元素下标，找不到返回 -1
 */
int ListLocate(pSqList_t list, data_t value)
{
    uint8_t i = 0;

    printf("[DEBUG] Enter ListLocate, list = %p, value = %d\r\n", list, value);

    if (list == NULL)
    {
        printf("[ERROR] no list\r\n");
        return -1;
    }

    for (i = 0; i <= list->last; i++)
    {
        printf("[DEBUG] Compare value=%d with buff[%d]=%d\r\n", value, i, list->buff[i]);

        if (value == list->buff[i])
        {
            printf("[DEBUG] Found value at pos = %d\r\n", i);
            return i;
        }
    }

    printf("[DEBUG] Value %d not found\r\n", value);
    return -1;
}

/**
 * @brief 在顺序表指定位置插入元素
 * @param list 顺序表句柄
 * @param value 要插入的元素值
 * @param pos 插入位置
 * @return true 插入成功
 * @return false 插入失败
 */
bool ListInsert(pSqList_t list, data_t value, uint8_t pos)
{
    int i;

    printf("[DEBUG] Enter ListInsert, list = %p, value = %d, pos = %d\r\n", list, value, pos);

    if (list == NULL)
    {
        printf("[ERROR] list is NULL\r\n");
        return false;
    }

    if (list->last >= BUFF_SIZE - 1)
    {
        printf("[ERROR] list is full, last = %d\r\n", list->last);
        return false;
    }

    if (pos > list->last + 1)
    {
        printf("[ERROR] pos is invalid, pos = %d, valid range = 0~%d\r\n", pos, list->last + 1);
        return false;
    }

    printf("[DEBUG] Before insert, last = %d\r\n", list->last);

    for (i = list->last; i >= pos; i--)
    {
        printf("[DEBUG] Move buff[%d] -> buff[%d], value = %d\r\n", i, i + 1, list->buff[i]);
        list->buff[i + 1] = list->buff[i];
    }

    list->buff[pos] = value;
    list->last++;

    printf("[DEBUG] Insert success, buff[%d] = %d, new last = %d\r\n", pos, list->buff[pos], list->last);
    return true;
}

/**
 * @brief 显示顺序表内容
 * @param list 顺序表句柄
 * @return true 显示成功
 * @return false 显示失败
 */
bool ListShow(pSqList_t list)
{
    uint8_t i;

    printf("[DEBUG] Enter ListShow, list = %p\r\n", list);

    if (list == NULL)
    {
        printf("[ERROR] List error\r\n");
        return false;
    }

    if (list->last == -1)
    {
        printf("[DEBUG] List empty\r\n");
        return false;
    }

    printf("*************** Show list data begin ***************\r\n");
    for (i = 0; i <= list->last; i++)
    {
        printf("[DEBUG] buff[%d] = %d\r\n", i, list->buff[i]);
    }
    printf("**************** Show list data end ****************\r\n");

    return true;
}

/**
 * @brief 删除顺序表指定位置元素
 * @param list 顺序表句柄
 * @param pos 删除位置
 * @return true 删除成功
 * @return false 删除失败
 */
bool ListDeletePos(pSqList_t list, uint8_t pos)
{
    uint8_t i;

    printf("[DEBUG] Enter ListDeletePos, list = %p, pos = %d\r\n", list, pos);

    if (list == NULL)
    {
        printf("[ERROR] List is NULL\r\n");
        return false;
    }

    if (list->last == -1)
    {
        printf("[ERROR] The list is empty\r\n");
        return false;
    }

    if (pos > list->last)
    {
        printf("[ERROR] pos error, pos = %d, last = %d\r\n", pos, list->last);
        return false;
    }

    printf("[DEBUG] Delete target: buff[%d] = %d\r\n", pos, list->buff[pos]);

    for (i = pos + 1; i <= list->last; i++)
    {
        printf("[DEBUG] Move buff[%d] -> buff[%d], value = %d\r\n", i, i - 1, list->buff[i]);
        list->buff[i - 1] = list->buff[i];
    }

    list->last--;

    printf("[DEBUG] Delete success, new last = %d\r\n", list->last);
    return true;
}

/**
 * @brief 合并两个顺序表（将 L2 中不重复元素追加到 L1）
 * @param L1 顺序表1
 * @param L2 顺序表2
 * @return true 合并成功
 * @return false 合并失败
 */
bool ListMerge(pSqList_t L1, pSqList_t L2)
{
    uint8_t i;
    int ret;

    printf("[DEBUG] Enter ListMerge, L1 = %p, L2 = %p\r\n", L1, L2);

    if (L1 == NULL || L2 == NULL)
    {
        printf("[ERROR] no list\r\n");
        return false;
    }

    printf("[DEBUG] Before merge: L1 last = %d, L2 last = %d\r\n", L1->last, L2->last);

    for (i = 0; i <= L2->last; i++)
    {
        printf("[DEBUG] Check L2->buff[%d] = %d\r\n", i, L2->buff[i]);

        ret = ListLocate(L1, L2->buff[i]);
        if (ret == -1)
        {
            printf("[DEBUG] Value %d not in L1, insert into L1\r\n", L2->buff[i]);

            if (ListInsert(L1, L2->buff[i], L1->last + 1) == false)
            {
                printf("[ERROR] Insert failed during merge\r\n");
                return false;
            }
        }
        else
        {
            printf("[DEBUG] Value %d already exists in L1 at pos %d\r\n", L2->buff[i], ret);
        }
    }

    printf("[DEBUG] Merge success, new L1 last = %d\r\n", L1->last);
    return true;
}
bool ListPurge(pSqList_t L)
{   
    int i = 1;
    int j;
    if (L->last == 0)
    {
        return true;
    }
    while (i <= L->last)
    {
        /* code */
        j = i - 1;
        while (j>=0)
        {
            if (L->buff[i] == L->buff[j])
            {
                ListDeletePos(L, i);
                break;
            }
            else
            {
                j--;
            }
            if (j < 0)
            {
                /* code */
                i++;
            }
            
        }
        
    }
    
    return true;
}