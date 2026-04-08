/**
 ******************************************************************************
 * @file    drv_capture.c
 * @brief   CAPTURE (输入捕获) 驱动源文件
 * @details 用于 GD32F303 的 TIMER 输入捕获驱动，测量 PWM 周期和占空比
 ******************************************************************************
 */

#include "drv_capture/drv_capture.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "gd32f30x.h"

/* ======================== 内部函数声明 ======================== */
static void timer_init(void);
static void gpio_init(void);

/* ======================== 全局变量 ======================== */
/**
 * @brief  捕获周期值的存储单位 us
 * @note   使用 TIMER1_CH0 在中断中更新
 *         g_period_us 表示相邻两个上升沿之间的时间差，即周期 T
 */
static volatile uint32_t g_period_us = 0U;

/**
 * @brief  捕获占空比值的存储单位 us
 * @note   计算脉冲宽度，通道为高电平 Ton 和低电平 Toff 之和
 *         寄存器更新 CH1 会捕获另一半沿的值。当前只使能 CH0 中断，读取 CH1
 *
 * @warning 当前只使用了 CH0 中断，未使用 CH1 中断。
 *          CH1 的配置（INDIRECTTI + FALLING）存在内部超时，GD32 TIMER 的 IC 功能比较复杂。
 *          实际发送 g_pulse_us 会直接超长/超短，该变量暂时不使用，建议后续配置。
 */
static volatile uint32_t g_pulse_us = 0U;

/* ======================== 模块初始化函数 ======================== */
/**
 * @brief  输入捕获模块初始化函数
 * @param  无
 * @retval 无
 * @note
 *   1) GPIO：PA0 配置为输入（对应 TIMER1_CH0）
 *   2) TIMER1：配置输入捕获，分辨率 1us/tick
 *   3) 中断：使能通道上升沿中断（目前只使能 CH0 上升沿中断）
 */
void drv_capture_init(void)
{
    gpio_init();
    timer_init();
}

/* ======================== GPIO 初始化函数 ======================== */
/**
 * @brief  输入捕获 GPIO 初始化函数
 * @param  无
 * @retval 无
 * @note
 *   - PA0 配置为浮空输入
 *   - 输入 PWM 信号连接到 TIMER1_CH0
 */
static void gpio_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);

    gpio_init(GPIOA,
              GPIO_MODE_IN_FLOATING,
              GPIO_OSPEED_MAX,
              GPIO_PIN_0);
}

/* ======================== 定时器初始化函数 ======================== */
/**
 * @brief  TIMER1 输入捕获初始化函数
 * @param  无
 * @retval 无
 * @note
 *   1) TIMER1 时钟：120MHz
 *   2) 预分频：120 - 1 => 1MHz (1us / tick)
 *   3) period：65535，最长可计 65ms，适合常见按键/蜂鸣器/PWM 信号
 *   4) CH0：RISING + DIRECTTI，上升沿直接捕获
 *   5) CH1：FALLING + INDIRECTTI，下降沿捕获（链接到 CH0 的触发）
 *
 * @warning
 *   - INDIRECTTI 的配置文档可能不直观，建议阅读 GPIO 复用映射。
 *   - 若将 CH1 配置为 INDIRECTTI，则 CH1 会捕获来自 TI0 的输入（即 PA0）
 *   - 若为同向沿（例如都上升沿），会漏掉中间的跳变沿（上升+下降）。
 *   - 但实际未使用 CH1 中断，仅配置，需要用 timer_input_capture_switch / slave mode / trigger 等。需要调试。
 */
