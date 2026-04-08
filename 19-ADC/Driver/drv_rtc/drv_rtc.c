/**
 ******************************************************************************
 * @file    drv_rtc.c
 * @brief   RTC 驱动源文件
 * @details 用于 GD32F303 的 RTC 实时时钟驱动，支持日期和时间的读写
 ******************************************************************************
 */

#include "drv_rtc/drv_rtc.h"
#include <time.h>
#include <string.h>

/**
 * @brief  RTC 硬件初始化函数
 * @param  无
 * @retval 无
 * @note
 *   该函数在上电后若备份寄存器数据丢失时才初始化 RTC
 *   初始化完成后写入 MAGIC_CODE，保证下次复位不再重新初始化
 */
void drv_rtc_init(void)
{
    /* 检查是否首次初始化（备份寄存器值不等于 MAGIC_CODE） */
    if (bkp_read_data(BKP_DATA_0) != MAGIC_CODE)
    {
        /* === 第一步阶段：使能 PMU 和 BKP 电源时钟 === */
        /* 使能 PMU 和 BKP 时钟 */
        rcu_periph_clock_enable(RCU_PMU);
        rcu_periph_clock_enable(RCU_BKPI);
        /* 使能对后备寄存器的 RTC 写权限 */
        pmu_backup_write_enable();
        /* 复位后备寄存器 */
        bkp_deinit();

        /* === 第二步阶段：RTC 时钟源配置 === */
        /* 使能外部低速晶振(LXTAL, 32.768KHz)，等待稳定 */
        rcu_osci_on(RCU_LXTAL);
        rcu_osci_stab_wait(RCU_LXTAL);
        /* 配置 RTC 时钟源为 LXTAL */
        rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);
        /* 使能 RTC 时钟 */
        rcu_periph_clock_enable(RCU_RTC);

        /* === 第三步阶段：RTC 配置参数设置 === */
        /* 等待 APB1 总线时钟和 RTC 时钟同步 */
        rtc_register_sync_wait();
        /* 等待上次对 RTC 寄存器的写操作完成 */
        rtc_lwoff_wait();

        /* 配置 RTC 预分频值 */
        /* LXTAL = 32768 Hz, 预分频值 = 32767 -> RTC时钟 = 1Hz (1秒/次) */
        rtc_prescaler_set(32767);
        rtc_lwoff_wait();

        /* === 第四步阶段：设置默认时间 === */
        /* 使用 drv_set_rtc_time 函数逻辑一样 */
        /* 默认时间：2023-02-28 23:59:50 */
        rtc_time_t default_time = {2023, 2, 28, 23, 59, 50};
        drv_set_rtc_time(&default_time);

        /* 写入初始化完成标志位，防止重复初始化 */
        bkp_write_data(BKP_DATA_0, MAGIC_CODE);
        return;
    }

    /* === 非首次初始化情况 (上电复位/断电) === */
    /* 重新使能时钟和写权限 (复位后备寄存器内容或丢失) */
    rcu_periph_clock_enable(RCU_PMU);
    rcu_periph_clock_enable(RCU_BKPI);
    pmu_backup_write_enable();

    /* 等待寄存器同步，确保安全访问 RTC 时钟 */
    rtc_register_sync_wait();
    rtc_lwoff_wait();
}

/**
 * @brief  设置 RTC 时间
 * @param  time : 指向时间结构体的指针，包含要设置的时间
 * @retval 无
 * @note
 *   设置时间为北京时间，自动转换为 UTC 时间存储
 */
void drv_set_rtc_time(rtc_time_t *time)
{
    time_t time_stamp;
    struct tm tm_info;

    memset(&tm_info, 0, sizeof(tm_info));

    /* 填充 tm 结构体 (tm_year 是从 1900 年开始的偏移年份，tm_mon 是 0-11) */
    tm_info.tm_year = time->year - 1900;
    tm_info.tm_mon  = time->month - 1;
    tm_info.tm_mday = time->day;
    tm_info.tm_hour = time->hour;
    tm_info.tm_min  = time->minute;
    tm_info.tm_sec  = time->second;

    /* 转换为 Unix 时间戳 */
    time_stamp = mktime(&tm_info);

    /* 时间戳调整为北京时间 (硬件存储 UTC 时间) */
    /* 注意：调用逻辑未考虑系统时区，系统默认为 UTC */
    time_stamp -= (8 * 60 * 60);

    /* 写入 RTC 计数器 */
    rtc_lwoff_wait();
    rtc_counter_set(time_stamp);
}

/**
 * @brief  获取 RTC 时间
 * @param  time : 指向时间结构体的指针，用于存储读取到的时间
 * @retval 无
 * @note
 *   从硬件读取 UTC 时间，自动转换为北京时间
 */
void drv_get_rtc_time(rtc_time_t *time)
{
    time_t time_stmp;
    struct tm *timeinfo;

    /* 读取硬件计数器值 (UTC) */
    time_stmp = rtc_counter_get();

    /* 时间戳由 UTC 转换为北京时间 */
    time_stmp += (8 * 60 * 60);

    /* 转换为 tm 结构体 */
    timeinfo = localtime(&time_stmp);

    /* 填充结构体 */
    time->year   = timeinfo->tm_year + 1900;
    time->month  = timeinfo->tm_mon + 1;
    time->day    = timeinfo->tm_mday;
    time->hour   = timeinfo->tm_hour;
    time->minute = timeinfo->tm_min;
    time->second = timeinfo->tm_sec;
}
