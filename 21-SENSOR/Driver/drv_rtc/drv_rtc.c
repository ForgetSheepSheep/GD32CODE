#include "drv_rtc/drv_rtc.h"
#include <time.h>
#include <string.h>

// 1. Correct Typo
#define MAGIC_CODE  0x5A5A

/**
***********************************************************
* @brief RTC硬件初始化
* @param  无
* @return 无
* @note  该函数仅在上电或后备寄存器数据丢失时初始化RTC。
*       初始化完成后，写入MAGIC_CODE标记，下次复位将跳过配置。
***********************************************************
*/
void drv_rtc_init(void)
{
    // 2. Fix BKP Register inconsistency (Use BKP_DATA_0 consistently)
    if(bkp_read_data(BKP_DATA_0) != MAGIC_CODE)
    {
        /* === 第一阶段：时钟与备份域配置 === */
        /* 使能PMU和BKP外设时钟 */
        rcu_periph_clock_enable(RCU_PMU);
        rcu_periph_clock_enable(RCU_BKPI);
        /* 使能对后备寄存器和RTC的写权限 */
        pmu_backup_write_enable();
        /* 复位后备寄存器 */
        bkp_deinit();

        /* === 第二阶段：RTC时钟源配置 === */
        /* 使能外部低速晶振(LXTAL, 32.768KHz)，并等待其稳定 */
        rcu_osci_on(RCU_LXTAL);
        rcu_osci_stab_wait(RCU_LXTAL);
        /* 设置RTC时钟源为LXTAL */
        rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);
        /* 使能RTC时钟 */
        rcu_periph_clock_enable(RCU_RTC);

        /* === 第三阶段：RTC基本参数配置 === */
        /* 等待APB1接口时钟和RTC时钟同步 */
        rtc_register_sync_wait();
        /* 等待上次对 RTC 寄存器写操作完成 */
        rtc_lwoff_wait();

        /* 设置RTC预分频值 */
        /* LXTAL = 32768 Hz, 预分频值 = 32767 -> RTC时钟 = 1Hz (1秒/次) */
        rtc_prescaler_set(32767);
        rtc_lwoff_wait();

        /* === 第四阶段：设置默认时间 === */
        /* 使用辅助函数设置初始时间，确保与drv_set_rtc_time逻辑一致 */
        /* 默认时间：2023-02-28 23:59:50 */
        rtc_time_t default_time = {2023, 2, 28, 23, 59, 50};
        drv_set_rtc_time(&default_time);

        /* 写入初始化标志位，防止重复初始化 */
        // 2. (Again) Ensure BKP_DATA_0 is used here
        bkp_write_data(BKP_DATA_0, MAGIC_CODE);
        return;
    }

    /* === 非首次初始化流程 (系统复位/唤醒) === */
    /* 重新使能时钟和权限 (复位后寄存器保护会恢复) */
    rcu_periph_clock_enable(RCU_PMU);
    rcu_periph_clock_enable(RCU_BKPI);
    pmu_backup_write_enable();

    /* 等待寄存器同步，确保能正确读取RTC时间 */
    rtc_register_sync_wait();
    rtc_lwoff_wait();
}

/**
***********************************************************
* @brief 设置RTC时间
* @param  time: 指向包含年月日时分秒的时间结构体指针
* @return 无
* @note  输入时间视为本地时间，函数将其转换为UTC时间戳存储。
***********************************************************
*/
void drv_set_rtc_time(rtc_time_t *time)
{
    time_t timeStamp;
    struct tm tmInfo;

    memset(&tmInfo, 0, sizeof(tmInfo));

    /* 填充tm结构体 (tm_year是从1900年开始的年份，tm_mon是0-11) */
    tmInfo.tm_year = time->year - 1900;
    tmInfo.tm_mon  = time->month - 1;
    tmInfo.tm_mday = time->day;
    tmInfo.tm_hour = time->hour;
    tmInfo.tm_min  = time->minute;
    tmInfo.tm_sec  = time->second;

    /* 转换为Unix时间戳 */
    timeStamp = mktime(&tmInfo);

    /* 时区补偿：假设传入的是东八区时间，硬件存储UTC时间 */
    /* 注意：此逻辑依赖于系统环境未设置时区或系统默认为UTC */
    timeStamp -= (8 * 60 * 60);

    /* 写入RTC计数器 */
    rtc_lwoff_wait();
    rtc_counter_set(timeStamp);
}

/**
***********************************************************
* @brief 获取RTC时间
* @param  time: 指向用于存储获取到的时间结构体指针
* @return 无
* @note  硬件读取UTC时间戳，函数将其转换为本地时间。
***********************************************************
*/
void drv_get_rtc_time(rtc_time_t *time)
{
    time_t timeStmp;
    struct tm *timeinfo;

    /* 读取硬件计数器值 (UTC) */
    timeStmp = rtc_counter_get();

    /* 时区补偿：将UTC转换为东八区时间 */
    timeStmp += (8 * 60 * 60);

    /* 转换为tm结构体 */
    timeinfo = localtime(&timeStmp);

    /* 填充输出结构体 */
    time->year   = timeinfo->tm_year + 1900;
    time->month  = timeinfo->tm_mon + 1;
    time->day    = timeinfo->tm_mday;
    time->hour   = timeinfo->tm_hour;
    time->minute = timeinfo->tm_min;
    time->second = timeinfo->tm_sec;
}
