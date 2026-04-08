#ifndef  __DRV_RTC_H
#define  __DRV_RTC_H
#include "config.h"


typedef struct
{
	uint16_t year;
	uint16_t month;
	uint8_t  day;
	uint8_t  hour;
	uint8_t  minute;
	uint8_t  second;
}rtc_time_t;

#include "drv_rtc/drv_rtc.h"
#include <time.h>
#include <string.h>
#define MIAGC_CODE  0x5A5A
/**
***********************************************************
* @brief RTCÓ²¼þ³õÊ¼»¯
* @param
* @return 
***********************************************************
*/
void drv_rtc_init(void);
/**
***********************************************************
* @brief get  time
* @param
* @return 
***********************************************************
*/
void drv_get_rtc_time(rtc_time_t *time);

void drv_set_rtc_time(rtc_time_t *time);
#endif /* __DRV_RTC_H */
