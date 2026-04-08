#include <stddef.h>

#include "linklist.h"

static int g_checks = 0;
static int g_failures = 0;

/* 记录单条断言结果，不因单个失败中断整个测试程序。
 * 实现思路：每次断言先累计总次数，表达式为假时再累计失败次数并输出失败位置。
 */
#define EXPECT_TRUE(expr)                                                        \
    do {                                                                         \
        g_checks++;                                                              \
        if (!(expr)) {                                                           \
            g_failures++;                                                        \
            printf("[FAIL] %s:%d: %s\n", __FILE__, __LINE__, #expr);             \
        }                                                                        \
    } while (0)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

typedef void (*test_func_t)(void);

/* 检查链表中的数据序列是否与预期数组一致。
 * 实现思路：从第一个数据结点开始逐个与预期数组比较，最后同时确认链表和数组都已遍历完。
 */
static int CheckSequence(linklist H, const data_t *expected, size_t expected_len)
{
    size_t index = 0;
    linklist node = NULL;

    if (H == NULL) {
        return 0;
    }

    node = H->next;
    while (node != NULL && index < expected_len) {
        if (node->data != expected[index]) {
            return 0;
        }
        node = node->next;
        index++;
    }

    return node == NULL && index == expected_len;
}

/* 使用尾插法快速构造测试链表。
 * 实现思路：先创建头结点，再按数组顺序逐个尾插；中途失败就立即释放已申请的链表。
 */
static linklist BuildListByTail(const data_t *values, size_t len)
{
    size_t index = 0;
    linklist head = ListCreate();

    if (head == NULL) {
        return NULL;
    }

    for (index = 0; index < len; index++) {
        if (ListTailInsert(head, values[index]) != 0) {
            ListDestroy(head);
            return NULL;
        }
    }

    return head;
}

/* 统一执行单个测试用例，并打印测试结果。
 * 实现思路：执行前记录当前失败数，执行后根据失败数是否增加判断该用例是否通过。
 */
static void RunTest(const char *name, test_func_t test)
{
    int failures_before = g_failures;

    printf("==== %s ====\n", name);
    test();

    if (g_failures == failures_before) {
        printf("[PASS] %s\n\n", name);
    } else {
        printf("[FAIL] %s\n\n", name);
    }
}

/* 验证链表创建和销毁的基本行为。
 * 实现思路：先检查创建后的头结点状态，再覆盖正常销毁和空指针销毁两个分支。
 */
static void TestCreateAndDestroy(void)
{
    linklist head = ListCreate();

    EXPECT_TRUE(head != NULL);
    if (head != NULL) {
        EXPECT_TRUE(head->next == NULL);
        EXPECT_TRUE(ListDestroy(head) == 0);
    }

    EXPECT_TRUE(ListDestroy(NULL) == -1);
}

/* 验证头插法插入顺序是否符合预期。
 * 实现思路：按 1、2、3 依次头插，最终链表顺序应变成 3、2、1，并补充空指针分支。
 */
static void TestHeadInsert(void)
{
    const data_t expected[] = {3, 2, 1};
    linklist head = ListCreate();

    EXPECT_TRUE(head != NULL);
    if (head == NULL) {
        return;
    }

    EXPECT_TRUE(ListHeadInsert(head, 1) == 0);
    EXPECT_TRUE(ListHeadInsert(head, 2) == 0);
    EXPECT_TRUE(ListHeadInsert(head, 3) == 0);
    EXPECT_TRUE(CheckSequence(head, expected, sizeof(expected) / sizeof(expected[0])));
    EXPECT_TRUE(ListHeadInsert(NULL, 10) == -1);
    EXPECT_TRUE(ListDestroy(head) == 0);
}

/* 验证尾插法插入顺序是否符合预期。
 * 实现思路：按 1、2、3 依次尾插，最终链表顺序应保持 1、2、3，并补充空指针分支。
 */
static void TestTailInsert(void)
{
    const data_t expected[] = {1, 2, 3};
    linklist head = ListCreate();

    EXPECT_TRUE(head != NULL);
    if (head == NULL) {
        return;
    }

    EXPECT_TRUE(ListTailInsert(head, 1) == 0);
    EXPECT_TRUE(ListTailInsert(head, 2) == 0);
    EXPECT_TRUE(ListTailInsert(head, 3) == 0);
    EXPECT_TRUE(CheckSequence(head, expected, sizeof(expected) / sizeof(expected[0])));
    EXPECT_TRUE(ListTailInsert(NULL, 10) == -1);
    EXPECT_TRUE(ListDestroy(head) == 0);
}

/* 验证按位置取节点时的正常路径和越界行为。
 * 实现思路：分别覆盖取头结点、取有效数据结点、空链表取值、越界取值和非法位置。
 */
static void TestListGet(void)
{
    const data_t values[] = {10, 20, 30};
    linklist head = BuildListByTail(values, sizeof(values) / sizeof(values[0]));
    linklist empty = ListCreate();
    linklist node = NULL;

    EXPECT_TRUE(head != NULL);
    EXPECT_TRUE(empty != NULL);
    if (head == NULL || empty == NULL) {
        if (head != NULL) {
            EXPECT_TRUE(ListDestroy(head) == 0);
        }
        if (empty != NULL) {
            EXPECT_TRUE(ListDestroy(empty) == 0);
        }
        return;
    }

    EXPECT_TRUE(ListGet(head, -1) == head);

    node = ListGet(head, 0);
    EXPECT_TRUE(node != NULL);
    if (node != NULL) {
        EXPECT_TRUE(node->data == 10);
    }

    node = ListGet(head, 1);
    EXPECT_TRUE(node != NULL);
    if (node != NULL) {
        EXPECT_TRUE(node->data == 20);
    }

    node = ListGet(head, 2);
    EXPECT_TRUE(node != NULL);
    if (node != NULL) {
        EXPECT_TRUE(node->data == 30);
    }

    EXPECT_TRUE(ListGet(head, 3) == NULL);
    EXPECT_TRUE(ListGet(empty, -1) == empty);
    EXPECT_TRUE(ListGet(empty, 0) == NULL);
    EXPECT_TRUE(ListGet(head, -2) == NULL);
    EXPECT_TRUE(ListGet(NULL, 0) == NULL);
    EXPECT_TRUE(ListDestroy(head) == 0);
    EXPECT_TRUE(ListDestroy(empty) == 0);
}

/* 验证按位置插入时，头部、中间、尾部和空链表场景都能正确处理。
 * 实现思路：先在已有链表中测试多位置插入，再对空链表插入和非法位置插入做边界校验。
 */
static void TestListInsert(void)
{
    const data_t values[] = {10, 30};
    const data_t expected[] = {5, 10, 20, 30, 40};
    linklist head = BuildListByTail(values, sizeof(values) / sizeof(values[0]));
    linklist empty = ListCreate();
    const data_t expected_single[] = {99};

    EXPECT_TRUE(head != NULL);
    EXPECT_TRUE(empty != NULL);
    if (head == NULL || empty == NULL) {
        if (head != NULL) {
            EXPECT_TRUE(ListDestroy(head) == 0);
        }
        if (empty != NULL) {
            EXPECT_TRUE(ListDestroy(empty) == 0);
        }
        return;
    }

    EXPECT_TRUE(ListInsert(head, 20, 1) == 0);
    EXPECT_TRUE(ListInsert(head, 5, 0) == 0);
    EXPECT_TRUE(ListInsert(head, 40, 4) == 0);
    EXPECT_TRUE(CheckSequence(head, expected, ARRAY_SIZE(expected)));
    EXPECT_TRUE(ListInsert(head, 100, 6) == -1);
    EXPECT_TRUE(ListInsert(head, 100, -1) == -1);

    EXPECT_TRUE(ListInsert(empty, 99, 0) == 0);
    EXPECT_TRUE(CheckSequence(empty, expected_single, ARRAY_SIZE(expected_single)));
    EXPECT_TRUE(ListInsert(empty, 100, 2) == -1);
    EXPECT_TRUE(ListInsert(NULL, 100, 0) == -1);
    EXPECT_TRUE(ListDestroy(head) == 0);
    EXPECT_TRUE(ListDestroy(empty) == 0);
}

/* 验证按位置删除时，首节点、中间节点、尾节点和非法位置都能正确处理。
 * 实现思路：依次删除不同位置的结点并检查剩余序列，最后补充删除失败分支。
 */
static void TestListDelete(void)
{
    const data_t values[] = {10, 20, 30, 40};
    const data_t expected_after_first[] = {20, 30, 40};
    const data_t expected_after_second[] = {20, 40};
    const data_t expected_after_third[] = {20};
    linklist head = BuildListByTail(values, ARRAY_SIZE(values));

    EXPECT_TRUE(head != NULL);
    if (head == NULL) {
        return;
    }

    EXPECT_TRUE(ListDelete(head, 10, 0) == 0);
    EXPECT_TRUE(CheckSequence(head, expected_after_first, ARRAY_SIZE(expected_after_first)));

    EXPECT_TRUE(ListDelete(head, 30, 1) == 0);
    EXPECT_TRUE(CheckSequence(head, expected_after_second, ARRAY_SIZE(expected_after_second)));

    EXPECT_TRUE(ListDelete(head, 40, 1) == 0);
    EXPECT_TRUE(CheckSequence(head, expected_after_third, ARRAY_SIZE(expected_after_third)));

    EXPECT_TRUE(ListDelete(head, 20, 0) == 0);
    EXPECT_TRUE(CheckSequence(head, NULL, 0));

    EXPECT_TRUE(ListDelete(head, 0, 0) == -1);
    EXPECT_TRUE(ListDelete(head, 0, -1) == -1);
    EXPECT_TRUE(ListDelete(NULL, 0, 0) == -1);
    EXPECT_TRUE(ListDestroy(head) == 0);
}

/* 验证整表释放后返回值是否符合预期。
 * 实现思路：分别释放非空链表和空链表头结点，确认接口返回空指针，便于调用者回收句柄。
 */
static void TestListFree(void)
{
    const data_t values[] = {1, 2, 3};
    linklist head = BuildListByTail(values, ARRAY_SIZE(values));
    linklist empty = ListCreate();

    EXPECT_TRUE(head != NULL);
    EXPECT_TRUE(empty != NULL);
    if (head == NULL || empty == NULL) {
        if (head != NULL) {
            head = ListFree(head);
            EXPECT_TRUE(head == NULL);
        }
        if (empty != NULL) {
            empty = ListFree(empty);
            EXPECT_TRUE(empty == NULL);
        }
        return;
    }

    head = ListFree(head);
    EXPECT_TRUE(head == NULL);

    empty = ListFree(empty);
    EXPECT_TRUE(empty == NULL);

    EXPECT_TRUE(ListFree(NULL) == NULL);
}

/* 验证打印接口在非空链表、空链表和空指针下的返回值。
 * 实现思路：分别调用非空链表打印、空链表打印和空指针打印，检查输出路径和返回值。
 */
static void TestListShow(void)
{
    const data_t values[] = {7, 8, 9};
    linklist head = BuildListByTail(values, sizeof(values) / sizeof(values[0]));
    linklist empty = ListCreate();

    EXPECT_TRUE(head != NULL);
    EXPECT_TRUE(empty != NULL);
    if (head == NULL || empty == NULL) {
        if (head != NULL) {
            EXPECT_TRUE(ListDestroy(head) == 0);
        }
        if (empty != NULL) {
            EXPECT_TRUE(ListDestroy(empty) == 0);
        }
        return;
    }

    printf("当前链表应输出：7 8 9\n");
    EXPECT_TRUE(ListShow(head) == 0);
    printf("空链表应只输出换行\n");
    EXPECT_TRUE(ListShow(empty) == 0);
    EXPECT_TRUE(ListShow(NULL) == -1);
    EXPECT_TRUE(ListDestroy(head) == 0);
    EXPECT_TRUE(ListDestroy(empty) == 0);
}

int main(void)
{
    printf("链表自动化测试开始\n\n");

    RunTest("CreateAndDestroy", TestCreateAndDestroy);
    RunTest("HeadInsert", TestHeadInsert);
    RunTest("TailInsert", TestTailInsert);
    RunTest("ListGet", TestListGet);
    RunTest("ListInsert", TestListInsert);
    RunTest("ListDelete", TestListDelete);
    RunTest("ListFree", TestListFree);
    RunTest("ListShow", TestListShow);

    printf("断言总数: %d\n", g_checks);
    printf("失败数量: %d\n", g_failures);

    if (g_failures == 0) {
        printf("全部测试通过。\n");
        return 0;
    }

    printf("存在测试失败。\n");
    return 1;
}
