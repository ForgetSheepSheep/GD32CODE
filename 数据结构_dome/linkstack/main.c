#include "linkstack.h"


int main()
{
    linkstack s;
    s = StackCreate();
    if (s == NULL)
    {
        printf("s create fail\r\n");
        return -1;
    }
    StackPush(s, 10);
    StackPush(s, 20);
    StackPush(s, 30);
    StackPush(s, 40);
    StackPush(s, 50);
    while (!StackEmpyt(s))
    {
        printf("stack pop:%d\r\n",StackPop(s));
    }
    
    return 0;
}