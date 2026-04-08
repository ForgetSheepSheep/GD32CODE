#ifndef  __SQLIST_H__
#define  __SQLIST_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef int data_t;
#define BUFF_SIZE  128
typedef struct sqlist_t
{
    data_t buff[BUFF_SIZE];
    int last; /* 最后一个元素的下标 */
}SqList_t, *pSqList_t;


/**
 * @brief 创建一个顺序表
 * @return 成功返回顺序表句柄，失败返回 NULL
 */
pSqList_t ListCreate(void);
bool ListDelete(pSqList_t list);
/**
 * @brief 清空顺序表中的所有数据
 * @param list 顺序表句柄
 * @return true 清空成功
 * @return false 清空失败
 */
bool ListClear(pSqList_t list);

/**
 * @brief 判断顺序表是否为空
 * @param list 顺序表句柄
 * @return true 顺序表为空
 * @return false 顺序表非空
 */
bool ListEmpty(pSqList_t list);

/**
 * @brief 获取顺序表当前长度
 * @param list 顺序表句柄
 * @return 顺序表中元素的个数
 */
uint8_t ListLength(pSqList_t list);

/**
 * @brief 查找顺序表中是否存在指定元素
 * @param list 顺序表句柄
 * @param value 要查找的元素值
 * @return true 表中存在该元素
 * @return false 表中不存在该元素
 */
int ListLocate(pSqList_t list, data_t value);

/**
 * @brief 在顺序表指定位置插入元素
 * @param list 顺序表句柄
 * @param value 要插入的元素值
 * @param pos 插入位置
 * @return true 插入成功
 * @return false 插入失败
 */
bool ListInsert(pSqList_t list, data_t value, uint8_t pos);

bool ListShow(pSqList_t list);

bool ListDeletePos(pSqList_t list, uint8_t pos);
bool ListMerge(pSqList_t L1, pSqList_t L2);
bool ListPurge(pSqList_t L);

#endif /* __SQLIST_H__ */
