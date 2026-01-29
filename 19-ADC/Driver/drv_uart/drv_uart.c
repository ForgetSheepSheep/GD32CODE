/**
 ******************************************************************************
 * @file    drv_uart.c
 * @brief   UART 驱动源文件
 * @details 用于 GD32F303 的 UART 模块驱动，支持 printf 重定向和接收回调
 ******************************************************************************
 */

#include "drv_uart/drv_uart.h"
#include <stdio.h>   /* FILE / fputc 用于 printf 重定向 */

/* ======================== 内部函数前置声明 ======================== */
static void uart_gpio_init(void);
static void uart_init(uint32_t baud_rate);

/* ======================== UART 硬件参数配置结构体 ======================== */
/*
 * 该结构体定义一个 UART 硬件参数配置模板
 * 目的：
 *  1) 选择硬件参数：包括串口编号、时钟、GPIO等参数
 *  2) 方便扩展其他串口，可改为参数列表或宏定义，适配 LED/KEY 等改动
 */
typedef struct
{
    uint32_t        uart_no;    /* USART 编号，例如 USART0 / USART1 */
    rcu_periph_enum rcu_uart;   /* USART 时钟枚举，例如 RCU_USART0 */
    rcu_periph_enum rcu_gpio;   /* GPIO 端口时钟枚举，例如 RCU_GPIOA */
    uint32_t        gpio;       /* GPIO 端口号，例如 GPIOA */
    uint32_t        tx_pin;     /* TX 引脚，例如 GPIO_PIN_9 */
    uint32_t        rx_pin;     /* RX 引脚，例如 GPIO_PIN_10 */
    uint8_t         irq;        /* 中断号，例如 USART0_IRQn */
} uart_hwinfo_t;

/* ======================== UART0 硬件参数配置 ======================== */
/*
 * 当前配置使用 USART0
 *  TX : PA9  (发送引脚)
 *  RX : PA10 (接收引脚)
 *
 * 注意：
 *  1) USB-TTL 接线需要接对：USB-TTL_TX -> PA10，USB-TTL_RX -> PA9
 *  2) 必须共地：USB-TTL_GND <-> 开发板 GND
 */
static const uart_hwinfo_t g_uart_hwinfo =
{
    USART0,        /* uart_no  : 选择 USART0 编号 */
    RCU_USART0,    /* rcu_uart : 使能 USART0 时钟 */
    RCU_GPIOA,     /* rcu_gpio : 使能 GPIOA 时钟 */
    GPIOA,         /* gpio     : GPIO 端口为 GPIOA */
    GPIO_PIN_9,    /* tx_pin   : TX -> PA9 */
    GPIO_PIN_10,   /* rx_pin   : RX -> PA10 */
    USART0_IRQn    /* irq      : USART0 中断号 */
};

/**
 * @brief  串口模块初始化函数
 * @param  baud_rate : 串口波特率，如 9600 / 115200
 * @retval 无
 * @note
 *   初始化流程：
 *    1) 初始化 GPIO：TX（推挽复用）/ RX（上拉输入）
 *    2) 初始化 USART：8N1、波特率、使能收发中断等
 */
void drv_uart_init(uint32_t baud_rate)
{
    uart_gpio_init();       /* 初始化 UART GPIO */
    uart_init(baud_rate);   /* 初始化 UART 配置 */
}

/**
 * @brief  UART GPIO 初始化
 * @param  无
 * @retval 无
 * @note
 *  TX: GPIO_MODE_AF_PP (推挽复用输出)
 *  RX: GPIO_MODE_IPU (上拉输入)
 *
 *  原因：
 *   - UART 总线空闲时为高电平，RX 引脚上拉可以防止误判
 *   - TX 需要复用到 USART 功能，使用复用功能模式
 */
