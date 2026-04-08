#include "drv_uart/drv_uart.h"
#include "drv_led/drv_led.h"

#define RX_BUF_SIZE 		20
#define DATA_HEAD_ONE 		0x55
#define DATA_HEAD_TOW 		0xAA
#define PACKET_DATA_LEN		(CTRL_DATA_LEN + 4)
#define CTRL_DATA_LEN		3
#define FUNC_DATA_IDX		3
#define LED_CTRL_CODE		0x06		
/* ======================== 内部函数前置声明 ======================== */
static void uart_gpio_init(void);
static void uart_init(uint32_t baud_rate);
static bool g_rc_succeed_flag = false;
static uint8_t g_rc_buf[RX_BUF_SIZE];
/* ======================== UART 硬件资源描述结构体 ======================== */
/*
 * 该结构体用于描述一组 UART 硬件资源
 * 便于后续扩展为多串口（可仿照 LED/KEY 的表驱动方式）
 */
typedef struct 
{
    uint32_t        uart_no;    /* USART 外设编号，如 USART0 / USART1 */
    rcu_periph_enum rcu_uart;   /* USART 外设时钟 */
    rcu_periph_enum rcu_gpio;   /* GPIO 端口时钟 */
    uint32_t        gpio;       /* GPIO 端口基地址，如 GPIOA */
    uint32_t        tx_pin;     /* TX 引脚 */
    uint32_t        rx_pin;     /* RX 引脚 */
	uint8_t        	irq;    	/* UART 中断号 */
} uart_hwinfo_t;

/* ======================== UART0 硬件资源配置 ======================== */
/*
 * 本工程使用 USART0
 * TX : PA9
 * RX : PA10
 */
static const uart_hwinfo_t g_uart_hwinfo =
{
    USART0,        /* uart_no  */
    RCU_USART0,    /* rcu_uart */
    RCU_GPIOA,     /* rcu_gpio */
    GPIOA,         /* gpio     */
    GPIO_PIN_9,    /* tx_pin   */
    GPIO_PIN_10,   /* rx_pin   */
	USART0_IRQn    /* uart_irq   */
};
typedef struct 
{
	uint8_t led_num;
	uint8_t led_state;
}led_ctrl_inof_t;
/**
***********************************************************
* @brief  串口驱动初始化函数
* @param  baud_rate : 串口波特率（如 9600 / 115200）
* @return 无
* @note
*  该函数完成 UART 所需的 GPIO 初始化和 UART 外设初始化
***********************************************************
*/

void drv_uart_init(uint32_t baud_rate)
{
    uart_gpio_init();   /* 初始化串口 GPIO */
    uart_init(baud_rate); /* 初始化 UART 外设参数 */
}

/**
***********************************************************
* @brief  UART GPIO 初始化
* @param  无
* @return 无
* @note
*  TX：复用推挽输出
*  RX：上拉输入（空闲为高，抗干扰更好）
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
*  配置为：8 数据位、无校验、1 停止位（8N1）
***********************************************************
*/
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
	
	/* 使能接收中断功能 */
	usart_interrupt_enable(g_uart_hwinfo.uart_no, USART_INT_RBNE);
	 
	/* 使能串口中断功能 */
	nvic_irq_enable(g_uart_hwinfo.irq, 0, 0);
	
    /* 使能 UART 外设 */
    usart_enable(g_uart_hwinfo.uart_no);
}

/**
***********************************************************
* @brief  发送单个字符（阻塞方式）
* @param  ch : 要发送的字符
* @return 无
* @note
*  使用发送缓冲空标志 TBE 判断是否可以发送下一个字节
***********************************************************
*/
void uart_send_char(const char ch)
{
    /* 将数据写入发送数据寄存器 */
    usart_data_transmit(g_uart_hwinfo.uart_no, (uint8_t)ch);

    /* 等待发送缓冲区为空（可以发送下一个字节） */
    while (usart_flag_get(g_uart_hwinfo.uart_no, USART_FLAG_TBE) == RESET);
}

/**
***********************************************************
* @brief  发送字符串（以 '\0' 结尾）
* @param  str : 字符串首地址
* @return 无
* @note
*  逐字节调用 uart_send_char 发送
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
* @brief  串口重定向
* @param  无
* @return 无
* @note   无
***********************************************************
*/
int fputc(int ch, FILE *f)
{
	uart_send_char((uint8_t)ch);
	return ch;
}
static void pro_uart_data(uint8_t dat)
{
	static uint8_t index = 0;
	g_rc_buf[index++] = dat;
	switch(index)
	{
		case 1: if(g_rc_buf[0] != DATA_HEAD_ONE) index = 0; break;
		case 2: if(g_rc_buf[1] != DATA_HEAD_TOW) index = 0; break;
		case 7: g_rc_succeed_flag = true; index = 0; break;		
		default:  break;
	}
}
static uint8_t cal_xor_sum(const uint8_t *data, uint8_t len)
{
	uint8_t xor_sum = 0;
	for(uint8_t i = 0; i < len; i++)
	{
		xor_sum ^= data[i];
	}
	return xor_sum;
}
static void clrt_led(led_ctrl_inof_t *ctrldata)
{
	(ctrldata->led_state != 0) ? drv_led_on(ctrldata->led_num) : drv_led_off(ctrldata->led_num);
}
/**
***********************************************************
* @brief  串口任务处理函数
* @param  无
* @return 无
* @note   无
***********************************************************
*/
void drv_uart_task(void)
{
	if(!g_rc_succeed_flag)
	{
		return;
	}
	
	g_rc_succeed_flag = false;
	
	if(cal_xor_sum(g_rc_buf, PACKET_DATA_LEN - 1) != g_rc_buf[PACKET_DATA_LEN - 1])
	{
		return;
	}
	if(g_rc_buf[FUNC_DATA_IDX] == LED_CTRL_CODE)
	{
		clrt_led((led_ctrl_inof_t*)(&g_rc_buf[FUNC_DATA_IDX + 1]));
	}
}
/**
***********************************************************
* @brief  串口中断服务函数
* @param  无
* @return 无
* @note   无
***********************************************************
*/
void USART0_IRQHandler(void)
{
	if(usart_interrupt_flag_get(g_uart_hwinfo.uart_no, USART_INT_FLAG_RBNE) != RESET)
	{
		uint8_t dat = (uint8_t)usart_data_receive(g_uart_hwinfo.uart_no);
		pro_uart_data(dat);
		/* 再清（如果库要求清），否则可以不清 */
		usart_interrupt_flag_clear(g_uart_hwinfo.uart_no, USART_INT_FLAG_RBNE);

	}
}
