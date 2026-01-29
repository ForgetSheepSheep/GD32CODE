#include "task.h"
#include <stdbool.h>
#include <stdint.h>
#include "drv_dgt/drv_dgt.h"
#include "app_uart/app_uart.h"
#include "sys_tick/sys_tick.h"
#include "app_key/app_key.h"
/* 任务回调：无参无返 */
typedef void (*task_cb_t)(void);

/* SysTick 1ms 钩子：驱动调度器计数（中断上下文调用） */
static void task_tick_hook(void);

/* tick 驱动的周期任务描述 */
typedef struct
{
    volatile bool is_ready;    /* 到期标志：ISR 置位，task_loop 执行后清零 */
    uint64_t      ticks_left;  /* 剩余 tick（倒计时） */
    uint64_t      period_ticks;/* 周期 tick（重装载值） */
    task_cb_t     cb;          /* 任务回调（主循环上下文调用） */
} task_t;

/* 任务表：在此处配置周期任务
 * 例：period_ticks=100，若 SysTick=1ms，则 100ms 执行一次 app_uart_task
 */
static task_t g_task_table[] =
{
    { false, 100, 100, app_uart_task },
	{ false, 5  , 5  , app_key_task  },
	{ false, 100  , 100  , rtc_task  },
	{ false, 1000  , 1000  , feed_dog },
	
};

#define TASK_COUNT   ((uint8_t)(sizeof(g_task_table) / sizeof(g_task_table[0])))

/**
 * @brief  任务模块初始化
 * @note   注册 1ms tick 钩子，用于驱动任务调度
 */
void task_init(void)
{
    sys_callback(task_tick_hook);
}

/**
 * @brief  任务轮询执行（放在 main while(1) 中反复调用）
 * @note   到期任务在主循环执行，避免在中断里跑耗时逻辑
 */
void task_loop(void)
{
    for (uint8_t i = 0; i < TASK_COUNT; i++)
    {
        if (g_task_table[i].is_ready)
        {
            g_task_table[i].is_ready = false;

            if (g_task_table[i].cb)
            {
                g_task_table[i].cb();
            }
        }
    }
}

/**
 * @brief  1ms tick 钩子（中断上下文调用）
 * @note   仅做计数与置位，不在此处直接执行任务回调
 */
static void task_tick_hook(void)
{
    for (uint8_t i = 0; i < TASK_COUNT; i++)
    {
        /* 周期为 0 视为禁用任务 */
        if (g_task_table[i].period_ticks == 0)
        {
            continue;
        }

        /* 倒计时递减，避免 0 下溢 */
        if (g_task_table[i].ticks_left > 0)
        {
            g_task_table[i].ticks_left--;
        }

        /* 到期：置位并重装 */
        if (g_task_table[i].ticks_left == 0)
        {
            g_task_table[i].is_ready  = true;
            g_task_table[i].ticks_left = g_task_table[i].period_ticks;
        }
    }
}
