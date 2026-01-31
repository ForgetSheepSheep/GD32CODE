#include "app_uart/app_uart.h"
#include "drv_uart/drv_uart.h"
#include "drv_led/drv_led.h"
#include "queue/queue.h"

/*
 * ========================= UART 应用层协议说明 =========================
 *
 * 1) 数据帧格式（典型短帧，示例为 7 字节）：
 *
 * ┌────────┬────────┬────────┬──────────┬──────────┬──────────┬────────┐
 * │  HEAD1 │  HEAD2 │  LEN   │  FUNC    │  DATA0   │  DATA1   │  XOR   │
 * ├────────┼────────┼────────┼──────────┼──────────┼──────────┼────────┤
 * │ 0x55   │ 0xAA   │   0x03 │  FUNC_ID │ payload  │ payload  │ 校验   │
 * └────────┴────────┴────────┴──────────┴──────────┴──────────┴────────┘
 *
 * 示例：55 AA 03 06 02 01 XX
 *   - HEAD1/HEAD2：帧头（同步用，接收端靠它“对齐帧起点”）
 *   - LEN：功能码 + 数据区的长度（本协议示例固定为 3 = 1字节FUNC + 2字节DATA）
 *   - FUNC：功能码（例如 0x06 表示 LED 控制）
 *   - DATA0/DATA1：负载数据（例如 led_num / led_state）
 *   - XOR：校验字节，等于 “从 HEAD1 到最后一个 DATA 字节”的逐字节异或结果
 *
 * 注意：
 *   - “串口自己加 XOR”通常不是硬件自动完成，而是发送端按协议计算后发送；
 *     接收端需要按相同规则重算并与 XOR 字节比较。
 *
 * 2) 软件架构（中断收字节 + 主循环解析）：
 *   UART RX 中断回调 -> pro_uart_data(dat) 仅做入队（快速、无阻塞）
 *                    -> app_uart_task() 在主循环中出队、组帧、校验、执行业务
 *
 * 这样做的好处：
 *   - 中断服务程序极短，减少丢字节风险
 *   - 解析/业务集中在主循环，便于调试与扩展（增加更多功能码）
 */

#define RX_BUF_SIZE            77      /* 队列底层缓冲区大小：需大于可能的最大积压字节数 */

#define DATA_HEAD_ONE          0x55    /* 帧头1 */
#define DATA_HEAD_TOW          0xAA    /* 帧头2（命名应为 TWO，此处沿用原宏名） */

/* 帧长约束：最短至少包含 HEAD(2)+LEN(1)+FUNC(1)+DATA(>=0)+XOR(1)
 * 你这里示例帧最短为 7；最大按需求扩展到 11（便于后续增加数据区长度）
 */
#define PACKET_DATA_LEN_MIN    7
#define PACKET_DATA_LEN_MAX    11

#define FUNC_DATA_IDX          3       /* FUNC 的下标：0x55[0],0xAA[1],LEN[2],FUNC[3] */
#define LED_CTRL_CODE          0x06    /* 功能码：LED 控制 */

/*
 * LED 控制数据结构（对应 DATA 区的解释）
 * 帧内映射：
 *   read_buffer[4] -> led_num   ：LED 编号
 *   read_buffer[5] -> led_state ：LED 状态（0关，非0开）
 */
typedef struct
{
    uint8_t led_num;     /* LED 编号 */
    uint8_t led_state;   /* LED 状态 */
} led_ctrl_inof_t;

/* 接收队列对象：
 * - ISR（中断）往里写（Push）
 * - 主循环从里读（Pop）
 */
static QueueType_t g_rcv_queue;

/* 队列实际存储区（环形缓冲区） */
static uint8_t g_rc_buf[RX_BUF_SIZE];

/**
***********************************************************
* @brief  串口字节接收处理函数（中断回调入口）
* @param  dat : UART 中断收到的 1 字节数据
* @return 无
* @note
*  1) 该函数应由 UART 驱动在“收到 1 字节”时调用（通常在 RXNE 中断中）
*  2) 仅做入队，不做组帧/校验/业务，保证中断执行快
*  3) 若队列满，QueuePush 的返回值应在内部处理（覆盖/丢弃/计数）
***********************************************************
*/
static void pro_uart_data(uint8_t dat)
{
    /* 将收到的字节放入队列，等待主循环解析 */
    QueuePush(&g_rcv_queue, dat);
}

/**
***********************************************************
* @brief  异或校验计算（XOR）
* @param  data : 参与校验的字节起始地址
* @param  len  : 参与校验的字节数量（不包含 XOR 校验字节本身）
* @return 计算得到的 XOR 校验值
* @note
*  xor_sum = data[0] ^ data[1] ^ ... ^ data[len-1]
***********************************************************
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
***********************************************************
* @brief  LED 控制执行函数（业务动作）
* @param  ctrldata : LED 控制参数指针（led_num/led_state）
* @return 无
* @note
*  - led_state != 0：开灯
*  - led_state == 0：关灯
***********************************************************
*/
static void ctrl_led(led_ctrl_inof_t *ctrldata)
{
    (ctrldata->led_state != 0) ?
        drv_led_on(ctrldata->led_num) :
        drv_led_off(ctrldata->led_num);
}

