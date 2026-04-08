#include "linkstack.h"

linkstack StackCreate()
{
    linkstack s;
    s = (linkstack)malloc(sizeof(listnode));
    if (s == NULL)
    {
        printf("s malloc failed\r\n");
        return NULL;
    }
    s->data = 0;
    s->node = NULL;
    return s;
}
int StackPush(linkstack s, data_t value)
{
    linkstack prve;
    if (s == NULL)
    {
        printf("stack create failed\r\n");
        return -1;
    }
    prve = (linkstack)malloc(sizeof(listnode));
    if (prve == NULL)
    {
        printf("prve malloc failed\r\n");
        return -1;
    }
    prve->data = value;
    prve->node = s->node;
    s->node = prve;
    return 0;
}
data_t StackPop(linkstack s)
{
    linkstack p;
    data_t t;
    p = s->node;
    s->node = p->node;
    t = p->data;
    free(p);
    p = NULL;
    return t;
}
int StackEmpyt(linkstack s)
{
    if (s == NULL)
    {
        printf("stack create failed\r\n");
        return -1;
    }
    if (s->node == NULL)
    {
        printf("stack Empyt\r\n");
        return 1;
    }
    else
    {
        return 0;
    }
}
data_t StackTop(linkstack s)
{
    if (s == NULL)
    {
        printf("stack create failed\r\n");
        return -1;
    }
    return s->node->data;
}
linkstack StackFree(linkstack s)
{
    linkstack prve;
    s = (linkstack)malloc(sizeof(listnode));
    if (s == NULL)
    {
        printf("p malloc failed\r\n");
        return NULL;
    }
    prve = (linkstack)malloc(sizeof(listnode));
    if (prve == NULL)
    {
        printf("prve malloc failed\r\n");
        return -1;
    }
    while (s != NULL)
    {
       prve = s;
       s = s->node;
        free(prve);
    }
    return NULL;
}
