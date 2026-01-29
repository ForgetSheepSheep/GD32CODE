#include "drv_uart/drv_uart.h"
#include <stdio.h>   /* FILE / fputc 用于 printf 重定向 */

/* ======================== 内部函数前置声明 ======================== */
static void uart_gpio_init(void);
static void uart_init(uint32_t baud_rate);

/* ======================== UART 硬件资源描述结构体 ======================== */
/*
 * 该结构体用于描述一组 UART 硬件资源（单实例）
 * 设计目的：
 *  1) 把“硬件相关参数”集中管理，便于移植/改引脚/换串口
 *  2) 后续如需扩展多串口，可改为数组表驱动（类似 LED/KEY 的表）
 */
typedef struct
{
    uint32_t        uart_no;    /* USART 外设编号，例如 USART0 / USART1 */
    rcu_periph_enum rcu_uart;   /* USART 外设时钟枚举，例如 RCU_USART0 */
    rcu_periph_enum rcu_gpio;   /* GPIO 端口时钟枚举，例如 RCU_GPIOA */
    uint32_t        gpio;       /* GPIO 端口基地址，例如 GPIOA */
    uint32_t        tx_pin;     /* TX 引脚，例如 GPIO_PIN_9 */
    uint32_t        rx_pin;     /* RX 引脚，例如 GPIO_PIN_10 */
    uint8_t         irq;        /* 串口中断号，例如 USART0_IRQn */
} uart_hwinfo_t;

/* ======================== UART0 硬件资源配置 ======================== */
/*
 * 当前工程使用 USART0：
 *  TX : PA9  （发送引脚）
 *  RX : PA10 （接收引脚）
 *
 * 注意：
 *  1) USB-TTL 连接需要交叉：USB-TTL_TX -> PA10，USB-TTL_RX -> PA9
 *  2) 必须共地：USB-TTL_GND <-> 板子 GND
 */
static const uart_hwinfo_t g_uart_hwinfo =
{
    USART0,        /* uart_no  : 选择 USART0 外设 */
    RCU_USART0,    /* rcu_uart : 使能 USART0 时钟 */
    RCU_GPIOA,     /* rcu_gpio : 使能 GPIOA 时钟 */
    GPIOA,         /* gpio     : GPIO 端口为 GPIOA */
    GPIO_PIN_9,    /* tx_pin   : TX -> PA9 */
    GPIO_PIN_10,   /* rx_pin   : RX -> PA10 */
    USART0_IRQn    /* irq      : USART0 中断向量 */
};

/**
***********************************************************
* @brief  串口驱动初始化函数
* @param  baud_rate : 串口波特率（如 9600 / 115200）
* @return 无
* @note
*  初始化流程：
*   1) 初始化 GPIO（TX 复用推挽 / RX 上拉输入）
*   2) 初始化 USART 外设参数（8N1、波特率、使能收发、中断等）
***********************************************************
*/
void drv_uart_init(uint32_t baud_rate)
{
    uart_gpio_init();       /* 初始化 UART GPIO */
    uart_init(baud_rate);   /* 初始化 UART 外设 */
}

/**
***********************************************************
* @brief  UART GPIO 初始化
* @param  无
* @return 无
* @note
*  TX：GPIO_MODE_AF_PP（复用推挽输出）
*  RX：GPIO_MODE_IPU（上拉输入）
*
*  原因：
*   - UART 空闲线电平为高，RX 上拉输入可增强抗干扰能力
*   - TX 需要由 USART 外设驱动输出，使用复用推挽
***********************************************************
*/
static void uart_gpio_init(void)
{
    /* 使能 GPIO 端口时钟 */
    rcu_periph_clock_enable(g_uart_hwinfo.rcu_gpio);

    /* TX 引脚：复用推挽输出 */
    gpio_init(g_uart_hwinfo.gpio,
              GPIO_MODE_AF_PP,
              GPIO_OSPEED_10MHZ,
              g_uart_hwinfo.tx_pin);

    /* RX 引脚：上拉输入（UART 空闲态为高电平） */
    gpio_init(g_uart_hwinfo.gpio,
              GPIO_MODE_IPU,
              GPIO_OSPEED_10MHZ,
              g_uart_hwinfo.rx_pin);
}

