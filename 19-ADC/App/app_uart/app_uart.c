/**
 ******************************************************************************
 * @file    app_uart.c
 * @brief   UART 应用层源文件
 * @details 基于 UART 驱动的应用层协议解析和 LED 控制
 ******************************************************************************
 */

#include "app_uart/app_uart.h"
#include "drv_uart/drv_uart.h"
#include "drv_led/drv_led.h"
#include "queue/queue.h"
#include <string.h>

/*
 * ======================== UART 应用层协议约定 ========================
 *
 * 1) 协议格式：帧头(2) + 长度(1) + 功能码(1) + 数据(N) + 校验(1)，例如 7 字节帧：
 *
 * 协议字段说明：
 *   HEAD1   : 0x55  (帧头1)
 *   HEAD2   : 0xAA  (帧头2，对应应 TWO，拼写错误)
 *   LEN     : 长度  (功能码+数据的长度，协议示例中帧长为 3 = 1字节功能 + 2字节数据)
 *   FUNC    : 功能码 (例如 0x06 表示 LED 控制)
 *   DATA0   : 数据0   (例如 led_num / led_state)
 *   DATA1   : 数据1
 *   XOR     : 校验码 (异或校验)
 *
 * 协议字段索引：
 *   0x55   : 0xAA   : 0x03 : FUNC_ID  : payload  : payload  : 校验   :
 *
 * 示例：55 AA 03 06 02 01 XX
 *   - HEAD1/HEAD2：帧同步，接收端以此识别帧头。起始
 *   - LEN：数据长度+功能码的长度，协议示例中帧长为 3 = 1字节功能 + 2字节数据
 *   - FUNC：功能码，例如 0x06 表示 LED 控制
 *   - DATA0/DATA1：负载数据，例如 led_num / led_state
 *   - XOR：异或校验，计算顺序从 HEAD1 开始，直到最后的 DATA 字段或前序异或值。
 *
 * 注意：协议实现细节有误，XOR 校验通常对整帧进行异或累加，但发送端未按协议发送，接收端需要按协议计算 XOR 并进行比对。
 */

#define RX_BUF_SIZE            77      /* 环形队列缓冲区大小，防止长时间中断时数据溢出 */
#define DATA_HEAD_ONE          0x55    /* 帧头1 */
#define DATA_HEAD_TWO          0xAA    /* 帧头2 (应 TWO，拼写错误，原代码保留) */
#define PACKET_DATA_LEN_MIN    7
#define PACKET_DATA_LEN_MAX    11
#define FUNC_DATA_IDX          3       /* FUNC 字节索引：0x55[0],0xAA[1],LEN[2],FUNC[3] */
#define LED_CTRL_CODE          0x06    /* 功能码：LED 控制 */

/*
 * LED 控制数据结构，对应 DATA 节的解析结构：
 * 帧长解析：
 *   read_buffer[4] -> led_num   (LED 编号)
 *   read_buffer[5] -> led_state (LED 状态，0熄灭，非0点亮)
 */
typedef struct
{
    uint8_t led_num;     /* LED 编号 */
    uint8_t led_state;   /* LED 状态 */
} led_ctrl_info_t;

/* 接收队列中断 */
/* - ISR 中断中，写入 Push */
/* - 应用层中，读取 Pop */
static QueueType_t g_rcv_queue;

/* 接收缓冲存储，环形队列 */
static uint8_t g_rc_buf[RX_BUF_SIZE];

/**
 * @brief  协议帧接收处理，由 UART 中断回调触发
 * @param  dat : UART 中断接收到的 1 字节数据
 * @retval 无
 * @note
 *   1) 该函数对应于 UART 驱动中 UART 接收回调函数，接收的 1 字节入队
 *   2) 若中断期间接收多个字节，暂存至缓冲区，避免中断中阻塞
 *   3) 入队函数 QueuePush 对应于队列缓冲区/缓冲
 */
