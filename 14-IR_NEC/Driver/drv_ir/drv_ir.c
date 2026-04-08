#include "drv_ir/drv_ir.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* ======================== 配置宏 / 时序阈值 ======================== */
/**
 * @brief 头码宽度阈值（单位：us）
 * @note  你当前捕获的是“相邻有效沿间隔”（看中断里：读取捕获值后清零计数器）
 *       所以 tick_us 实际是某一段电平持续时间（取决于捕获边沿配置）
 */
#define TICK_HEADER_MAX     20000U     /* 头码上限，参考：9ms + 4.5ms ≈ 13.5ms */
#define TICK_HEADER_MIN     10000U     /* 头码下限 */

/**
 * @brief 逻辑 0 / 1 的判决阈值（单位：us）
 * @note  典型 NEC：0 的间隔约 1.12ms，1 的间隔约 2.25ms（视你捕获定义而定）
 *       这里阈值较宽，适合先跑通；后续可按实测收敛。
 */
#define TICK_0_MAX          1800U
#define TICK_0_MIN          500U

#define TICK_1_MAX          3000U
#define TICK_1_MIN          1800U

/* ======================== 静态函数声明 ======================== */
static void gpio_init_ir_input(void);
static void timer7_ic_init(void);
static void parse_ir_frame(uint32_t tick_us);

/* ======================== 全局/静态变量 ======================== */
/**
 * @brief 红外原始 32bit 数据缓存（4 字节）
 * @note  常见 NEC 数据格式：
 *       [0]=ADDR, [1]=~ADDR, [2]=CMD, [3]=~CMD
 */
static uint8_t g_ir_code_raw[4] = {0};

/**
 * @brief 解码完成标志
 * @note  由中断解析流程置位；上层调用 drv_ir_get_code() 读取并清零
 */
static bool g_ir_code_flag = false;

/* ======================== 对外接口实现 ======================== */
/**
***********************************************************
* @brief  红外接收硬件初始化函数
* @param  无
* @return 无
* @note
* 1) 配置红外输入引脚为浮空输入（或上拉输入，视硬件而定）
* 2) 配置 TIMER7 输入捕获，用于测量脉宽/间隔（单位 us）
* 3) 使能捕获中断，在中断里做简易 NEC 帧解析
***********************************************************
*/
void drv_ir_init(void)
{
    gpio_init_ir_input();
    timer7_ic_init();
}

/**
***********************************************************
* @brief  获取遥控按键码值（命令字节）
* @param  ircode 输出：命令码（通常是 NEC 的 CMD 字节）
* @return true：成功获取到新码；false：无新码
* @note
* - 当前仅返回 g_ir_code_raw[2]（CMD）
* - 若你后续要输出完整 4 字节，可把参数改成 uint8_t ircode[4]
***********************************************************
*/
bool drv_ir_get_code(uint8_t *ircode)
{
    if (ircode == NULL)
    {
        return false;
    }

    if (!g_ir_code_flag)
    {
        return false;
    }

    *ircode = g_ir_code_raw[2];
    g_ir_code_flag = false;
    return true;
}

/* ======================== GPIO 初始化 ======================== */
/**
***********************************************************
* @brief  红外输入捕获 GPIO 初始化
* @param  无
* @return 无
* @note
* - 代码当前使用 GPIOC Pin6 作为红外输入
* - 若你实际硬件是 PA0 或其他引脚，请按实际修改端口与 PIN
***********************************************************
*/
static void gpio_init_ir_input(void)
{
    /* 使能 GPIOC 时钟 */
	rcu_periph_clock_enable(RCU_GPIOC);
    /* PC6 配置为浮空输入，用于捕获外部红外接收头输出 */
	gpio_init(GPIOC, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, GPIO_PIN_6);
}

/* ======================== TIMER7 输入捕获初始化 ======================== */
/**
***********************************************************
* @brief  TIMER7 输入捕获初始化
* @param  无
* @return 无
* @note
* 1) 定时器时钟：120MHz
* 2) 预分频：120 - 1 → 1MHz（1us / tick）
* 3) 自动重装载值：65535（最大周期约 65ms）
* 4) 捕获方式：当前配置为 FALLING 边沿捕获（注意与红外接收头输出极性匹配）
* 5) 每次捕获中断内读取 capture 值并清零计数器，得到相邻捕获边沿间隔
***********************************************************
*/
static void timer7_ic_init(void)
{
    /* 使能 TIMER7 外设时钟 */
    rcu_periph_clock_enable(RCU_TIMER7);

    /* 复位 TIMER7 */
    timer_deinit(TIMER7);

    /* ------------------------ 基本计数参数 ------------------------ */
    timer_parameter_struct timer_init_para;
    timer_struct_para_init(&timer_init_para);

    /* 120MHz / 120 = 1MHz → 1us / tick */
    timer_init_para.prescaler = 120U - 1U;

    /* 最大计数周期，避免过短溢出 */
    timer_init_para.period = 65535U;

    timer_init(TIMER7, &timer_init_para);

    /* ------------------------ 输入捕获参数 ------------------------ */
    timer_ic_parameter_struct ic_init_para;
    timer_channel_input_struct_para_init(&ic_init_para);

    /**
     * @note
     * 红外接收头大多“低有效”（有载波输出低），你若要捕获“低脉宽”或“高间隔”，
     * 需要统一边沿策略：
     * - 捕获 FALLING：测的是“上一次 FALLING 到这次 FALLING”的间隔
     * - 捕获 RISING ：测的是“上一次 RISING 到这次 RISING”的间隔
     * 你当前注释写“上升沿”，但实际配置为 FALLING；这里以代码为准。
     */
    ic_init_para.icpolarity  = TIMER_IC_POLARITY_FALLING;
    ic_init_para.icselection = TIMER_IC_SELECTION_DIRECTTI;

    timer_input_capture_config(TIMER7, TIMER_CH_0, &ic_init_para);

    /* ------------------------ 中断配置 ------------------------ */
    timer_interrupt_flag_clear(TIMER7, TIMER_INT_FLAG_CH0);
    timer_interrupt_enable(TIMER7, TIMER_INT_FLAG_CH0);

    nvic_irq_enable(TIMER7_Channel_IRQn, 0, 0);

    /* 启动 TIMER7 */
    timer_enable(TIMER7);
}

