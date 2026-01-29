#include "config.h"     // 工程全局配置文件（芯片型号、宏定义、功能开关等）
#include "task.h"

/******************************** 系统层头文件 ********************************/
#include "sys_dwt_delay/delay.h"   // 基于 DWT 的微秒级延时模块
#include "sys_tick/sys_tick.h"     // SysTick 定时器驱动（系统时基）

/******************************** 应用层头文件 ********************************/
#include "app_uart/app_uart.h"     // UART 应用层逻辑（协议解析 / 任务处理）

/******************************** 驱动层头文件 ********************************/
#include "drv_led/drv_led.h"       // LED 硬件驱动
#include "drv_key/drv_key.h"       // 按键硬件驱动
#include "drv_uart/drv_uart.h"     // UART 底层驱动（初始化、发送、接收）
#include "drv_pwm/drv_pwm.h"
#include "drv_capture/drv_capture.h"
#include "drv_rtc/drv_rtc.h"
#include "drv_dgt/drv_dgt.h"
/**
 * @brief  系统级初始化
 * @note
 *  - 只初始化与 MCU 内核相关的基础模块
 *  - 不涉及具体外设（GPIO、UART 等）
 */
static void sys_init(void)
{
    delay_init();      // 初始化 DWT 延时功能（需使能 DWT->CYCCNT）
    sys_tick_init();   // 初始化 SysTick，提供系统节拍（如 1ms）
}

/**
 * @brief  应用层初始化
 * @note
 *  - 负责业务逻辑相关模块的初始化
 *  - 不直接操作硬件寄存器
 */
static void app_init(void)
{
    app_uart_init();   // 初始化 UART 应用层（状态机、缓冲区等）
}

/**
 * @brief  驱动层初始化
 * @note
 *  - 直接与硬件打交道
 *  - 包括 GPIO、外设寄存器等
 */
static void drv_init(void)
{
    drv_led_init();            // 初始化 LED 引脚（输出模式）
    drv_key_init();            // 初始化按键引脚（输入 + 上拉/下拉）
    drv_uart_init(115200);     // 初始化串口，波特率 115200
	drv_pwm_init();
	drv_capture_init();
	drv_rtc_init();
	drv_dgt_init();
}


/**
 ***********************************************************
 * @brief  主函数
 * @note
 *  功能说明：
 *   - 本示例为 UART + 按键功能测试工程
 *   - 主循环中采用轮询方式执行应用层任务
 ***********************************************************
 */
int main(void)
{
	task_init();

    /******** 系统初始化 ********/
    sys_init();     // 初始化系统基础模块（时基、延时）

    /******** 驱动初始化 ********/
    drv_init();     // 初始化所有硬件驱动

    /******** 应用初始化 ********/
    app_init();     // 初始化应用层逻辑


    while (1)
    {
		task_loop();
//		//led_pwm_test();
//		capture_test();
		
    }
}