static void pro_uart_data(uint8_t dat)
{
    /* 将接收到的字节暂存至环形队列，用于后续处理 */
    QueuePush(&g_rcv_queue, dat);
}

/**
 * @brief  异或校验计算（XOR 校验）
 * @param  data : 需要校验的数据帧
 * @param  len  : 需要校验的数据帧长度（不包含 XOR 校验字节）
 * @retval 计算得到的 XOR 校验值
 * @note
 *   xor_sum = data[0] ^ data[1] ^ ... ^ data[len-1]
 */
static uint8_t cal_xor_sum(const uint8_t *data, uint8_t len)
{
    uint8_t xor_sum = 0;

    for (uint8_t i = 0; i < len; i++)
    {
        xor_sum ^= data[i];
    }

    return xor_sum;
}

/**
 * @brief  LED 控制处理和命令响应
 * @param  ctrl_data : LED 控制参数结构（led_num/led_state）
 * @retval 无
 * @note
 *   - led_state != 0：点亮
 *   - led_state == 0：熄灭
 */
static void ctrl_led(led_ctrl_info_t *ctrl_data)
{
    (ctrl_data->led_state != 0) ?
        drv_led_on(ctrl_data->led_num) :
        drv_led_off(ctrl_data->led_num);
}

/**
 * @brief  UART APP 任务处理，用于主循环调用
 * @param  无
 * @retval 无
 * @note
 *   帧解析流程，从队列中取出完整帧并进行解析、校验、数据处理：
 *   1) 读取帧头：读取帧头字节，确认协议起始。
 *   2) 读取长度：读取下一字节，确认为协议帧或帧头丢失。同步至字节为 0xAA (HEAD2)
 *   3) 读取 LEN：读取长度字节，示例中帧长为 FUNC + DATA = 3
 *   4) 从队列再取 LEN 字节存入：
 *        - LEN 字节存入 FUNC + DATA
 *        - +1 字节存入 XOR 校验字节
 *   5) 执行校验：对 [HEAD1..最新数据] 进行异或计算，对应值与位 XOR 校验字节
 *   6) 解析 FUNC 字段发送至 LED 控制：
 *
 * 状态机索引说明 read_buffer 为计算基准：
 *   read_buffer[0] = HEAD1
 *   read_buffer[1] = HEAD2
 *   read_buffer[2] = LEN
 *   read_buffer[3] = FUNC
 *   read_buffer[4..(3+LEN-1)] = DATA (LEN-1 字节存入)
 *   read_buffer[3+LEN] = XOR (校验字节存入)
 *
 * 重要说明：当前代码存在逻辑错误，若先中断接收，则 g_rc_buf[FUNC_DATA_IDX] 是环队列缓冲区，frame 是暂存帧，
 *     若解析完后，frame 是正确帧，应按 frame 来控制 LED，而 ctrl_led 已按 read_buffer 解析完成，
 *     若 FUNC 判断可随时中断校验，故应参考解析完毕后的 frame 为基准。
 */