/**
***********************************************************
* @brief  UART APP 层任务处理函数（主循环调用）
* @param  无
* @return 无
* @note
*  组帧解析流程（从队列里按字节取数据）：
*   1) 找帧头 0x55（HEAD1），不是则继续丢弃该字节
*   2) 读取下一个字节并判断是否为 0xAA（HEAD2），不是则丢弃并重新同步
*   3) 读取 LEN 字节：表示后续 “FUNC + DATA” 的长度（示例为 3）
*   4) 从队列再读取 (LEN + 1) 字节：
*        - LEN 字节：FUNC + DATA
*        - +1 字节：XOR 校验字节
*   5) 计算校验：对 [HEAD1..最后一个DATA] 做 XOR，结果应等于帧尾 XOR 字节
*   6) 根据 FUNC 分发业务处理（例如 LED 控制）
*   7) 打印整帧内容用于调试（hex dump）
*
* 关键下标关系（以 read_buffer 为准）：
*   read_buffer[0] = HEAD1
*   read_buffer[1] = HEAD2
*   read_buffer[2] = LEN
*   read_buffer[3] = FUNC
*   read_buffer[4..(3+LEN-1)] = DATA 区（LEN-1 字节）
*   read_buffer[3+LEN] = XOR（校验字节）
*
* 重要提示（当前代码逻辑风险，仅作为注释说明）：
*   - 业务分发处使用的是 g_rc_buf[FUNC_DATA_IDX]，g_rc_buf 是“队列底层缓存”，
*     并不等价于当前已解析出的 read_buffer。更严谨的做法应以 read_buffer 为准。
*     （你这里 ctrl_led 已经用 read_buffer 的数据区了，但 FUNC 判断仍可能误判）
***********************************************************
*/
void app_uart_task(void)
{
    /* 解析状态 */
    typedef enum {
        ST_WAIT_HEAD1 = 0,
        ST_WAIT_HEAD2,
        ST_WAIT_LEN,
        ST_WAIT_PAYLOAD_XOR
    } parse_state_t;

    static parse_state_t st = ST_WAIT_HEAD1;

    /* 帧缓存（承载一帧完整数据） */
    static uint8_t  frame[PACKET_DATA_LEN_MAX];
    static uint8_t  idx = 0;          /* 已收集到 frame 的字节数 */
    static uint8_t  len = 0;          /* LEN 字段：FUNC+DATA 的长度 */
    static uint8_t  need = 0;         /* 还需要收多少字节（FUNC+DATA+XOR） */

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
            /* 否则继续丢弃直到看到 0x55 */
            break;

        case ST_WAIT_HEAD2:
            if (byte == DATA_HEAD_TOW)
            {
                frame[1] = byte;
                idx = 2;
                st = ST_WAIT_LEN;
            }
            else if (byte == DATA_HEAD_ONE)
            {
                /* 滑窗重同步：当前字节可能是下一帧 HEAD1 */
                frame[0] = byte;
                idx = 1;
                st = ST_WAIT_HEAD2;
            }
            else
            {
                /* 彻底失配，回到找 HEAD1 */
                st = ST_WAIT_HEAD1;
                idx = 0;
            }
            break;

        case ST_WAIT_LEN:
            len = byte;

            /* LEN 合法性：整帧长度 = LEN + 4，必须 <= PACKET_DATA_LEN_MAX */
            if (len < 1 || (uint16_t)(len + 4) > PACKET_DATA_LEN_MAX)
            {
                /* 长度异常，重新同步 */
                st = ST_WAIT_HEAD1;
                idx = 0;
                break;
            }

            frame[2] = len;
            idx = 3;

            /* 接下来还需要：LEN 字节(FUNC+DATA) + 1 字节(XOR) */
            need = (uint8_t)(len + 1);
            st = ST_WAIT_PAYLOAD_XOR;
            break;

        case ST_WAIT_PAYLOAD_XOR:
            frame[idx++] = byte;

            if (--need == 0)
            {
                /* 一帧收齐：frame[0..(len+3)] 共 len+4 字节 */
                uint8_t total = (uint8_t)(len + 4);
                uint8_t xor_calc = cal_xor_sum(frame, (uint8_t)(len + 3));
                uint8_t xor_recv = frame[len + 3];

                if (xor_calc == xor_recv)
                {
                    /* 业务分发：FUNC 必须来自当前帧 frame */
                    if (frame[FUNC_DATA_IDX] == LED_CTRL_CODE)
                    {
                        /* 协议要求 LED 控制至少 1(FUNC)+2(DATA) => LEN >= 3 */
                        if (len >= 3)
                        {
                            led_ctrl_inof_t info;
                            info.led_num   = frame[FUNC_DATA_IDX + 1];
                            info.led_state = frame[FUNC_DATA_IDX + 2];
                            ctrl_led(&info);
                        }
                    }

                    /* 调试打印整帧（校验通过后再打印） */
                    for (uint8_t i = 0; i < total; i++)
                    {
                        printf("%02x ", frame[i]);
                    }
                    printf("\n");
                }

                /* 无论成功与否，都回到找下一帧 */
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
#include <string.h> // for memmove

void app_uart_task2(void)
{
    /* 线性流缓存：保存“已收到但尚未解析完”的字节 */
    static uint8_t  stream[RX_BUF_SIZE * 2];   // 你也可以按需求调大
    static uint16_t stream_len = 0;

    uint8_t byte;

    /* 1) 把队列里现有的字节尽可能搬到 stream 里 */
    while (QueuePop(&g_rcv_queue, &byte) == QUEUE_OK)
    {
        if (stream_len < sizeof(stream))
        {
            stream[stream_len++] = byte;
        }
        else
        {
            /* 流缓存满：丢弃最老的数据（也可选择清空/计数/告警） */
            memmove(stream, stream + 1, stream_len - 1);
            stream[stream_len - 1] = byte;
        }
    }

    /* 2) 在 stream 里按“原逻辑”循环解析：找头->读len->校验->分发 */
    while (1)
    {
        /* 至少要有 HEAD1+HEAD2+LEN 三个字节 */
        if (stream_len < 3)
        {
            break;
        }

        /* 2.1 找帧头 0x55 0xAA（对齐帧起点） */
        if (stream[0] != DATA_HEAD_ONE)
        {
            /* 丢弃一个字节，继续对齐 */
            memmove(stream, stream + 1, stream_len - 1);
            stream_len--;
            continue;
        }

        if (stream[1] != DATA_HEAD_TOW)
        {
            /* 如果第二字节不是 0xAA，丢弃第一个字节，继续找 */
            memmove(stream, stream + 1, stream_len - 1);
            stream_len--;
            continue;
        }

        /* 2.2 读取 LEN，并计算整帧长度 total = LEN + 4 */
        uint8_t len = stream[2];

        /* 合法性检查：整帧长度不能超过 PACKET_DATA_LEN_MAX */
        if (len < 1 || (uint16_t)(len + 4) > PACKET_DATA_LEN_MAX)
        {
            /* 长度异常：丢弃帧头第一个字节，重新同步 */
            memmove(stream, stream + 1, stream_len - 1);
            stream_len--;
            continue;
        }

        uint8_t total = (uint8_t)(len + 4); // 2头 + 1len + len(payload) + 1xor

        /* 2.3 数据不够一整帧：先退出，等下一轮再继续拼 */
        if (stream_len < total)
        {
            break;
        }

        /* 2.4 至此 stream[0..total-1] 是一帧完整数据，做 XOR 校验 */
        uint8_t xor_calc = cal_xor_sum(stream, (uint8_t)(len + 3)); // HEAD1..DATA end
        uint8_t xor_recv = stream[len + 3];                         // XOR 字节

        if (xor_calc != xor_recv)
        {
            /* 校验失败：丢弃一个字节，继续找下一个可能帧头 */
            memmove(stream, stream + 1, stream_len - 1);
            stream_len--;
            continue;
        }

        /* 2.5 校验通过：按功能码分发（FUNC 在下标 3） */
        uint8_t func = stream[FUNC_DATA_IDX];

        if (func == LED_CTRL_CODE)
        {
            /* LED 控制至少要求：LEN >= 3 (FUNC + 2字节DATA) */
            if (len >= 3)
            {
                led_ctrl_inof_t info;
                info.led_num   = stream[FUNC_DATA_IDX + 1];
                info.led_state = stream[FUNC_DATA_IDX + 2];
                ctrl_led(&info);
            }
        }

        /* 调试打印整帧 */
        for (uint8_t i = 0; i < total; i++)
        {
            printf("%02x ", stream[i]);
        }
        printf("\n");

        /* 2.6 消费掉这一帧：从 stream 中移除前 total 个字节 */
        memmove(stream, stream + total, stream_len - total);
        stream_len -= total;
    }
}


/**
***********************************************************
* @brief  UART APP 初始化函数
* @param  无
* @return 无
* @note
*  1) 注册 UART 驱动层的“字节到达回调函数”
*     - UART 驱动收到 1 字节后，会回调 pro_uart_data(dat)
*  2) 初始化接收队列
*     - g_rc_buf 作为队列底层存储区
*     - RX_BUF_SIZE 为队列容量（字节数）
***********************************************************
*/
void app_uart_init(void)
{
    uart_callback(pro_uart_data);               /* 注册字节接收回调 */
    QueueInit(&g_rcv_queue, g_rc_buf, RX_BUF_SIZE); /* 初始化环形队列 */
}