static void uart_gpio_init(void)
{
    /* 使能 GPIO 端口时钟 */
    rcu_periph_clock_enable(g_uart_hwinfo.rcu_gpio);

    /* TX 引脚：推挽复用输出 */
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
 * @brief  UART 参数初始化
 * @param  baud_rate : 波特率
 * @retval 无
 * @note
 *   参数：8 数据位、无校验、1 停止位（8N1）
 *   中断：
 *   - 仅接收中断
 *   - RBNE 接收中断（收到 1 字节即触发中断）
 *   - NVIC 对应中断使能
 */
static void uart_init(uint32_t baud_rate)
{
    /* 使能 UART 模块时钟 */
    rcu_periph_clock_enable(g_uart_hwinfo.rcu_uart);

    /* 复位 UART 配置，恢复默认状态 */
    usart_deinit(g_uart_hwinfo.uart_no);

    /* 设置字长：8 位数据位 */
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

    /* 使能接收缓冲区非空中断（RBNE 中断） */
    usart_interrupt_enable(g_uart_hwinfo.uart_no, USART_INT_RBNE);

    /* 使能 USART 外设（UEN=1） */
    usart_enable(g_uart_hwinfo.uart_no);

    /* 使能 NVIC 对应中断（抢占优先级=0，响应优先级=0） */
    nvic_irq_enable(g_uart_hwinfo.irq, 0, 0);
}

/**
 * @brief  发送单个字符，阻塞式
 * @param  ch : 要发送的字符
 * @retval 无
 * @note
 *   流程：
 *    1) 写入发送数据寄存器
 *    2) 等待发送缓冲区为空（TBE=1，此时可以发送下一个字节）
 */
void uart_send_char(const char ch)
{
    /* 将字符写入发送数据寄存器 */
    usart_data_transmit(g_uart_hwinfo.uart_no, (uint8_t)ch);

    /* 等待发送缓冲区为空（可以发送下一个字节） */
    while (usart_flag_get(g_uart_hwinfo.uart_no, USART_FLAG_TBE) == RESET)
    {
    }
}

/**
 * @brief  发送字符串，以 '\0' 为结尾
 * @param  str : 字符串首地址
 * @retval 无
 * @note
 *   逐字节发送，直到遇到字符串结尾的 '\0'
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
 * @brief  printf 重定向，使 printf 能通过串口发送
 * @param  ch : 输出字符
 * @param  f  : 文件指针（此处未使用，仅为标准函数原型）
 * @retval 输出字符
 * @note
 *   使用方法：
 *    1) 先调用 drv_uart_init() 完成串口初始化
 *    2) 之后直接 printf("xxx\r\n");
 */
int fputc(int ch, FILE *f)
{
    (void)f; /* 参数未使用，消除警告 */
    uart_send_char((uint8_t)ch);
    return ch;
}

/* ======================== 接收回调函数 ======================== */
/*
 * 设计目的：
 *  - USART 中断中只读取 1 字节数据。
 *  - 具体协议由上层 APP 通过回调函数实现（例如命令解析）
 *
 * 约束条件：
 *  - callback == NULL 表示未注册/注销回调
 *  - 中断处理前必须检查回调是否为空，否则可能 HardFault
 */
static void (*callback)(uint8_t) = NULL;

/**
 * @brief  注册/注销串口接收回调
 * @param  func : 回调函数指针
 *               func != NULL : 注册
 *               func == NULL : 注销
 * @retval 无
 * @note
 *   回调函数在 USART 中断服务程序中执行：
 *   - 注意不要在中断中执行耗时操作，例如 printf 等函数
 *   - 推荐只设置标志位或简单处理，在主循环 task 中再处理
 */
void uart_callback(void (*func)(uint8_t))
{
    callback = func;
}

/**
 * @brief  USART0 中断服务函数（接收中断）
 * @param  无
 * @retval 无
 * @note
 *   触发条件：RBNE=1（接收缓冲区非空）
 *   处理流程：
 *    1) 读取接收数据寄存器，读取 1 字节数据（通过读取 DR 可以自动清除 RBNE）
 *    2) 若回调已注册，则将该字节传给回调函数
 *    3) 不需要手动清除 RBNE 标志，因为读 DR 会自动清除
 */
void USART0_IRQHandler(void)
{
    if (usart_interrupt_flag_get(g_uart_hwinfo.uart_no, USART_INT_FLAG_RBNE) != RESET)
    {
        /* 读取 1 字节数据 */
        uint8_t dat = (uint8_t)usart_data_receive(g_uart_hwinfo.uart_no);

        /* 回调非空则调用（防止未注册/注销） */
        if (callback != NULL)
        {
            callback(dat);
        }

        /* 不需要手动清除 RBNE 标志，因为读取 DR 会自动清除 */
        usart_interrupt_flag_clear(g_uart_hwinfo.uart_no, USART_INT_FLAG_RBNE);
    }
}