static void timer_init(void)
{
    rcu_periph_clock_enable(RCU_TIMER1);
    timer_deinit(TIMER1);

    /* ======================== 定时器基础参数配置 ======================== */
    timer_parameter_struct timer_init_para;
    timer_struct_para_init(&timer_init_para);

    timer_init_para.prescaler = 120U - 1U;   /* 1MHz => 1us/tick */
    timer_init_para.period    = 65535U;

    timer_init(TIMER1, &timer_init_para);

    /* ======================== 输入捕获通道参数配置 ======================== */
    timer_ic_parameter_struct ic_init_para;
    timer_channel_input_struct_para_init(&ic_init_para);

    /* -------- CH0：上升沿捕获，直连 TI0 -------- */
    ic_init_para.icpolarity  = TIMER_IC_POLARITY_RISING;
    ic_init_para.icselection = TIMER_IC_SELECTION_DIRECTTI;
    timer_input_capture_config(TIMER1, TIMER_CH_0, &ic_init_para);

#if 0
    /* -------- CH1：下降沿捕获，链接 TI0（来自通道映射到 TI0） -------- */
    ic_init_para.icpolarity  = TIMER_IC_POLARITY_FALLING;
    ic_init_para.icselection = TIMER_IC_SELECTION_INDIRECTTI;
    timer_input_capture_config(TIMER1, TIMER_CH_1, &ic_init_para);
#else
#endif

    timer_input_pwm_capture_config(TIMER1, TIMER_CH_0, &ic_init_para);

    /* ======================== 中断配置 ======================== */
    /* 清除 CH0 捕获中断标志位 */
    timer_interrupt_flag_clear(TIMER1, TIMER_INT_FLAG_CH0);

    /**
     * @note 当前设置是 TIMER_INT_FLAG_CH0 的 enable
     *       但 GD32 手册中 TIMER_INT_CH0 的 enable/FLAG 与寄存器不对应。
     *       实际发送中断不会触发，该函数可能是占位符或枚举错误。
     */
    timer_interrupt_enable(TIMER1, TIMER_INT_FLAG_CH0);

    /* NVIC 使能 TIMER1 中断 */
    nvic_irq_enable(TIMER1_IRQn, 0, 0);

    /* 使能定时器 */
    timer_enable(TIMER1);
}

/* ======================== 输入捕获中断服务函数 ======================== */
/**
 * @brief  TIMER1 输入捕获中断处理函数（仅处理 CH0）
 * @param  无
 * @retval 无
 * @note
 *   1) 在 CH0 上升沿触发时进入
 *   2) 读取 CH0 捕获值 => 周期计数值 (us)
 *   3) 读取 CH1 捕获值 => 脉宽计数值，但当前 CH1 未实际触发/使用
 *   4) 将计数器清零，为下次捕获做准备
 */
void TIMER1_IRQHandler(void)
{
    if (timer_interrupt_flag_get(TIMER1, TIMER_INT_FLAG_CH0) == SET)
    {
        /* 周期：直接捕获的上升沿间隔 */
        g_period_us = (uint32_t)timer_channel_capture_value_register_read(TIMER1, TIMER_CH_0) + 1U;

        /* 另一通道：捕获脉冲宽度，但 CH1 未使能中断或配置可能有问题 */
        g_pulse_us  = (uint32_t)timer_channel_capture_value_register_read(TIMER1, TIMER_CH_1) + 1U;

        /* 清零计数器，等待下一次捕获 */
        timer_counter_value_config(TIMER1, 0U);

        /* 清除 CH0 捕获中断标志位 */
        timer_interrupt_flag_clear(TIMER1, TIMER_INT_FLAG_CH0);
    }
}

/* ======================== 测试接口函数 ======================== */
/**
 * @brief  输入捕获测试函数
 * @param  无
 * @retval 无
 * @note
 *   - 打印周期和占空比（单位 us）
 *   - 注意：g_pulse_us 未经过实际验证，需要后续调试
 */
void capture_test(void)
{
    /* 暂时注释掉打印，待调试 */
    /* printf("period=%lu us, pulse=%lu us\r\n", */
    /*        (unsigned long)g_period_us, */
    /*        (unsigned long)g_pulse_us); */
}
