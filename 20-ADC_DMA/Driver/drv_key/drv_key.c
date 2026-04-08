#include "drv_key/drv_key.h"
#include "sys_tick/sys_tick.h"



#define KEY_DEBOUNCE_MS     20U
#define KEY_LONG_MS        800U
#define KEY_DOUBLE_GAP_MS  300U

static void key_gpio_init(void);

typedef struct
{
	rcu_periph_enum rcu;
	uint32_t gpio;
	uint32_t gpio_pin;
}Key_GPIO_t;

static const Key_GPIO_t g_gpio_list[] = 
{
	{RCU_GPIOA, GPIOA, GPIO_PIN_0},
	{RCU_GPIOG, GPIOG, GPIO_PIN_13},
	{RCU_GPIOG, GPIOG, GPIO_PIN_14},
	{RCU_GPIOG, GPIOG, GPIO_PIN_15},
};
#define KEY_MAX_NUM  ((sizeof(g_gpio_list)) / (sizeof(g_gpio_list[0])))
	

/**
***********************************************************
* @brief  key硬件初始化
* @param  无
* @return 无
***********************************************************
*/
void drv_key_init(void)
{
	key_gpio_init();

}
/**************************IO_Init**************************/
static void key_gpio_init(void)
{
	for(uint8_t i = 0; i < KEY_MAX_NUM; i++)
	{
		rcu_periph_clock_enable(g_gpio_list[i].rcu);
		gpio_init(g_gpio_list[i].gpio, GPIO_MODE_IPU, GPIO_OSPEED_2MHZ,
				  g_gpio_list[i].gpio_pin);
	}
}
typedef enum
{
	KEY_STATE_RELEASE = 0,		// 按键未按下 / 空闲状态
	KEY_STATE_CONFIRM, 			// 按键按下后的确认/消抖阶段
	KEY_STATE_PRESSING,			// 短按事件
	KEY_STATE_LONG,				// 长按事件
}key_state_t;


typedef struct
{
    key_state_t state;          /* 当前按键状态 */

    uint8_t     click_cnt;      /* 已确认的短按次数（用于双击判断） */

    uint64_t    press_time;     /* 稳定按下时刻（ms） */
    uint64_t    release_time;   /* 稳定释放时刻（ms） */

} key_info_t;

static  key_info_t g_key_info[KEY_MAX_NUM];

static uint8_t key_scan(uint8_t key_index)
{
    uint64_t now;
    uint8_t  pressed;   /* pressed = 1 表示按下，0 表示松开（低有效：按下为RESET） */

    /* 参数检查：key_index 必须在有效范围内 */
    if (key_index >= KEY_MAX_NUM)
    {
        return 0xFF;
    }

    /* 读取按键电平：上拉输入 + 按下接地 => RESET 代表按下 */
    pressed = (gpio_input_bit_get(g_gpio_list[key_index].gpio,
                                 g_gpio_list[key_index].gpio_pin) == RESET);

    /* 统一读取一次系统时间，避免重复调用 */
    now = sys_tick_get_runtime();

    switch (g_key_info[key_index].state)
    {
        case KEY_STATE_RELEASE:
        {
            /* -------- 单击延迟确认 --------
             * 如果已经发生过一次短按释放（click_cnt == 1）
             * 并且超过双击窗口还没出现第二次点击，则确认单击
             */
            if (g_key_info[key_index].click_cnt == 1)
            {
                if ((now - g_key_info[key_index].release_time) > KEY_DOUBLE_GAP_MS)
                {
                    g_key_info[key_index].click_cnt = 0;
                    return (uint8_t)(key_index + 0x01); /* 单击：0x01~ */
                }
            }

            /* -------- 检测按下沿：进入消抖 -------- */
            if (pressed)
            {
                g_key_info[key_index].press_time = now;
                g_key_info[key_index].state = KEY_STATE_CONFIRM;
            }
            break;
        }

        case KEY_STATE_CONFIRM:
        {
            /* 消抖确认：按下保持超过 KEY_DEBOUNCE_MS 才算有效 */
            if (pressed)
            {
                if ((now - g_key_info[key_index].press_time) >= KEY_DEBOUNCE_MS)
                {
                    /* 消抖通过：以稳定按下时刻作为后续长按计时基准 */
                    g_key_info[key_index].press_time = now;
                    g_key_info[key_index].state = KEY_STATE_PRESSING;
                }
            }
            else
            {
                /* 消抖期间松开：认为是抖动/误触，回到空闲 */
                g_key_info[key_index].state = KEY_STATE_RELEASE;
            }
            break;
        }

        case KEY_STATE_PRESSING:
        {
            /* 注意：释放应该是 !pressed（松开后电平为高） */
            if (!pressed)
            {
                /* 松开：一次点击完成（可能是单击或双击的一部分） */
                g_key_info[key_index].state = KEY_STATE_RELEASE;

                g_key_info[key_index].click_cnt++;

                if (g_key_info[key_index].click_cnt == 1)
                {
                    /* 第一次释放：记录释放时刻，等待双击窗口 */
                    g_key_info[key_index].release_time = now;
                }
                else
                {
                    /* 第二次释放：若在窗口内则判为双击 */
                    if ((now - g_key_info[key_index].release_time) <= KEY_DOUBLE_GAP_MS)
                    {
                        g_key_info[key_index].click_cnt = 0;
                        return (uint8_t)(key_index + 0x51); /* 双击：0x51~ */
                    }

                    /* 超出窗口：本次作为新的第一次点击 */
                    g_key_info[key_index].click_cnt = 1;
                    g_key_info[key_index].release_time = now;
                }
            }
            else
            {
                /* 仍按住：判断长按 */
                if ((now - g_key_info[key_index].press_time) >= KEY_LONG_MS)
                {
                    g_key_info[key_index].state = KEY_STATE_LONG;
                }
            }
            break;
        }

        case KEY_STATE_LONG:
        {
            /* 长按成立后，松开才返回长按事件 */
            if (!pressed)
            {
                g_key_info[key_index].state = KEY_STATE_RELEASE;
                g_key_info[key_index].click_cnt = 0;  /* 长按后不再参与单双击 */
                return (uint8_t)(key_index + 0x81);   /* 长按：0x81~ */
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
***********************************************************
* @brief  获取按键码值
* @param  无
* @return res 按键码值
***********************************************************
*/
uint8_t drv_get_key_val(void)
{
	uint8_t res = 0;
	for(uint8_t i = 0; i < KEY_MAX_NUM; i++)
	{
		res = key_scan(i);
		if(res != 0 )
		{
			break;
		}
	}
	return res;
}
