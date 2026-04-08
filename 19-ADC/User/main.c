/**
 ******************************************************************************
 * @file    main.c
 * @brief   主程序文件
 * @details 包含系统初始化、驱动初始化、应用层初始化和主循环
 ******************************************************************************
 */

#include "config.h"     // 系统基础配置，包含中断向量表、系统时钟、外设初始化回调等
#include "task.h"

/******************************** 底层通用模块的 ********************************/
#include "sys_dwt_delay/delay.h"   // 用于 DWT 高精度延时，微秒级延时
#include "sys_tick/sys_tick.h"     // 用于 SysTick 系统滴答定时器，提供运行时间

/******************************** 应用层的模块 ********************************/
#include "app_uart/app_uart.h"     // UART 应用层：协议解析和命令处理

/******************************** 底层驱动模块 ********************************/
#include "drv_led/drv_led.h"       // LED 驱动
#include "drv_key/drv_key.h"       // KEY 驱动
#include "drv_uart/drv_uart.h"     // UART 驱动：串口发送和接收
#include "drv_pwm/drv_pwm.h"
#include "drv_capture/drv_capture.h"
#include "drv_rtc/drv_rtc.h"
#include "drv_dgt/drv_dgt.h"
#include "drv_adc/drv_adc.h"

/**
 * @brief  底层通用模块初始化
 * @note
 *   - 延时初始化：初始化 MCU 内核可用的外设（如 DWT CYCCNT）
 *   - 系统滴答：提供系统运行时间基准
 */
static void sys_init(void)
{
    delay_init();      // 初始化 DWT 延时模块，使能 DWT->CYCCNT
    sys_tick_init();   // 初始化 SysTick 滴答定时器，1ms 中断
}

/**
 * @brief  应用层模块初始化
 * @note
 *   - 注册应用层的回调
 *   - 初始化队列等数据结构
 */
static void app_init(void)
{
    app_uart_init();   // 初始化 UART 应用层：协议解析、队列缓冲
}

/**
 * @brief  底层驱动模块初始化
 * @note
 *   - 初始化所有驱动
 *   - 配置 GPIO、UART 等外设
 */
static void drv_init(void)
{
    drv_led_init();            // 初始化 LED 驱动：点亮/熄灭
    drv_key_init();            // 初始化 KEY 驱动：按键检测、单击/双击/长按
    drv_uart_init(115200);     // 初始化 UART 驱动：波特率 115200
    drv_pwm_init();
    drv_capture_init();
    drv_rtc_init();
    drv_dgt_init();
    drv_adc_init();
}

/**
 * @brief  主函数
 * @note
 *   按照以下顺序初始化：
 *   - 系统基础初始化
 *   - 底层驱动初始化
 *   - 应用层初始化
 *   - 任务系统初始化
 *   - 主循环：任务调度
 */
int main(void)
{
    /* ============= 任务系统初始化 ============= */
    task_init();

    /* ============= 底层通用模块初始化 ============= */
    sys_init();     // 初始化底层通用模块：延时、系统滴答

    /* ============= 底层驱动模块初始化 ============= */
    drv_init();     // 初始化底层驱动：LED、KEY、UART、PWM、RTC、ADC 等

    /* ============= 应用层模块初始化 ============= */
    app_init();     // 初始化应用层：UART 协议解析

    /* ============= 主循环 ============= */
    while (1)
    {
        task_loop();  // 任务调度：轮询执行就绪的任务
        // led_pwm_test();
        // capture_test();
        // adc_test();
    }
}
