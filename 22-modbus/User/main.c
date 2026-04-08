#include "config.h"     // ����ȫ�������ļ���оƬ�ͺš��궨�塢���ܿ��صȣ�
#include "task.h"

/******************************** ϵͳ��ͷ�ļ� ********************************/
#include "sys_dwt_delay/delay.h"   // ���� DWT ��΢�뼶��ʱģ��
#include "sys_tick/sys_tick.h"     // SysTick ��ʱ��������ϵͳʱ����

/******************************** Ӧ�ò�ͷ�ļ� ********************************/
#include "app_uart/app_uart.h"     // UART Ӧ�ò��߼���Э����� / ��������

/******************************** ������ͷ�ļ� ********************************/
#include "drv_led/drv_led.h"       // LED Ӳ������
#include "drv_key/drv_key.h"       // ����Ӳ������
#include "drv_uart/drv_uart.h"     // UART �ײ���������ʼ�������͡����գ�
#include "drv_pwm/drv_pwm.h"
#include "drv_capture/drv_capture.h"
#include "drv_rtc/drv_rtc.h"
#include "drv_dgt/drv_dgt.h"
#include "drv_adc_dma/drv_adc_dma.h"
#include "drv_sensor/drv_sensor.h"
#include "modbus_app.h"
/**
 * @brief  ϵͳ����ʼ��
 * @note
 *  - ֻ��ʼ���� MCU �ں���صĻ���ģ��
 *  - ���漰�������裨GPIO��UART �ȣ�
 */
static void sys_init(void)
{
    delay_init();      // ��ʼ�� DWT ��ʱ���ܣ���ʹ�� DWT->CYCCNT��
    sys_tick_init();   // ��ʼ�� SysTick���ṩϵͳ���ģ��� 1ms��
}

/**
 * @brief  Ӧ�ò��ʼ��
 * @note
 *  - ����ҵ���߼����ģ��ĳ�ʼ��
 *  - ��ֱ�Ӳ���Ӳ���Ĵ���
 */
static void app_init(void)
{
    app_uart_init();   // ��ʼ�� UART Ӧ�ò㣨״̬�����������ȣ�
	ModbusAppInit();
}

/**
 * @brief  �������ʼ��
 * @note
 *  - ֱ����Ӳ���򽻵�
 *  - ���� GPIO������Ĵ�����
 */
static void drv_init(void)
{
    drv_led_init();            // ��ʼ�� LED ���ţ����ģʽ��
    drv_key_init();            // ��ʼ���������ţ����� + ����/������
    drv_uart_init(115200);     // ��ʼ�����ڣ������� 115200
	drv_pwm_init();
	drv_capture_init();
	drv_rtc_init();
	drv_dgt_init();
//	drv_adc_dma_init();
	drv_sensor_init();
}
#define SHENGXU  1
#define JIANGXUE 0
#include <stdlib.h>
#if JIANGXUE

int32_t CmpCb(const void *_a, const void *_b)
{
	uint16_t *a = (uint16_t *)_a;
	uint16_t *b = (uint16_t *)_b;
	int32_t val;
	if(*a > *b)
	{
		val = 1;
	}
	else if(*a < *b)
	{
		val = -1;
	}
	else
	{
		val = 0;
	}
	return val;
}

#else
int32_t CmpCb(const void *_a, const void *_b)
{
	uint16_t *a = (uint16_t *)_a;
	uint16_t *b = (uint16_t *)_b;
	int32_t val;
	if(*a > *b)
	{
		val = -1;
	}
	else if(*a < *b)
	{
		val = 1;
	}
	else
	{
		val = 0;
	}
	return val;
}
#endif
/**
 ***********************************************************
 * @brief  ������
 * @note
 *  ����˵����
 *   - ��ʾ��Ϊ UART + �������ܲ��Թ���
 *   - ��ѭ���в�����ѯ��ʽִ��Ӧ�ò�����
 ***********************************************************
 */
int main(void)
{
	
	
	
	task_init();

    /******** ϵͳ��ʼ�� ********/
    sys_init();     // ��ʼ��ϵͳ����ģ�飨ʱ������ʱ��

    /******** ������ʼ�� ********/
    drv_init();     // ��ʼ������Ӳ������

    /******** Ӧ�ó�ʼ�� ********/
    app_init();     // ��ʼ��Ӧ�ò��߼�

    while (1)
    {

		task_loop();
    }
}
