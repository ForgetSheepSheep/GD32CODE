#include "drv_uart/drv_uart.h"
#include "drv_led/drv_led.h"

/* ======================== 协议/缓存相关宏 ======================== */
#define RX_BUF_SIZE         20          /* DMA 接收缓冲区大小（字节） */
#define DATA_HEAD_ONE       0x55        /* 帧头 1 */
#define DATA_HEAD_TOW       0xAA        /* 帧头 2（注意：TOW 是拼写，保持原代码） */

#define CTRL_DATA_LEN       3           /* 控制数据长度：示例为 led_num(1) + led_state(1) + ...（按协议定义） */
#define PACKET_DATA_LEN     (CTRL_DATA_LEN + 4) /* 帧总长度：HEAD1(1)+HEAD2(1)+LEN(1)+FUNC(1)+CTRL_DATA(3)+XOR(1) 这里按你工程定义 */
#define FUNC_DATA_IDX       3           /* FUNC 字段在帧内索引：0:55 1:AA 2:LEN 3:FUNC ... */

#define LED_CTRL_CODE       0x06        /* 功能码：LED 控制 */

/* ======================== 内部函数前置声明 ======================== */
static void uart_gpio_init(void);               /* UART GPIO 初始化 */
static void uart_init(uint32_t baud_rate);      /* UART 外设参数初始化 */
static void uart_dma_int(void);                /* UART RX DMA 初始化 */

/* ======================== 接收状态变量 ======================== */
static bool     g_rc_succeed_flag = false;      /* 一帧接收完成标志（由 IDLE 中断置位） */
static uint8_t  g_rc_buf[RX_BUF_SIZE];          /* DMA 接收缓冲区 */

/* ======================== UART 硬件资源描述结构体 ======================== */
/*
 * 该结构体用于描述一组 UART 硬件资源（单实例）
 * 目的：
 *  1) 把硬件相关参数集中管理，便于移植/改引脚/换串口
 *  2) 后续扩展多串口时，可改成数组/表驱动
 */
typedef struct
{
    uint32_t         uart_no;   /* USART 外设编号，例如 USART0 / USART1 */
    rcu_periph_enum  rcu_uart;  /* USART 外设时钟枚举，例如 RCU_USART0 */
    rcu_periph_enum  rcu_gpio;  /* GPIO 端口时钟枚举，例如 RCU_GPIOA */
    uint32_t         gpio;      /* GPIO 端口基地址，例如 GPIOA */
    uint32_t         tx_pin;    /* TX 引脚，例如 GPIO_PIN_9 */
    uint32_t         rx_pin;    /* RX 引脚，例如 GPIO_PIN_10 */
    uint8_t          irq;       /* 串口中断号，例如 USART0_IRQn */

    uint32_t         dma_num;   /* DMA 控制器编号，例如 DMA0 */
    rcu_periph_enum  rcu_dma;   /* DMA 时钟枚举，例如 RCU_DMA0 */
    dma_channel_enum dam_ch;    /* DMA 通道枚举，例如 DMA_CH4（注意：原变量名 dam_ch） */
} uart_hwinfo_t;

/* ======================== UART0 硬件资源配置 ======================== */
/*
 * 当前工程使用 USART0：
 *  TX : PA9  （发送）
 *  RX : PA10 （接收）
 *
 * 注意：
 *  1) USB-TTL 连接需要交叉：USB-TTL_TX -> PA10，USB-TTL_RX -> PA9
 *  2) 必须共地：USB-TTL_GND <-> 板子 GND
 */
static const uart_hwinfo_t g_uart_hwinfo =
{
    USART0,          /* uart_no  : 选择 USART0 外设 */
    RCU_USART0,      /* rcu_uart : 使能 USART0 时钟 */
    RCU_GPIOA,       /* rcu_gpio : 使能 GPIOA 时钟 */
    GPIOA,           /* gpio     : GPIO 端口为 GPIOA */
    GPIO_PIN_9,      /* tx_pin   : TX -> PA9 */
    GPIO_PIN_10,     /* rx_pin   : RX -> PA10 */
    USART0_IRQn,     /* irq      : USART0 中断向量 */

    DMA0,            /* dma_num  : DMA0 控制器 */
    RCU_DMA0,        /* rcu_dma  : DMA0 时钟 */
    DMA_CH4          /* dam_ch   : DMA 通道 4（用于 USART0 RX，依芯片映射） */
};

