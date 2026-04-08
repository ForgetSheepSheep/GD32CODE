#include "task.h"
#include <stdbool.h>
#include <stdint.h>
#include "drv_dgt/drv_dgt.h"
#include "app_uart/app_uart.h"
#include "sys_tick/sys_tick.h"
#include "app_key/app_key.h"
#include "drv_adc_dma/drv_adc_dma.h"
#include "app_sensor/app_sensor.h"
#include "modbus_app.h"
#include "drv_led/drv_led.h"
/* ����ص����޲��޷� */
typedef void (*task_cb_t)(void);

/* SysTick 1ms ���ӣ������������������ж������ĵ��ã� */
static void task_tick_hook(void);
static void test_led_task(void);

/* tick ������������������ */
typedef struct
{
    volatile bool is_ready;    /* ���ڱ�־��ISR ��λ��task_loop ִ�к����� */
    uint64_t      ticks_left;  /* ʣ�� tick������ʱ�� */
    uint64_t      period_ticks;/* ���� tick����װ��ֵ�� */
    task_cb_t     cb;          /* ����ص�����ѭ�������ĵ��ã� */
} task_t;

/* ��������ڴ˴�������������
 * ����period_ticks=100���� SysTick=1ms���� 100ms ִ��һ�� app_uart_task
 */
static task_t g_task_table[] =
{
    { false, 100   , 100  , app_uart_task 	},
	{ false, 5     , 5    , app_key_task    },
	{ false, 100   , 100  , rtc_task  		},
	{ false, 1000  , 1000 , feed_dog 		},
	{ false, 1000  , 1000 , app_sensor_printf_task},
	{ false, 500   , 500  , app_sensor_task },
	{ false, 100   , 100  , ModbusTask     },
	{ false, 500   , 500  , test_led_task   },
};

#define TASK_COUNT   ((uint8_t)(sizeof(g_task_table) / sizeof(g_task_table[0])))

/**
 * @brief  ����ģ���ʼ��
 * @note   ע�� 1ms tick ���ӣ����������������
 */
void task_init(void)
{
    sys_callback(task_tick_hook);
}

/**
 * @brief  ������ѯִ�У����� main while(1) �з������ã�
 * @note   ������������ѭ��ִ�У��������ж����ܺ�ʱ�߼�
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
 * @brief  1ms tick ���ӣ��ж������ĵ��ã�
 * @note   ������������λ�����ڴ˴�ֱ��ִ������ص�
 */
static void task_tick_hook(void)
{
    for (uint8_t i = 0; i < TASK_COUNT; i++)
    {
        /* ����Ϊ 0 ��Ϊ�������� */
        if (g_task_table[i].period_ticks == 0)
        {
            continue;
        }

        /* ����ʱ�ݼ������� 0 ���� */
        if (g_task_table[i].ticks_left > 0)
        {
            g_task_table[i].ticks_left--;
        }

        /* ���ڣ���λ����װ */
        if (g_task_table[i].ticks_left == 0)
        {
            g_task_table[i].is_ready  = true;
            g_task_table[i].ticks_left = g_task_table[i].period_ticks;
        }
    }
}

/**
 * @brief  LED测试任务
 * @note   每500ms切换一次LED1状态，测试LED控制
 */
static void test_led_task(void)
{
    /* 切换LED1状态 */
    drv_led_toggle(LED1);
}