/**
***********************************************************
* @brief  UART 外设初始化
* @param  baud_rate : 波特率
* @return 无
* @note
*  本配置为：8 数据位、无校验、1 停止位（8N1）
*  并开启：
*   - 接收功能
*   - RBNE 接收中断（收到 1 字节即触发中断）
*   - NVIC 对应中断使能
***********************************************************
*/
static void uart_init(uint32_t baud_rate)
{
    /* 使能 UART 外设时钟 */
    rcu_periph_clock_enable(g_uart_hwinfo.rcu_uart);

    /* 复位 UART 外设，恢复默认状态 */
    usart_deinit(g_uart_hwinfo.uart_no);

    /* 设置字长：8 位数据 */
    usart_word_length_set(g_uart_hwinfo.uart_no, USART_WL_8BIT);

    /* 设置校验方式：无校验 */
    usart_parity_config(g_uart_hwinfo.uart_no, USART_PM_NONE);

    /* 设置停止位：1 位停止位 */
    usart_stop_bit_set(g_uart_hwinfo.uart_no, USART_STB_1BIT);

    /* 设置波特率 */
    usart_baudrate_set(g_uart_hwinfo.uart_no, baud_rate);

    /* 使能发送功能（TEN=1） */
    usart_transmit_config(g_uart_hwinfo.uart_no, USART_TRANSMIT_ENABLE);

    /* 使能接收功能（REN=1） */
    usart_receive_config(g_uart_hwinfo.uart_no, USART_RECEIVE_ENABLE);

    /* 使能接收缓冲非空中断（RBNE 中断） */
    usart_interrupt_enable(g_uart_hwinfo.uart_no, USART_INT_RBNE);

    /* 使能 USART 外设（UEN=1） */
    usart_enable(g_uart_hwinfo.uart_no);

    /* 使能 NVIC 对应中断（抢占优先级=0，响应优先级=0） */
    nvic_irq_enable(g_uart_hwinfo.irq, 0, 0);
}

/**
***********************************************************
* @brief  发送单个字符（阻塞方式）
* @param  ch : 要发送的字符
* @return 无
* @note
*  流程：
*   1) 写入发送数据寄存器
*   2) 等待发送缓冲区空（TBE=1），表示可以发送下一个字节
***********************************************************
*/
void uart_send_char(const char ch)
{
    /* 将数据写入发送数据寄存器 */
    usart_data_transmit(g_uart_hwinfo.uart_no, (uint8_t)ch);

    /* 等待发送缓冲区为空（可以发送下一个字节） */
    while (usart_flag_get(g_uart_hwinfo.uart_no, USART_FLAG_TBE) == RESET)
    {
    }
}

/**
***********************************************************
* @brief  发送字符串（以 '\0' 结尾）
* @param  str : 字符串首地址
* @return 无
* @note
*  逐字节发送，直到遇到字符串结束符 '\0'
***********************************************************
*/
void uart_send_string(const char *str)
{
    if (str == 0)
    {
        return;
    }

    while (*str != '\0')
    {
        uart_send_char(*str);
        str++;
    }
}

/**
***********************************************************
* @brief  printf 输出重定向（使 printf 走串口发送）
* @param  ch : 输出字符
* @param  f  : 文件指针（此处不使用，仅为满足库函数原型）
* @return 输出字符
* @note
*  使用方法：
*   1) 调用 drv_uart_init() 完成串口初始化
*   2) 之后可直接 printf("xxx\r\n");
***********************************************************
*/
int fputc(int ch, FILE *f)
{
    (void)f; /* 避免未使用参数告警 */
    uart_send_char((uint8_t)ch);
    return ch;
}

/* ======================== 接收回调机制 ======================== */
/*
 * 设计目的：
 *  - USART 中断里只负责“取出 1 字节数据”
 *  - 具体协议解析交给上层 APP（通过回调函数实现解耦）
 *
 * 约定：
 *  - callback == NULL ：表示未注册/已注销回调
 *  - 中断里调用前必须判空，避免 HardFault
 */
static void (*callback)(uint8_t) = NULL;

/**
***********************************************************
* @brief  注册/注销串口接收回调
* @param  func : 回调函数指针
*               func != NULL：注册
*               func == NULL：注销
* @return 无
* @note
*  回调函数在 USART 中断上下文执行：
*   - 不要做长时间阻塞操作（如大段 printf、延时）
*   - 推荐只做“入队/置标志”，在主循环 task 中再处理
***********************************************************
*/
void uart_callback(void (*func)(uint8_t))
{
    callback = func;
}

/**
***********************************************************
* @brief  USART0 中断服务函数（接收中断）
* @param  无
* @return 无
* @note
*  触发条件：RBNE=1（接收缓冲非空）
*  处理流程：
*   1) 读取接收数据寄存器获取 1 字节数据（通常读 DR 会清 RBNE）
*   2) 若回调已注册，则把该字节交给回调处理
*   3) 如库要求手动清中断标志，则清 RBNE 标志
***********************************************************
*/
void USART0_IRQHandler(void)
{
    if (usart_interrupt_flag_get(g_uart_hwinfo.uart_no, USART_INT_FLAG_RBNE) != RESET)
    {
        /* 读取 1 字节数据 */
        uint8_t dat = (uint8_t)usart_data_receive(g_uart_hwinfo.uart_no);

        /* 回调判空：允许未注册/注销 */
        if (callback != NULL)
        {
            callback(dat);
        }

        /* 若库要求手动清 RBNE 标志则保留；否则通常读 DR 已自动清 */
        usart_interrupt_flag_clear(g_uart_hwinfo.uart_no, USART_INT_FLAG_RBNE);
    }
}