/* ======================== LED 控制数据结构（协议 payload） ======================== */
typedef struct
{
    uint8_t led_num;     /* LED 编号/索引 */
    uint8_t led_state;   /* LED 状态：0=关，非0=开 */
} led_ctrl_inof_t;       /* 注意：原拼写 inof，保持一致 */

/* ***********************************************************
 * @brief  串口驱动初始化函数
 * @param  baud_rate : 串口波特率（如 9600 / 115200）
 * @return 无
 * @note
 *  1) 初始化 UART GPIO
 *  2) 初始化 UART 外设参数
 *  3) 初始化 UART RX DMA（配合 IDLE 中断做“帧接收”）
 *********************************************************** */
void drv_uart_init(uint32_t baud_rate)
{
    uart_gpio_init();        /* 初始化串口 GPIO：TX 复用推挽，RX 上拉输入 */
    uart_init(baud_rate);    /* 初始化 UART 外设参数：8N1 + 使能收发 + IDLE 中断 */
    uart_dma_int();          /* 初始化 DMA：USART0->g_rc_buf 循环接收 */
}

/* ***********************************************************
 * @brief  UART GPIO 初始化
 * @param  无
 * @return 无
 * @note
 *  TX：复用推挽输出
 *  RX：上拉输入（UART 空闲态为高电平，上拉抗干扰更好）
 *********************************************************** */
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

/* ***********************************************************
 * @brief  UART 外设初始化
 * @param  baud_rate : 波特率
 * @return 无
 * @note
 *  配置为：8 数据位、无校验、1 停止位（8N1）
 *  并开启 IDLE 中断用于“帧结束”判定（配合 DMA）
 *********************************************************** */
static void uart_init(uint32_t baud_rate)
{
    /* 使能 UART 外设时钟 */
    rcu_periph_clock_enable(g_uart_hwinfo.rcu_uart);

    /* 复位 UART 外设，恢复到默认状态 */
    usart_deinit(g_uart_hwinfo.uart_no);

    /* 设置字长：8 位数据 */
    usart_word_length_set(g_uart_hwinfo.uart_no, USART_WL_8BIT);

    /* 设置校验方式：无校验 */
    usart_parity_config(g_uart_hwinfo.uart_no, USART_PM_NONE);

    /* 设置停止位：1 位停止位 */
    usart_stop_bit_set(g_uart_hwinfo.uart_no, USART_STB_1BIT);

    /* 设置波特率 */
    usart_baudrate_set(g_uart_hwinfo.uart_no, baud_rate);

    /* 使能发送功能 */
    usart_transmit_config(g_uart_hwinfo.uart_no, USART_TRANSMIT_ENABLE);

    /* 使能接收功能 */
    usart_receive_config(g_uart_hwinfo.uart_no, USART_RECEIVE_ENABLE);

    /* 使能接收空闲中断（IDLE）：用于检测一帧数据接收结束 */
    usart_interrupt_enable(g_uart_hwinfo.uart_no, USART_INT_IDLE);

    /* 使能 NVIC 中断 */
    nvic_irq_enable(g_uart_hwinfo.irq, 0, 0);

    /* 使能 UART 外设 */
    usart_enable(g_uart_hwinfo.uart_no);
}

/* USART0 数据寄存器地址（用于 DMA 外设地址） */
#define USART0_DATA_ADDR    (USART0 + 0x04)

/* ***********************************************************
 * @brief  UART RX DMA 初始化（外设->内存）
 * @param  无
 * @return 无
 * @note
 *  1) DMA 源：USART0 数据寄存器
 *  2) DMA 目的：g_rc_buf
 *  3) 源地址不自增，目的地址自增
 *  4) 传输计数：RX_BUF_SIZE
 *  5) 配合 IDLE 中断，在“空闲”到来时统计已接收字节数
 *********************************************************** */
