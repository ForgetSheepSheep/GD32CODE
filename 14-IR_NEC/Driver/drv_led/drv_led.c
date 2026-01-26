#include "drv_led/drv_led.h"

/* ======================== LED GPIO 映射结构体定义 ======================== */
/**
 * @brief  LED 对应的 GPIO 硬件描述结构体
 * @note
 * 1) 每一个 LED 由一个 GPIO 端口和一个引脚控制
 * 2) rcu      ：GPIO 外设时钟使能枚举
 * 3) gpio     ：GPIO 外设基地址
 * 4) gpio_pin ：GPIO 引脚编号
 */
typedef struct
{
    rcu_periph_enum rcu;   /* GPIO 时钟控制 */
    uint32_t        gpio;  /* GPIO 端口 */
    uint32_t        gpio_pin; /* GPIO 引脚 */
} Led_GPIO_t;

/* ======================== LED GPIO 配置表 ======================== */
/**
 * @brief  LED GPIO 映射表
 * @note
 * 通过表驱动方式统一管理多个 LED，
 * 上层仅需使用 LED_ID 即可控制具体硬件
 */
static const Led_GPIO_t g_gpio_list[] =
{
    {RCU_GPIOA, GPIOA, GPIO_PIN_8},   /* LED1 */
    {RCU_GPIOE, GPIOE, GPIO_PIN_6},   /* LED2 */
    {RCU_GPIOF, GPIOF, GPIO_PIN_6},   /* LED3 */
};

/* LED 数量宏定义 */
#define LED_NUM_MAX  (sizeof(g_gpio_list) / sizeof(g_gpio_list[0]))

/* ======================== LED 初始化函数 ======================== */
/**
***********************************************************
* @brief  LED 模块初始化
* @note
* 1) 依次使能每个 LED 对应 GPIO 的时钟
* 2) 将 GPIO 配置为推挽输出模式
* 3) 初始化默认输出为低电平（LED 默认熄灭）
* @param  无
* @return 无
***********************************************************
*/
void drv_led_init(void)
{
    /* 遍历 LED GPIO 映射表，逐个初始化 */
    for (uint8_t i = 0; i < LED_NUM_MAX; i++)
    {
        /* 使能 GPIO 外设时钟 */
        rcu_periph_clock_enable(g_gpio_list[i].rcu);

        /* GPIO 配置为推挽输出，10MHz 输出速度 */
        gpio_init(g_gpio_list[i].gpio,
                  GPIO_MODE_OUT_PP,
                  GPIO_OSPEED_10MHZ,
                  g_gpio_list[i].gpio_pin);

        /* 默认输出低电平，LED 熄灭 */
        gpio_bit_reset(g_gpio_list[i].gpio,
                       g_gpio_list[i].gpio_pin);
    }
}

/* ======================== LED 点亮函数 ======================== */
/**
***********************************************************
* @brief  点亮指定 LED
* @param  led_id LED 编号（0 ~ LED_NUM_MAX-1）
* @note
* 1) 根据 LED_ID 从 GPIO 映射表中查找对应硬件
* 2) 输出高电平点亮 LED
* @return 无
***********************************************************
*/
void drv_led_on(uint8_t led_id)
{
    /* 参数合法性检查 */
    if (led_id > LED_NUM_MAX)
    {
        return;
    }

    /* 输出高电平，点亮 LED */
    gpio_bit_set(g_gpio_list[led_id].gpio,
                 g_gpio_list[led_id].gpio_pin);
}

/* ======================== LED 熄灭函数 ======================== */
/**
***********************************************************
* @brief  熄灭指定 LED
* @param  led_id LED 编号（0 ~ LED_NUM_MAX-1）
* @note
* 输出低电平关闭 LED
* @return 无
***********************************************************
*/
void drv_led_off(uint8_t led_id)
{
    /* 参数合法性检查 */
    if (led_id > LED_NUM_MAX)
    {
        return;
    }

    /* 输出低电平，熄灭 LED */
    gpio_bit_reset(g_gpio_list[led_id].gpio,
                   g_gpio_list[led_id].gpio_pin);
}

/* ======================== LED 翻转函数 ======================== */
/**
***********************************************************
* @brief  翻转指定 LED 状态
* @param  led_id LED 编号（0 ~ LED_NUM_MAX-1）
* @note
* 1) 先读取当前 GPIO 输出状态
* 2) 若为高电平，则置低
* 3) 若为低电平，则置高
* @return 无
***********************************************************
*/
void drv_led_toggle(uint8_t led_id)
{
    /* 参数合法性检查 */
    if (led_id > LED_NUM_MAX)
    {
        return;
    }

    /* 读取当前 LED 输出状态并翻转 */
    if (gpio_output_bit_get(g_gpio_list[led_id].gpio,
                            g_gpio_list[led_id].gpio_pin) == SET)
    {
        /* 当前为高电平，翻转为低电平 */
        gpio_bit_reset(g_gpio_list[led_id].gpio,
                       g_gpio_list[led_id].gpio_pin);
    }
    else
    {
        /* 当前为低电平，翻转为高电平 */
        gpio_bit_set(g_gpio_list[led_id].gpio,
                     g_gpio_list[led_id].gpio_pin);
    }
}
