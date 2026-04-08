#include "config.h"

/********************************系统头文件********************************/
#include "sys_dwt_delay/delay.h"
#include "sys_tick/sys_tick.h"
/********************************驱动头文件********************************/
#include "drv_led/drv_led.h"
#include "drv_key/drv_key.h"



static void sys_init(void)
{
    delay_init();
    sys_tick_init();
}

static void drv_init(void)
{
    drv_led_init();
    drv_key_init();
}

/**
***********************************************************
* @brief  主函数：按键功能测试
* @note
*  - 单击：松开后约300ms才返回（单击延迟确认，为了与双击区分）
*  - 双击：第二次松开时立即返回
*  - 长按：按住超过阈值后松开返回
***********************************************************
*/
int main(void)
{
    /*******系统初始化********/
    sys_init();
    /*******驱动初始化********/
    drv_init();

    uint8_t key_num;

    /* 用于“双击”实现“两个灯互换状态”的软件标志位（不依赖 toggle 接口） */
    static uint8_t swap_led12 = 0;

    while (1)
    {
        key_num = drv_get_key_val();

        /* 无事件：直接继续 */
        if (key_num == KEY_NULL_PRESS)
        {
            continue;
        }

        switch (key_num)
        {
            /********************* KEY1：控制 LED1 *********************/
            case KEY1_LONG_PRESS:
                drv_led_on(LED1);
                break;

            case KEY1_SHORT_PRESS:
                drv_led_off(LED1);
                break;

            case KEY1_DOUBLE_PRESS:
                /* 双击 KEY1：点亮 LED1 和 LED2（验证双击立即生效） */
                drv_led_on(LED1);
                drv_led_on(LED2);
                break;

            /********************* KEY2：控制 LED2 *********************/
            case KEY2_LONG_PRESS:
                drv_led_off(LED2);
                break;

            case KEY2_SHORT_PRESS:
                drv_led_on(LED2);
                break;

            case KEY2_DOUBLE_PRESS:
                /* 双击 KEY2：LED1/LED2 互换状态（等效 toggle，但不依赖 drv_led_toggle） */
                swap_led12 ^= 1;
                if (swap_led12)
                {
                    drv_led_on(LED1);
                    drv_led_off(LED2);
                }
                else
                {
                    drv_led_off(LED1);
                    drv_led_on(LED2);
                }
                break;

            /********************* KEY3：控制 LED3 *********************/
            case KEY3_LONG_PRESS:
                drv_led_on(LED3);
                break;

            case KEY3_SHORT_PRESS:
                drv_led_off(LED3);
                break;

            case KEY3_DOUBLE_PRESS:
                /* 双击 KEY3：全部灭灯（快速复位测试） */
                drv_led_off(LED1);
                drv_led_off(LED2);
                drv_led_off(LED3);
                break;

            /********************* KEY4：预留（若你板子有第四键） *********************/
            case KEY4_SHORT_PRESS:
                /* 单击 KEY4：全部亮灯（若无 KEY4，可忽略此 case） */
                drv_led_on(LED1);
                drv_led_on(LED2);
                drv_led_on(LED3);
                break;

            case KEY4_LONG_PRESS:
                /* 长按 KEY4：全部灭灯 */
                drv_led_off(LED1);
                drv_led_off(LED2);
                drv_led_off(LED3);
                break;

            case KEY4_DOUBLE_PRESS:
                /* 双击 KEY4：LED2/LED3 互换（简单验证） */
                drv_led_on(LED2);
                drv_led_off(LED3);
                break;

            /********************* 错误码处理 *********************/
            case KEY_ERROR_PRESS:
                /* 测试阶段建议在这里打断点，或点亮一个错误指示灯 */
                break;

            default:
                break;
        }
    }
}