static void uart_dma_int(void)
{
    /* 使能 DMA 时钟 */
    rcu_periph_clock_enable(g_uart_hwinfo.rcu_dma);

    /* 复位 DMA 通道 */
    dma_deinit(g_uart_hwinfo.dma_num, g_uart_hwinfo.dam_ch);

    dma_parameter_struct dma_struct;

    /* 传输方向：外设 -> 内存 */
    dma_struct.direction = DMA_PERIPHERAL_TO_MEMORY;

    /* 外设地址：USART0 数据寄存器 */
    dma_struct.periph_addr = USART0_DATA_ADDR;

    /* 外设地址不自增（始终读同一个数据寄存器） */
    dma_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;

    /* 外设数据宽度：8bit */
    dma_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;

    /* 内存地址：接收缓冲区 */
    dma_struct.memory_addr = (uint32_t)g_rc_buf;

    /* 内存地址自增（顺序填充缓冲） */
    dma_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;

    /* 内存数据宽度：8bit */
    dma_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;

    /* 传输数量：缓冲区大小 */
    dma_struct.number = RX_BUF_SIZE;

    /* DMA 通道优先级 */
    dma_struct.priority = DMA_PRIORITY_HIGH;

    /* 初始化 DMA 通道参数 */
    dma_init(g_uart_hwinfo.dma_num, g_uart_hwinfo.dam_ch, &dma_struct);

    /* 使能 USART 接收使用 DMA */
    usart_dma_receive_config(g_uart_hwinfo.uart_no, USART_RECEIVE_DMA_ENABLE);

    /* 使能 DMA 通道，开始接收 */
    dma_channel_enable(g_uart_hwinfo.dma_num, g_uart_hwinfo.dam_ch);
}

/* ***********************************************************
 * @brief  发送单个字符（阻塞方式）
 * @param  ch : 要发送的字符
 * @return 无
 * @note
 *  1) 写发送数据寄存器
 *  2) 等待发送缓冲空标志 TBE，表示可以继续发送下一个字节
 *********************************************************** */
void uart_send_char(const char ch)
{
    /* 将数据写入发送数据寄存器 */
    usart_data_transmit(g_uart_hwinfo.uart_no, (uint8_t)ch);

    /* 等待发送缓冲区为空（可以发送下一个字节） */
    while (usart_flag_get(g_uart_hwinfo.uart_no, USART_FLAG_TBE) == RESET)
    {
        /* busy wait */
    }
}

/* ***********************************************************
 * @brief  发送字符串（以 '\0' 结尾）
 * @param  str : 字符串首地址
 * @return 无
 * @note 逐字节调用 uart_send_char 发送
 *********************************************************** */
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

/* ***********************************************************
 * @brief  printf 重定向（把标准输出重定向到串口）
 * @param  ch : 要输出的字符
 * @param  f  : 文件指针（此处不用）
 * @return 输出的字符
 *********************************************************** */
int fputc(int ch, FILE *f)
{
    (void)f; /* 避免未使用参数告警 */
    uart_send_char((uint8_t)ch);
    return ch;
}

/* ***********************************************************
 * @brief  计算 XOR 校验（逐字节异或）
 * @param  data : 数据起始地址
 * @param  len  : 参与校验的长度
 * @return XOR 校验值
 * @note   常见做法：对“除最后校验字节外”的所有字节 XOR，结果放到最后一字节
 *********************************************************** */
static uint8_t cal_xor_sum(const uint8_t *data, uint8_t len)
{
    uint8_t xor_sum = 0;

    for (uint8_t i = 0; i < len; i++)
    {
        xor_sum ^= data[i];
    }

    return xor_sum;
}

/* ***********************************************************
 * @brief  按协议控制 LED
 * @param  ctrldata : 指向 LED 控制 payload（包含 led_num, led_state）
 * @return 无
 *********************************************************** */
static void clrt_led(led_ctrl_inof_t *ctrldata)
{
    /* led_state 非 0 则开灯，否则关灯 */
    (ctrldata->led_state != 0) ? drv_led_on(ctrldata->led_num)
                              : drv_led_off(ctrldata->led_num);
}

