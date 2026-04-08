#include "drv_capture/drv_capture.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "gd32f30x.h"

/* ======================== 静态函数声明 ======================== */
static void timerinit(void);
static void GPIO_Init(void);

/* ======================== 全局变量 ======================== */
/**
 * @brief  周期测量值（单位：us）
 * @note   在 TIMER1_CH0 捕获中断中更新
 *         若捕获配置为“上升沿”，且你每次中断把计数器清零：
 *         g_period_us 表示“相邻两次上升沿之间的间隔”，即周期 T。
 */
static volatile uint32_t g_period_us = 0U;

/**
 * @brief  脉宽测量值（单位：us）
 * @note   意图：测某个相位（通常为高电平宽度 Ton 或低电平宽度 Toff）
 *         你的代码尝试在 CH1 上做一次捕获，然后在同一次中断里读取 CH1。
 *
 * @warning 目前只使能了 CH0 中断，未使能 CH1 中断；
 *          并且 CH1 的捕获配置（INDIRECTTI + FALLING）依赖外设内部映射，
 *          是否“真的能在 CH1 得到有效捕获值”需要结合 GD32 TIMER 的 IC 架构确认。
 *          若你发现 g_pulse_us 一直异常/不变，这里就是首要排查点。
 */
static volatile uint32_t g_pulse_us = 0U;

/* ======================== 对外接口函数 ======================== */
/**
***********************************************************
* @brief  输入捕获模块初始化
* @param  无
* @return 无
* @note
* 1) GPIO：PA0 配置为输入（对应 TIMER1_CH0）
* 2) TIMER1：输入捕获，计数单位 1us/tick
* 3) 使能捕获中断：当前仅使能 CH0 捕获中断
***********************************************************
*/
void drv_capture_init(void)
{
    GPIO_Init();
    timerinit();
}

/* ======================== GPIO 初始化 ======================== */
/**
***********************************************************
* @brief  输入捕获 GPIO 初始化
* @param  无
* @return 无
* @note
* - PA0 配置为浮空输入
* - 用于输入捕获信号（TIMER1_CH0）
***********************************************************
*/
static void GPIO_Init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);

    gpio_init(GPIOA,
              GPIO_MODE_IN_FLOATING,
              GPIO_OSPEED_MAX,
              GPIO_PIN_0);
}

/* ======================== 定时器初始化 ======================== */
/**
***********************************************************
* @brief  TIMER1 输入捕获初始化
* @param  无
* @return 无
* @note
* 1) TIMER1 时钟：120MHz
* 2) 预分频：120 - 1 => 1MHz（1us / tick）
* 3) period：65535（最大约 65ms，适合一般遥控/低频 PWM）
* 4) CH0：RISING + DIRECTTI（常用于测周期：上升沿到上升沿）
* 5) CH1：FALLING + INDIRECTTI（意图：配合 CH0 测脉宽）
*
* @warning
* - “INDIRECTTI” 不是随便写就能用的，它表示该通道输入来自另一个 TIx。
 *  你这里把 CH1 配为 INDIRECTTI，意味着 CH1 的输入可能来自 TI0（即 PA0）。
 *  这种配置通常用于：同一个引脚同时测上升沿/下降沿（周期 + 脉宽）。
 *  但是否需要额外配置（如 timer_input_capture_switch、slave mode、trigger）取决于库封装。
***********************************************************
*/
static void timerinit(void)
{
    rcu_periph_clock_enable(RCU_TIMER1);
    timer_deinit(TIMER1);

    /* ------------------------ 基本计数参数 ------------------------ */
    timer_parameter_struct timerInitPara;
    timer_struct_para_init(&timerInitPara);

    timerInitPara.prescaler = 120U - 1U;   /* 1MHz => 1us/tick */
    timerInitPara.period    = 65535U;

    timer_init(TIMER1, &timerInitPara);

    /* ------------------------ 输入捕获参数配置 ------------------------ */
    timer_ic_parameter_struct icInitPara;
    timer_channel_input_struct_para_init(&icInitPara);

    /* -------- CH0：上升沿捕获，直接输入 TI0 -------- */
    icInitPara.icpolarity  = TIMER_IC_POLARITY_RISING;
    icInitPara.icselection = TIMER_IC_SELECTION_DIRECTTI;
    timer_input_capture_config(TIMER1, TIMER_CH_0, &icInitPara);
#if 0
    /* -------- CH1：下降沿捕获，间接输入（通常映射到 TI0） -------- */
    icInitPara.icpolarity  = TIMER_IC_POLARITY_FALLING;
    icInitPara.icselection = TIMER_IC_SELECTION_INDIRECTTI;
    timer_input_capture_config(TIMER1, TIMER_CH_1, &icInitPara);
#eles
#endif
	timer_input_pwm_capture_config(TIMER1, TIMER_CH_0, &icInitPara);
    /* ------------------------ 中断配置 ------------------------ */
    /* 清除 CH0 捕获中断标志 */
    timer_interrupt_flag_clear(TIMER1, TIMER_INT_FLAG_CH0);

    /**
     * @note 这里你用的是 TIMER_INT_FLAG_CH0 来 enable。
     *       许多 GD32 例程是用 TIMER_INT_CH0 来 enable（FLAG 用于查询/清除）。
     *       如果你发现中断不进或行为异常，优先核对这个宏是否用对。
     */
    timer_interrupt_enable(TIMER1, TIMER_INT_FLAG_CH0);

    /* NVIC 使能 TIMER1 中断 */
    nvic_irq_enable(TIMER1_IRQn, 0, 0);

    /* 启动定时器 */
    timer_enable(TIMER1);
}

/* ======================== 输入捕获中断服务函数 ======================== */
/**
***********************************************************
* @brief  TIMER1 输入捕获中断处理函数（当前仅处理 CH0）
* @param  无
* @return 无
* @note
* 1) 当 CH0 捕获到上升沿（RISING）时进入
* 2) 读取 CH0 捕获值 => 周期计数（us）
* 3) 读取 CH1 捕获值 => 脉宽计数（us）（前提是 CH1 确实发生过捕获）
* 4) 把计数器清零，用于下一次测量
***********************************************************
*/
void TIMER1_IRQHandler(void)
{
    if (timer_interrupt_flag_get(TIMER1, TIMER_INT_FLAG_CH0) == SET)
    {
        /* 周期：上升沿到上升沿 */
        g_period_us = (uint32_t)timer_channel_capture_value_register_read(TIMER1, TIMER_CH_0) + 1U;

        /* 脉宽：通常希望是上升沿到下降沿 or 下降沿到上升沿（取决于配置） */
        g_pulse_us  = (uint32_t)timer_channel_capture_value_register_read(TIMER1, TIMER_CH_1) + 1U;

        /* 清零计数器：让下一次捕获直接得到新的间隔 */
        timer_counter_value_config(TIMER1, 0U);

        /* 清除 CH0 捕获中断标志 */
        timer_interrupt_flag_clear(TIMER1, TIMER_INT_FLAG_CH0);
    }
}

/* ======================== 测试接口 ======================== */
/**
***********************************************************
* @brief  输入捕获测试函数
* @param  无
* @return 无
* @note
* - 打印周期与脉宽（单位 us）
* - 建议用 %lu 打印 uint32_t（并转 unsigned long），避免格式告警
***********************************************************
*/
void capture_test(void)
{
//    printf("period=%lu us, pulse=%lu us\r\n",
//           (unsigned long)g_period_us,
//           (unsigned long)g_pulse_us);
}