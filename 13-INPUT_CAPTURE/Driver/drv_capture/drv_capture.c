#include "drv_capture/drv_capture.h"

/* ======================== 静态函数声明 ======================== */
static void timerinit(void);
static void GPIO_Init(void);

/* ======================== 全局变量 ======================== */
/**
 * @brief  输入捕获计数值
 * @note   单位：us（当前定时器配置下）
 */
static uint32_t g_ic_val = 0;

/* ======================== 对外接口函数 ======================== */
/**
***********************************************************
* @brief  输入捕获模块初始化
* @note
* 1) 使用 TIMER1_CH0 进行输入捕获
* 2) 捕获上升沿，测量信号周期
* 3) 定时器计数单位为 1us
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
* @note
* PA0 配置为浮空输入
* 对应 TIMER1_CH0 输入捕获通道
***********************************************************
*/
static void GPIO_Init(void)
{
    /* 使能 GPIOA 时钟 */
    rcu_periph_clock_enable(RCU_GPIOA);

    /* PA0 配置为浮空输入，用于捕获外部信号 */
    gpio_init(GPIOA,
              GPIO_MODE_IN_FLOATING,
              GPIO_OSPEED_MAX,
              GPIO_PIN_0);
}

/* ======================== 定时器初始化 ======================== */
/**
***********************************************************
* @brief  TIMER1 输入捕获初始化
* @note
* 1) 定时器时钟：120MHz
* 2) 预分频：120 - 1 → 1MHz（1us / tick）
* 3) 自动重装载值：65535（最大周期约 65ms）
* 4) 捕获方式：CH0 上升沿捕获
***********************************************************
*/
static void timerinit(void)
{
    /* 使能 TIMER1 外设时钟 */
    rcu_periph_clock_enable(RCU_TIMER1);

    /* 复位 TIMER1 到默认状态 */
    timer_deinit(TIMER1);

    /* ------------------------ 基本计数参数 ------------------------ */
    timer_parameter_struct timerInitPara;
    timer_struct_para_init(&timerInitPara);

    /* 120MHz / 120 = 1MHz → 1us / tick */
    timerInitPara.prescaler = 120 - 1;

    /* 最大计数周期，避免溢出 */
    timerInitPara.period = 65535;

    /* 初始化 TIMER1 */
    timer_init(TIMER1, &timerInitPara);

    /* ------------------------ 输入捕获参数配置 ------------------------ */
    timer_ic_parameter_struct icInitPara;
    timer_channel_input_struct_para_init(&icInitPara);

    /* 上升沿捕获 */
    icInitPara.icpolarity  = TIMER_IC_POLARITY_RISING;

    /* 直接连接到 TI0（TIMER1_CH0） */
    icInitPara.icselection = TIMER_IC_SELECTION_DIRECTTI;

    /* 配置 TIMER1_CH0 为输入捕获 */
    timer_input_capture_config(TIMER1, TIMER_CH_0, &icInitPara);

    /* ------------------------ 中断配置 ------------------------ */
    /* 清除通道 0 捕获中断标志 */
    timer_interrupt_flag_clear(TIMER1, TIMER_INT_FLAG_CH0);

    /* 使能通道 0 捕获中断 */
    timer_interrupt_enable(TIMER1, TIMER_INT_FLAG_CH0);

    /* 使能 TIMER1 中断 */
    nvic_irq_enable(TIMER1_IRQn, 0, 0);

    /* 启动定时器 */
    timer_enable(TIMER1);
}

/* ======================== 输入捕获中断服务函数 ======================== */
/**
***********************************************************
* @brief  TIMER1 输入捕获中断处理函数
* @note
* 1) 每次捕获到上升沿进入中断
* 2) 读取捕获寄存器值（单位 us）
* 3) 复位计数器，用于测量下一个周期
***********************************************************
*/
void TIMER1_IRQHandler(void)
{
    if (timer_interrupt_flag_get(TIMER1, TIMER_INT_FLAG_CH0) == SET)
    {
        /* 读取捕获值（单位：us） */
        g_ic_val = timer_channel_capture_value_register_read(TIMER1, TIMER_CH_0) + 1;

        /* 清零计数器，重新开始计数 */
        timer_counter_value_config(TIMER1, 0);

        /* 清除捕获中断标志 */
        timer_interrupt_flag_clear(TIMER1, TIMER_INT_FLAG_CH0);
    }
}

/* ======================== 测试接口 ======================== */
/**
***********************************************************
* @brief  输入捕获测试函数
* @note
* 打印测得的信号周期（单位 us）
***********************************************************
*/
void capture_test(void)
{
    printf("period is %d us\r\n", g_ic_val);
}
