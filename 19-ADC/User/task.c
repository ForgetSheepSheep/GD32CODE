/**
 ******************************************************************************
 * @file    task.c
 * @brief   任务调度系统源文件
 * @details 基于 SysTick 的简单任务调度系统
 ******************************************************************************
 */

#include "task.h"
#include <stdbool.h>
#include <stdint.h>
#include "drv_dgt/drv_dgt.h"
#include "app_uart/app_uart.h"
#include "sys_tick/sys_tick.h"
#include "app_key/app_key.h"

/* 回调函数类型，无返回值，无参数 */
typedef void (*task_cb_t)(void);

/* SysTick 1ms 中断时，调用此函数进行任务调度的勾子 */
static void task_tick_hook(void);

/* tick 任务结构体，用于描述每个任务 */
typedef struct
{
    volatile bool is_ready;    /* 标志位：ISR 设置，task_loop 处理完后清除 */
    uint64_t      ticks_left;  /* 剩余 tick 数，未执行时倒计时 */
    uint64_t      period_ticks;/* 任务执行周期 ticks 数 */
    task_cb_t     cb;          /* 任务回调函数，任务就绪时调用 */
} task_t;

/* 任务表，存储所有任务的定义 */
/* period_ticks=100 => SysTick=1ms，周期 100ms，执行一次 app_uart_task */
static task_t g_task_table[] =
{
    { false, 100, 100, app_uart_task },  /* UART 任务：100ms */
    { false, 5  , 5  , app_key_task  },  /* KEY 任务：5ms */
    { false, 100 , 100 , rtc_task  },    /* RTC 任务：100ms */
    { false, 1000 , 1000 , feed_dog },   /* 喂狗：1000ms */
};

#define TASK_COUNT   ((uint8_t)(sizeof(g_task_table) / sizeof(g_task_table[0])))

/**
 * @brief  任务调度系统初始化函数
 * @note
 *   注册 1ms tick 中断回调，用于任务调度
 * @param  无
 * @retval 无
 */
void task_init(void)
{
    sys_callback(task_tick_hook);
}

/**
 * @brief  任务调度主循环函数
 * @note
 *   在 main while(1) 中调用，遍历任务表并执行就绪的任务
 *   中断和定时函数调用任务
 * @param  无
 * @retval 无
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
 * @brief  1ms tick 中断时调用的勾子函数
 * @note
 *   对任务表进行减计数，当减为 0 时，将任务标记为就绪
 * @param  无
 * @retval 无
 */
static void task_tick_hook(void)
{
    for (uint8_t i = 0; i < TASK_COUNT; i++)
    {
        /* 若周期为 0，则跳过该任务 */
        if (g_task_table[i].period_ticks == 0)
        {
            continue;
        }

        /* 若倒计时未结束，则继续减计数 */
        if (g_task_table[i].ticks_left > 0)
        {
            g_task_table[i].ticks_left--;
        }

        /* 倒计时结束，标记任务为就绪，并重置 ticks_left 为周期值 */
        if (g_task_table[i].ticks_left == 0)
        {
            g_task_table[i].is_ready  = true;
            g_task_table[i].ticks_left = g_task_table[i].period_ticks;
        }
    }
}