/* ***********************************************************
 * @brief  串口任务处理函数（在主循环/任务中周期调用）
 * @param  无
 * @return 无
 * @note
 *  工作流程：
 *   1) 等待 ISR 将 g_rc_succeed_flag 置位（表示一帧数据接收完成）
 *   2) 检查帧头
 *   3) 校验 XOR
 *   4) 根据 FUNC 分发处理（这里示例：LED 控制）
 *********************************************************** */
void drv_uart_task(void)
{
    /* 未接收到完整一帧则直接返回 */
    if (!g_rc_succeed_flag)
    {
        return;
    }

    /* 消耗标志（本次处理完再等下一帧） */
    g_rc_succeed_flag = false;

    /* 帧头校验：必须是 0x55 0xAA */
    if (g_rc_buf[0] != DATA_HEAD_ONE || g_rc_buf[1] != DATA_HEAD_TOW)
    {
        return;
    }

    /* XOR 校验：
     * 参与校验长度 = PACKET_DATA_LEN - 1（最后一个字节是校验字节本身）
     */
    if (cal_xor_sum(g_rc_buf, PACKET_DATA_LEN - 1) != g_rc_buf[PACKET_DATA_LEN - 1])
    {
        return;
    }

    /* 功能码分发 */
    if (g_rc_buf[FUNC_DATA_IDX] == LED_CTRL_CODE)
    {
        /* FUNC 后面紧跟 payload：
         * g_rc_buf[FUNC_DATA_IDX + 1] 开始是 led_ctrl_inof_t
         */
        clrt_led((led_ctrl_inof_t*)(&g_rc_buf[FUNC_DATA_IDX + 1]));
    }
}

/* ***********************************************************
 * @brief  USART0 中断服务函数（使用 IDLE 中断判断“接收结束”）
 * @param  无
 * @return 无
 * @note
 *  IDLE 中断清除的一般流程（很多芯片/库要求）：
 *   1) 读 STAT/标志寄存器
 *   2) 读 DATA 寄存器
 *  这样才能清掉 IDLE 标志，否则会重复进中断
 *
 *  本代码使用 DMA 接收，IDLE 到来时用：
 *   已接收字节数 = RX_BUF_SIZE - dma_transfer_number_get(...)
 *  如果等于 PACKET_DATA_LEN，则认为一帧接收完成。
 *
 *  随后重启 DMA 以接收下一帧：
 *   disable -> config number -> enable
 *********************************************************** */
void USART0_IRQHandler(void)
{
    /* 判断是否为 IDLE 中断 */
    if (usart_interrupt_flag_get(g_uart_hwinfo.uart_no, USART_INT_FLAG_IDLE) != RESET)
    {
        /* 按常见要求：读数据寄存器以清 IDLE（配合读状态寄存器） */
        usart_data_receive(g_uart_hwinfo.uart_no);

        /* 清除 IDLE 中断标志（部分库/芯片需要显式清） */
        usart_interrupt_flag_clear(g_uart_hwinfo.uart_no, USART_INT_FLAG_IDLE);

        /* 计算 DMA 已接收的字节数：
         * DMA 剩余传输次数 = dma_transfer_number_get(...)
         * 已接收 = RX_BUF_SIZE - 剩余次数
         */
        if (PACKET_DATA_LEN ==
            (RX_BUF_SIZE - dma_transfer_number_get(g_uart_hwinfo.dma_num, g_uart_hwinfo.dam_ch)))
        {
            /* 接收到期望长度，置位一帧完成标志，等待任务函数解析 */
            g_rc_succeed_flag = true;
        }

        /* 重新装载 DMA：
         * 1) 关闭 DMA 通道
         * 2) 重设传输数量
         * 3) 重新使能 DMA 通道
         */
        dma_channel_disable(g_uart_hwinfo.dma_num, g_uart_hwinfo.dam_ch);
        dma_transfer_number_config(g_uart_hwinfo.dma_num, g_uart_hwinfo.dam_ch, RX_BUF_SIZE);
        dma_channel_enable(g_uart_hwinfo.dma_num, g_uart_hwinfo.dam_ch);
    }
}
