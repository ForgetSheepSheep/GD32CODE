/**
 ******************************************************************************
 * @file    drv_key.c
 * @brief   KEY 驱动源文件
 * @details 用于 GD32F303 的 KEY 模块驱动，支持单击、双击、长按检测
 ******************************************************************************
 */

#include "drv_key/drv_key.h"
#include "sys_tick/sys_tick.h"

/* ======================== 按键时间参数定义 ======================== */
#define KEY_DEBOUNCE_MS     20U      /* 消抖时间：20ms */
#define KEY_LONG_MS         800U     /* 长按时间：800ms */
#define KEY_DOUBLE_GAP_MS   300U     /* 双击间隔：300ms */

/* ======================== 内部函数声明 ======================== */
static void key_gpio_init(void);
static uint8_t key_scan(uint8_t key_index);

/* ======================== 按键 GPIO 配置结构体 ======================== */
typedef struct
{
    rcu_periph_enum rcu;      /* GPIO 时钟使能枚举 */
    uint32_t        gpio;     /* GPIO 端口号 */
    uint32_t        gpio_pin; /* GPIO 引脚号 */
} Key_GPIO_t;

/* ======================== 按键 GPIO 配置列表 ======================== */
/**
 * @brief  按键引脚配置
 * @note
 *   KEY1 : PA0
 *   KEY2 : PG13
 *   KEY3 : PG14
 *   KEY4 : PG15
 */
static const Key_GPIO_t g_gpio_list[] =
{
    {RCU_GPIOA, GPIOA, GPIO_PIN_0},   /* KEY1 */
    {RCU_GPIOG, GPIOG, GPIO_PIN_13},  /* KEY2 */
    {RCU_GPIOG, GPIOG, GPIO_PIN_14},  /* KEY3 */
    {RCU_GPIOG, GPIOG, GPIO_PIN_15},  /* KEY4 */
};
#define KEY_MAX_NUM  ((sizeof(g_gpio_list)) / (sizeof(g_gpio_list[0])))

/* ======================== 按键状态机定义 ======================== */
/**
 * @brief  按键状态枚举
 * @note
 *   状态机流程：
 *   RELEASE -> CONFIRM -> PRESSING -> LONG
 *   (释放)    (确认)      (按下)      (长按)
 */
typedef enum
{
    KEY_STATE_RELEASE = 0,  /* 按键未按下 / 释放状态 */
    KEY_STATE_CONFIRM,      /* 按键按下后确认/消抖阶段 */
    KEY_STATE_PRESSING,     /* 按下状态 / 单击事件 */
    KEY_STATE_LONG,         /* 长按事件 */
} key_state_t;

/* ======================== 按键信息结构体 ======================== */
/**
 * @brief  按键信息结构体
 * @note
 *   每个按键维护一个状态机信息
 */
typedef struct
{
    key_state_t state;      /* 当前按键状态 */

    uint8_t     click_cnt;  /* 已确认的单击次数，用于双击判断 */

    uint64_t    press_time;  /* 稳定按下时刻（ms） */
    uint64_t    release_time;/* 稳定释放时刻（ms） */

} key_info_t;

/* 全局按键信息数组 */
static key_info_t g_key_info[KEY_MAX_NUM];

/**
 * @brief  KEY 硬件初始化函数
 * @param  无
 * @retval 无
 */
void drv_key_init(void)
{
    key_gpio_init();
}

/**
 * @brief  按键 GPIO 初始化
 * @param  无
 * @retval 无
 * @note
 *   配置为上拉输入模式，按下时为低电平
 */
static void key_gpio_init(void)
{
    for (uint8_t i = 0; i < KEY_MAX_NUM; i++)
    {
        /* 使能 GPIO 时钟 */
        rcu_periph_clock_enable(g_gpio_list[i].rcu);

        /* 配置为上拉输入模式，2MHz 速度 */
        gpio_init(g_gpio_list[i].gpio,
                  GPIO_MODE_IPU,
                  GPIO_OSPEED_2MHZ,
                  g_gpio_list[i].gpio_pin);
    }
}

/**
 * @brief  按键扫描函数（状态机）
 * @param  key_index : 按键索引 (0 ~ KEY_MAX_NUM-1)
 * @retval 按键事件值
 *         - 0x00 : 无事件
 *         - 0x01~0x04 : 单击
 *         - 0x51~0x54 : 双击
 *         - 0x81~0x84 : 长按
 *         - 0xFF : 错误
 */
