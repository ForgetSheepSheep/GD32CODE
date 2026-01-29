/**
 ******************************************************************************
 * @file    drv_pwm.c
 * @brief   PWM 驱动源文件
 * @details 用于 GD32F303 的 PWM 输出驱动，配置 TIMER0_CH0 输出 PWM 信号
 ******************************************************************************
 */

#include "drv_pwm/drv_pwm.h"
#include "drv_led/drv_led.h"
#include "sys_dwt_delay/delay.h"

/* ======================== 内部函数声明 ======================== */
static void gpio_init(void);
static void timer_init(void);

/**
 * @brief  PWM 模块初始化函数
 * @param  无
 * @retval 无
 */
void drv_pwm_init(void)
{
    gpio_init();
    timer_init();
}

/**
 * @brief  PWM GPIO 初始化函数
 * @note
 *   配置 PA8 为复用推挽输出，对应 TIMER0_CH0
 * @param  无
 * @retval 无
 */
static void gpio_init(void)
{
    /* 使能 GPIOA 时钟 */
    rcu_periph_clock_enable(RCU_GPIOA);

    /* PA8 配置为复用推挽输出，确认 PA8 对应 TIMER0_CH0 的 AF 映射正确 */
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_MAX, GPIO_PIN_8);
}

/**
 * @brief  PWM 定时器初始化函数
 * @note
 *   配置 TIMER0 生成 PWM 信号：
 *   - 时钟：120MHz / 120 = 1MHz
 *   - 周期：500us (2kHz)
 *   - 输出通道：CH0
 *   - 初始占空比：50% (250/500)
 * @param  无
 * @retval 无
 */
static void timer_init(void)
{
    /* 使能 TIMER0 时钟 */
    rcu_periph_clock_enable(RCU_TIMER0);

    /* 复位 TIMER0 */
    timer_deinit(TIMER0);

    /* ======================== 定时器基础参数配置 ======================== */
    timer_parameter_struct timer_init_para;
    timer_struct_para_init(&timer_init_para);

    /* 120MHz / (120) = 1MHz -> 1us/tick */
    timer_init_para.prescaler = PWM_PRESCALER - 1;

    /* 周期 500us -> 2kHz */
    timer_init_para.period = PWM_PERIOD - 1;

    timer_init(TIMER0, &timer_init_para);

    /* ======================== PWM 通道参数配置 ======================== */
    timer_oc_parameter_struct oc_para;
    timer_channel_output_struct_para_init(&oc_para);

    /* 使能通道 + 输出极性（强制为高） */
    oc_para.outputstate  = TIMER_CCX_ENABLE;            /* 通道输出使能 */
    oc_para.ocpolarity   = TIMER_OC_POLARITY_HIGH;      /* 高电平有效 */
    oc_para.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;     /* 空闲电平（可选） */

    /* 将结构体写入通道输出控制寄存器 */
    timer_channel_output_config(TIMER0, TIMER_CH_0, &oc_para);

    /* PWM 模式 */
    timer_channel_output_mode_config(TIMER0, TIMER_CH_0, TIMER_OC_MODE_PWM0);

    /* CCR占空比 = CCR/(ARR+1)，初始值 50% */
    timer_channel_output_pulse_value_config(TIMER0, TIMER_CH_0, 250);

    /* 缓存器：使能预装载寄存器，更新占空比时不会毛刺 */
    timer_channel_output_shadow_config(TIMER0, TIMER_CH_0, TIMER_OC_SHADOW_ENABLE);

    /* 高级定时器需要使能主输出（TIMER0/TIMER7 需要使能） */
    timer_primary_output_config(TIMER0, ENABLE);

    /* ======================== 中断配置（可选） ======================== */
    /* timer_interrupt_flag_clear(TIMER0, TIMER_INT_FLAG_UP); */
    /* timer_interrupt_enable(TIMER0, TIMER_INT_UP); */
    /* nvic_irq_enable(TIMER0_UP_IRQn, 0, 0); */

    /* 使能定时器 */
    timer_enable(TIMER0);
}

/**
 * @brief  LED PWM 测试函数
 * @note
 *   逐渐增加和减少 PWM 占空比，实现呼吸灯效果
 *   占空比范围：0 ~ 500 (0% ~ 100%)
 * @param  无
 * @retval 无
 */
void led_pwm_test(void)
{
    /* 逐渐增加占空比（变亮） */
    for (uint32_t i = 0; i < 500; i += 10)
    {
        timer_channel_output_pulse_value_config(TIMER0, TIMER_CH_0, i);
        delay_us(1000);
    }

    /* 逐渐减少占空比（变暗） */
    for (uint32_t i = 500; i > 0; i -= 10)
    {
        timer_channel_output_pulse_value_config(TIMER0, TIMER_CH_0, i);
        delay_us(1000);
    }
}