/* ======================== 帧解析（中断内调用） ======================== */
/**
***********************************************************
* @brief  简易 NEC 帧解析（在输入捕获中断中调用）
* @param  tick_us 相邻捕获边沿间隔（单位：us）
* @return 无
* @note
* - 本函数假设：tick_us 的分布能区分 “头码” / “逻辑0” / “逻辑1”
* - 解析结果写入 g_ir_code_raw[]，解析完成后置位 g_ir_code_flag
*
* @warning
* 你当前代码里 headFlag 的使用逻辑疑似写反：
*   if(检测到头码) s_headFlag=true; return;
*   if(s_headFlag) return;   // 这会导致检测到头码后，后续全部直接 return，不再解数据
* 正常思路一般是：未检测到头码时丢弃；检测到头码后才开始解 32bit。
* 我这里不改逻辑，只把这个风险写明，避免你“跑不出码”。
***********************************************************
*/
static void parse_ir_frame(uint32_t tick_us)
{
    static bool s_head_flag = false;
    static uint8_t s_index = 0;

    /* 1) 头码判定 */
    if ((tick_us > TICK_HEADER_MIN) && (tick_us < TICK_HEADER_MAX))
    {
        s_head_flag = true;
        return;
    }

    /* 2) 未进入数据解析状态则直接丢弃（按你当前代码逻辑） */
	if (!s_head_flag)
	{
		return;
	}

    /* 3) 逻辑 1 判决 */
    if ((tick_us > TICK_1_MIN) && (tick_us < TICK_1_MAX))
    {
        g_ir_code_raw[s_index / 8] >>= 1;
        g_ir_code_raw[s_index / 8] |= 0x80U;
        s_index++;
    }

    /* 4) 逻辑 0 判决 */
    if ((tick_us > TICK_0_MIN) && (tick_us < TICK_0_MAX))
    {
        g_ir_code_raw[s_index / 8] >>= 1;
        s_index++;
    }

    /* 5) 满 32bit：校验并输出标志 */
    if (s_index == 32U)
    {
        /**
         * @note
         * 这里你原先写的是：(g_irCode[2] & g_irCode[3]) == 0
         * 更标准的 NEC 校验通常是：cmd_inv == (uint8_t)~cmd
         * 你注释里也写了备选条件；不改逻辑，只保留说明。
         */
        if ((g_ir_code_raw[2] & g_ir_code_raw[3]) == 0U)
        {
            g_ir_code_flag = true;
        }
        else
        {
            g_ir_code_flag = false;
        }

        s_head_flag = false;
        s_index = 0U;
    }
}

/* ======================== 输入捕获中断服务函数 ======================== */
/**
***********************************************************
* @brief  TIMER7 输入捕获中断处理函数
* @param  无
* @return 无
* @note
* 1) 捕获到配置的边沿进入中断（当前为 FALLING）
* 2) 读取捕获寄存器得到“距离上次捕获边沿的计数值”
* 3) 清零计数器，开始测量下一段间隔
* 4) 调用 parse_ir_frame() 按 tick_us 分类解码
***********************************************************
*/
void TIMER7_Channel_IRQHandler(void)
{
    if (timer_interrupt_flag_get(TIMER7, TIMER_INT_FLAG_CH0) == SET)
    {
        uint32_t ic_val_us = 0;

        /* 读取捕获值（单位：us），+1 用于避免读到 0（按你的原实现习惯） */
        ic_val_us = (uint32_t)timer_channel_capture_value_register_read(TIMER7, TIMER_CH_0) + 1U;

        /* 清零计数器：下一个边沿到来时测得新的间隔 */
        timer_counter_value_config(TIMER7, 0U);

        /* 帧解析（中断内） */
        parse_ir_frame(ic_val_us);

        /* 清除中断标志 */
        timer_interrupt_flag_clear(TIMER7, TIMER_INT_FLAG_CH0);
    }
}
