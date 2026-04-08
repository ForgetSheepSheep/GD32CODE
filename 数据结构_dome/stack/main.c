#include "stack.h"

int main(void)
{
    int i;
    data_t value;
    sqstack *s;

    /* 创建一个最多保存 5 个元素的顺序栈。 */
    s = StackCreate(5);
    if (s == NULL)
    {
        printf("[ERROR] stack create fail\r\n");
        return -1;
    }

    printf("===== 顺序栈演示开始 =====\r\n");

    /*
     * main 的演示思路：
     * 1. 先连续入栈几个数据，观察栈顶变化。
     * 2. 故意多入栈一次，验证满栈判断是否生效。
     * 3. 读取栈顶元素，但不删除它。
     * 4. 循环出栈，观察“后进先出”的特点。
     * 5. 再次出栈，验证空栈判断。
     */
    for (i = 0; i < 5; i++)
    {
        value = (i + 1) * 10;
        if (StackPush(s, value) == 0)
        {
            printf("入栈成功: %d\r\n", value);
        }
    }

    if (StackPush(s, 60) == -1)
    {
        printf("额外入栈 60 失败，说明栈满判断正常\r\n");
    }

    printf("当前栈顶元素: %d\r\n", StackTop(s));

    printf("开始出栈:\r\n");
    while (!StackEmpty(s))
    {
        printf("出栈元素: %d\r\n", StackPop(s));
    }

    if (StackPop(s) == -1)
    {
        printf("空栈不能继续出栈\r\n");
    }

    if (StackClear(s) == 0)
    {
        printf("调试信息: 栈已清空\r\n");
    }

    if (StackFree(s) == 0)
    {
        printf("调试信息: 栈已释放\r\n");
        s = NULL;
    }

    return 0;
}