void app_uart_task(void)
{
    /* 有限状态机 */
    typedef enum {
        ST_WAIT_HEAD1 = 0,
        ST_WAIT_HEAD2,
        ST_WAIT_LEN,
        ST_WAIT_PAYLOAD_XOR
    } parse_state_t;

    static parse_state_t st = ST_WAIT_HEAD1;

    /* 帧缓冲（暂存头、长度、数据） */
    static uint8_t  frame[PACKET_DATA_LEN_MAX];
    static uint8_t  idx = 0;          /* 接收字节到 frame 的索引 */
    static uint8_t  len = 0;          /* LEN 字段值，FUNC+DATA 的长度 */
    static uint8_t  need = 0;         /* 还需要从队列取出的字节数（FUNC+DATA+XOR） */

    uint8_t byte = 0;

    while (QueuePop(&g_rcv_queue, &byte) == QUEUE_OK)
    {
        switch (st)
        {
        case ST_WAIT_HEAD1:
            if (byte == DATA_HEAD_ONE)
            {
                frame[0] = byte;
                idx = 1;
                st = ST_WAIT_HEAD2;
            }
            /* 若非帧头字节，则继续等待 0x55 */
            break;

        case ST_WAIT_HEAD2:
            if (byte == DATA_HEAD_TWO)
            {
                frame[1] = byte;
                idx = 2;
                st = ST_WAIT_LEN;
            }
            else if (byte == DATA_HEAD_ONE)
            {
                /* 若重复帧头，则丢弃当前帧，将当前字节存为 HEAD1 */
                frame[0] = byte;
                idx = 1;
                st = ST_WAIT_HEAD2;
            }
            else
            {
                /* 解析失败，回到等待 HEAD1 */
                st = ST_WAIT_HEAD1;
                idx = 0;
            }
            break;

        case ST_WAIT_LEN:
            len = byte;

            /* LEN 值有效，示例帧长为 LEN + 4，需确保 total <= PACKET_DATA_LEN_MAX */
            if (len < 1 || (uint16_t)(len + 4) > PACKET_DATA_LEN_MAX)
            {
                /* 帧长错误，丢弃当前帧，回到 HEAD1 */
                st = ST_WAIT_HEAD1;
                idx = 0;
                break;
            }

            frame[2] = len;
            idx = 3;

            /* 计算还需从队列取出字节数：LEN 的字节数(FUNC+DATA) + 1 字节(XOR) */
            need = (uint8_t)(len + 1);
            st = ST_WAIT_PAYLOAD_XOR;
            break;

        case ST_WAIT_PAYLOAD_XOR:
            frame[idx++] = byte;

            if (--need == 0)
            {
                /* 缓冲、校验：对 frame[0..(len+3)] 计算异或，共 len+4 字节 */
                uint8_t total = (uint8_t)(len + 4);
                uint8_t xor_calc = cal_xor_sum(frame, (uint8_t)(len + 3));
                uint8_t xor_recv = frame[len + 3];

                if (xor_calc == xor_recv)
                {
                    /* 命中：发送端的 FUNC 字段对应为当前的 frame */
                    if (frame[FUNC_DATA_IDX] == LED_CTRL_CODE)
                    {
                        /* 协议说明为 LED 控制，数据长为 1(FUNC)+2(DATA) => LEN >= 3 */
                        if (len >= 3)
                        {
                            led_ctrl_info_t info;
                            info.led_num   = frame[FUNC_DATA_IDX + 1];
                            info.led_state = frame[FUNC_DATA_IDX + 2];
                            ctrl_led(&info);
                        }
                    }

                    /* 调试：打印帧头、长度、校验、通用的 hex dump */
                    for (uint8_t i = 0; i < total; i++)
                    {
                        printf("%02x ", frame[i]);
                    }
                    printf("\r\n");
                }

                /* 处理成功，状态回到等待 HEAD1 */
                st = ST_WAIT_HEAD1;
                idx = 0;
            }
            break;

        default:
            st = ST_WAIT_HEAD1;
            idx = 0;
            break;
        }
    }
}

/**
 * @brief  UART APP 初始化函数
 * @param  无
 * @retval 无
 * @note
 *   1) 注册 UART 中断接收回调函数
 *     - UART 接收到 1 字节后，会调用 pro_uart_data(dat)
 *   2) 初始化接收队列
 *     - g_rc_buf 为环队列缓冲区
 *     - RX_BUF_SIZE 为缓冲区大小，可缓存较长时间中断时数据
 */
void app_uart_init(void)
{
    uart_callback(pro_uart_data);               /* 注册接收队列回调 */
    QueueInit(&g_rcv_queue, g_rc_buf, RX_BUF_SIZE); /* 初始化接收队列 */
}
