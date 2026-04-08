#include <stdio.h>
#include "sqlist.h"

void InsertTest(void);
void DeleteTest(void);
void MergeTest(void);
void PurgeTest(void);
int main(int argc, const char *argv[])
{
    // printf("***************** test insert *****************\r\n");
    // InsertTest();

    // printf("\r\n***************** test delete *****************\r\n");
    // DeleteTest();

    printf("\r\n***************** test merge ******************\r\n");
    // MergeTest();
     printf("\r\n***************** test purge ******************\r\n");
     PurgeTest();
    return 0;
}

void InsertTest(void)
{
    pSqList_t L;

    printf("[TEST] Enter InsertTest\r\n");

    L = ListCreate();
    if (L == NULL)
    {
        printf("[TEST] list create failed\r\n");
        return;
    }

    printf("[TEST] list addr: %p\r\n", L);

    ListInsert(L, 10, 0);
    ListInsert(L, 1, 1);
    ListInsert(L, 2, 2);
    ListInsert(L, 3, 3);
    ListInsert(L, 5, 4);
    ListInsert(L, 2, 3);

    printf("[TEST] After insert:\r\n");
    ListShow(L);

    printf("[TEST] length = %d\r\n", ListLength(L));

    ListDelete(L);
    L = NULL;

    printf("[TEST] after delete, L = %p\r\n", L);
}

void DeleteTest(void)
{
    pSqList_t L;

    printf("[TEST] Enter DeleteTest\r\n");

    L = ListCreate();
    if (L == NULL)
    {
        printf("[TEST] list create failed\r\n");
        return;
    }

    printf("[TEST] list addr: %p\r\n", L);

    ListInsert(L, 1, 0);
    ListInsert(L, 2, 1);
    ListInsert(L, 3, 2);
    ListInsert(L, 4, 3);
    ListInsert(L, 4, 4);
    ListInsert(L, 5, 5);
    ListInsert(L, 6, 6);

    printf("[TEST] Before delete:\r\n");
    ListShow(L);

    printf("[TEST] length = %d\r\n", ListLength(L));

    printf("[TEST] Delete pos = 3\r\n");
    ListDeletePos(L, 3);

    printf("[TEST] After delete:\r\n");
    ListShow(L);

    ListDelete(L);
    L = NULL;

    printf("[TEST] after delete list, L = %p\r\n", L);
}

void MergeTest(void)
{
    pSqList_t L;
    pSqList_t S;

    printf("[TEST] Enter MergeTest\r\n");

    L = ListCreate();
    if (L == NULL)
    {
        printf("[TEST] L create failed\r\n");
        return;
    }

    S = ListCreate();
    if (S == NULL)
    {
        printf("[TEST] S create failed\r\n");
        ListDelete(L);
        L = NULL;
        return;
    }

    printf("[TEST] L addr: %p\r\n", L);
    printf("[TEST] S addr: %p\r\n", S);

    ListInsert(S, 1, 0);
    ListInsert(S, 2, 1);
    ListInsert(S, 3, 2);
    ListInsert(S, 7, 3);
    ListInsert(S, 8, 4);
    ListInsert(S, 54, 5);

    printf("[TEST] S before merge:\r\n");
    ListShow(S);

    if (ListMerge(L, S) == true)
    {
        printf("[TEST] L after merge:\r\n");
        ListShow(L);
    }
    else
    {
        printf("[TEST] merge failed\r\n");
    }

    ListDelete(L);
    L = NULL;

    ListDelete(S);
    S = NULL;

    printf("[TEST] after delete, L = %p, S = %p\r\n", L, S);
}
void PurgeTest(void)
{
    pSqList_t L;

    printf("[TEST] Enter DeleteTest\r\n");

    L = ListCreate();
    if (L == NULL)
    {
        printf("[TEST] list create failed\r\n");
        return;
    }

    printf("[TEST] list addr: %p\r\n", L);

    ListInsert(L, 1, 0);
    ListInsert(L, 2, 1);
    ListInsert(L, 3, 2);
    ListInsert(L, 4, 3);
    ListInsert(L, 4, 4);
    ListInsert(L, 5, 5);
    ListInsert(L, 6, 6);

    printf("[TEST] Before delete:\r\n");
    ListShow(L);

    printf("[TEST] length = %d\r\n", ListLength(L));
    ListPurge(L);
    ListShow(L);


}