static uint8_t key_scan(uint8_t key_index)
{
    uint64_t now;
    uint8_t  pressed;   /* pressed = 1 表示按下，0 表示松开（硬件有效电平为 RESET） */

    /* 参数检查：key_index 是否在有效范围内 */
    if (key_index >= KEY_MAX_NUM)
    {
        return 0xFF;
    }

    /* 获取按键电平（上拉输入 + 接下拉按键 => RESET 为按下） */
    pressed = (gpio_input_bit_get(g_gpio_list[key_index].gpio,
                                 g_gpio_list[key_index].gpio_pin) == RESET);

    /* 统一获取一次系统时间，避免重复获取 */
    now = sys_tick_get_runtime();

    /* ======================== 按键状态机 ======================== */
    switch (g_key_info[key_index].state)
    {
        case KEY_STATE_RELEASE:
        {
            /* -------- 判断双击超时 --------
             * 如果已经检测到一次短按释放（click_cnt == 1）
             * 并且超过了双击间隔内还没有检测到第二次按键确认
             */
            if (g_key_info[key_index].click_cnt == 1)
            {
                if ((now - g_key_info[key_index].release_time) > KEY_DOUBLE_GAP_MS)
                {
                    g_key_info[key_index].click_cnt = 0;
                    return (uint8_t)(key_index + 0x01); /* 单击返回0x01~ */
                }
            }

            /* -------- 检测按键按下（开始新按键） -------- */
            if (pressed)
            {
                g_key_info[key_index].press_time = now;
                g_key_info[key_index].state = KEY_STATE_CONFIRM;
            }
            break;
        }

        case KEY_STATE_CONFIRM:
        {
            /* 持续确认：按下仍保持超过 KEY_DEBOUNCE_MS 才有效 */
            if (pressed)
            {
                if ((now - g_key_info[key_index].press_time) >= KEY_DEBOUNCE_MS)
                {
                    /* 消抖通过：重新记录稳定按下时刻作为长按判定基准 */
                    g_key_info[key_index].press_time = now;
                    g_key_info[key_index].state = KEY_STATE_PRESSING;
                }
            }
            else
            {
                /* 在消抖期间松开：认为是误触/抖动，回到释放态 */
                g_key_info[key_index].state = KEY_STATE_RELEASE;
            }
            break;
        }

        case KEY_STATE_PRESSING:
        {
            /* 注意：释放应该判断 !pressed（松开为高电平） */
            if (!pressed)
            {
                /* 松开第一下：这是单击还是双击的第一击 */
                g_key_info[key_index].state = KEY_STATE_RELEASE;
                g_key_info[key_index].click_cnt++;

                if (g_key_info[key_index].click_cnt == 1)
                {
                    /* 第一次释放：记录释放时刻，等待双击 */
                    g_key_info[key_index].release_time = now;
                }
                else
                {
                    /* 第二次释放：判断是否在双击间隔内 */
                    if ((now - g_key_info[key_index].release_time) <= KEY_DOUBLE_GAP_MS)
                    {
                        g_key_info[key_index].click_cnt = 0;
                        return (uint8_t)(key_index + 0x51); /* 双击返回0x51~ */
                    }

                    /* 超出间隔：作为新的第一次点击 */
                    g_key_info[key_index].click_cnt = 1;
                    g_key_info[key_index].release_time = now;
                }
            }
            else
            {
                /* 仍按住：判断是否超长 */
                if ((now - g_key_info[key_index].press_time) >= KEY_LONG_MS)
                {
                    g_key_info[key_index].state = KEY_STATE_LONG;
                }
            }
            break;
        }

        case KEY_STATE_LONG:
        {
            /* 长按期间松开才返回长按事件 */
            if (!pressed)
            {
                g_key_info[key_index].state = KEY_STATE_RELEASE;
                g_key_info[key_index].click_cnt = 0;  /* 长按后不再参与单双击 */
                return (uint8_t)(key_index + 0x81);   /* 长按返回0x81~ */
            }
            break;
        }

        default:
            g_key_info[key_index].state = KEY_STATE_RELEASE;
            g_key_info[key_index].click_cnt = 0;
            break;
    }

    return 0;  /* 无事件 */
}

/**
 * @brief  获取按键值
 * @param  无
 * @retval res 按键值
 */
uint8_t drv_get_key_val(void)
{
    uint8_t res = 0;
    for (uint8_t i = 0; i < KEY_MAX_NUM; i++)
    {
        res = key_scan(i);
        if (res != 0)
        {
            break;
        }
    }
    return res;
}
