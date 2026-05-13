#include "serial_comm.h"
#include "gd32f30x_rcu.h"
#include "gd32f30x_gpio.h"
#include "gd32f30x_usart.h"
#include "systick.h"
#include "FreeRTOS.h"
#include "task.h"

/* ============================================================
 *  内部变量
 * ============================================================ */

/* 发送缓冲区 */
static uint8_t tx_buf[FRAME_BUF_SIZE];

/* 接收缓冲区 */
static uint8_t rx_buf[FRAME_BUF_SIZE];

/* 序列号计数器 */
static uint16_t seq_num_counter = 0;

/* ============================================================
 *  CRC32 计算
 *  多项式：0x04C11DB7，宽度32位
 *  初始值：0xFFFFFFFF，结果异或值：0xFFFFFFFF
 *  非反射模式，MSB优先
 * ============================================================ */
uint32_t crc32_calc(const uint8_t *data, uint16_t length){
    uint32_t crc = 0xFFFFFFFF;
    uint16_t i;
    uint8_t j;

    for(i = 0; i < length; i++){
        crc ^= (uint32_t)data[i] << 24;
        for(j = 0; j < 8; j++){
            if(crc & 0x80000000){
                crc = (crc << 1) ^ 0x04C11DB7;
            }else{
                crc <<= 1;
            }
        }
    }

    return crc ^ 0xFFFFFFFF;
}

/* ============================================================
 *  串口初始化
 *  USART0: PA9=TX, PA10=RX
 *  波特率115200, 8位数据位, 无校验, 1位停止位
 * ============================================================ */
void serial_comm_init(void){
    /* 使能 GPIOA 时钟 */
    rcu_periph_clock_enable(RCU_GPIOA);
    /* 使能 USART0 时钟 */
    rcu_periph_clock_enable(RCU_USART0);

    /* 配置 PA9 为 USART0_TX（复用推挽输出） */
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);
    /* 配置 PA10 为 USART0_RX（浮空输入） */
    gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_10);

    /* 配置 USART0 参数 */
    usart_deinit(USART0);
    usart_baudrate_set(USART0, 115200);
    usart_word_length_set(USART0, USART_WL_8BIT);
    usart_parity_config(USART0, USART_PM_NONE);
    usart_stop_bit_set(USART0, USART_STB_1BIT);
    usart_hardware_flow_cts_config(USART0, USART_CTS_DISABLE);
    usart_hardware_flow_rts_config(USART0, USART_RTS_DISABLE);
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
    usart_enable(USART0);
}



/* ============================================================
 *  获取并自增序列号
 * ============================================================ */
uint16_t serial_next_seq_num(void){
    uint16_t seq = seq_num_counter;
    seq_num_counter++;
    if(seq_num_counter == 0){
        seq_num_counter = 1;
    }
    return seq;
}

/* ============================================================
 *  组帧函数
 *  将各字段按照协议格式打包到 out_buf 中
 *  arg_data: 命令参数数据（不包含命令码，命令码由arg_data的前2字节提供）
 *  arg_len:  命令参数数据长度（包含2字节命令码）
 *  返回值: 帧总长度
 *
 *  帧格式：
 *  [0-1]   帧头 0xAA55
 *  [2-3]   长度（大端序，帧总长度）
 *  [4]     版本 0x01
 *  [5]     类型高字节（方向）
 *  [6]     类型低字节（状态）
 *  [7-8]   序列号（大端序）
 *  [9]     通道
 *  [10..]  数据域（命令码 + 命令参数）
 *  [n-5..n-2] CRC32（大端序）
 *  [n-1..n] 帧尾 0xFEEF
 * ============================================================ */
uint16_t serial_build_frame(uint8_t type_high, uint8_t type_low,
                            uint16_t seq_num, uint8_t channel,
                            const uint8_t *arg_data, uint16_t arg_len,
                            uint8_t *out_buf){
    uint16_t frame_len;
    uint32_t crc;
    uint16_t i;

    /* 计算帧总长度 = 固定开销16 + 数据域长度 */
    frame_len = FRAME_OVERHEAD + arg_len;

    /* 帧头 0xAA55 */
    out_buf[0] = FRAME_HEADER_HIGH;
    out_buf[1] = FRAME_HEADER_LOW;

    /* 长度字段（大端序） */
    out_buf[2] = (uint8_t)(frame_len >> 8);
    out_buf[3] = (uint8_t)(frame_len & 0xFF);

    /* 版本 */
    out_buf[4] = FRAME_VERSION;

    /* 类型字段 */
    out_buf[5] = type_high;
    out_buf[6] = type_low;

    /* 序列号（大端序） */
    out_buf[7] = (uint8_t)(seq_num >> 8);
    out_buf[8] = (uint8_t)(seq_num & 0xFF);

    /* 通道 */
    out_buf[9] = channel;

    /* 数据域（命令码 + 命令参数） */
    for(i = 0; i < arg_len; i++){
        out_buf[10 + i] = arg_data[i];
    }

    /* CRC32计算范围：从帧头到数据域末尾 */
    crc = crc32_calc(out_buf, 10 + arg_len);

    /* CRC32字段（大端序） */
    out_buf[10 + arg_len]     = (uint8_t)(crc >> 24);
    out_buf[10 + arg_len + 1] = (uint8_t)(crc >> 16);
    out_buf[10 + arg_len + 2] = (uint8_t)(crc >> 8);
    out_buf[10 + arg_len + 3] = (uint8_t)(crc & 0xFF);

    /* 帧尾 0xFEEF */
    out_buf[10 + arg_len + 4] = FRAME_TAIL_HIGH;
    out_buf[10 + arg_len + 5] = FRAME_TAIL_LOW;

    return frame_len;
}

/* ============================================================
 *  发送帧
 *  组帧后通过串口逐字节发送
 *  seq_num: 序列号，通过serial_next_seq_num()函数递增获取
 *  channel: 通道号，默认0x00
 * ============================================================ */
void serial_send_frame(uint8_t type_high, uint8_t type_low,
                       uint16_t seq_num, uint8_t channel,
                       const uint8_t *arg_data, uint16_t arg_len){
    uint16_t frame_len;
    uint16_t i;

    frame_len = serial_build_frame(type_high, type_low, seq_num, channel,
                                   arg_data, arg_len, tx_buf);

    for(i = 0; i < frame_len; i++){
        while(RESET == usart_flag_get(USART0, USART_FLAG_TBE)){
        }
        usart_data_transmit(USART0, tx_buf[i]);
        while(RESET == usart_flag_get(USART0, USART_FLAG_TC)){
        }
    }
}

/* ============================================================
 *  接收帧
 *  从串口接收数据并解析为协议帧
 *  返回值：0=超时或校验失败，1=成功接收并解析
 *
 *  接收流程：
 *  1. 查找帧头 0xAA55
 *  2. 读取长度字段（2字节大端序）
 *  3. 根据长度读取剩余数据
 *  4. 验证帧尾 0xFEEF
 *  5. 验证CRC32
 *  6. 解析各字段到 ProtocolFrame 结构体
 * ============================================================ */
// 内部辅助宏：带超时的字节接收
#define USART_RECV_BYTE(byte, timeout) do { \
    uint32_t _tick = 0; \
    while(RESET == usart_flag_get(USART0, USART_FLAG_RBNE)){ \
        vTaskDelay(1); \
        if(++_tick >= (timeout)){ return 0; } \
    } \
    (byte) = (uint8_t)usart_data_receive(USART0); \
} while(0)

uint8_t serial_receive_frame(ProtocolFrame *frame, uint32_t timeout_ms){
    uint16_t frame_len;
    uint16_t data_len;
    uint32_t recv_crc;
    uint32_t calc_crc;
    uint16_t i;
    uint8_t byte;

    /* 第1步：查找帧头 0xAA 0x55 */
    while(1){
        USART_RECV_BYTE(byte, timeout_ms);
        if(byte == FRAME_HEADER_HIGH){
            USART_RECV_BYTE(byte, timeout_ms);
            if(byte == FRAME_HEADER_LOW){
                rx_buf[0] = FRAME_HEADER_HIGH;
                rx_buf[1] = FRAME_HEADER_LOW;
                break;
            }
        }
    }

    /* 第2步：读取长度字段（2字节大端序） */
    USART_RECV_BYTE(rx_buf[2], timeout_ms);
    USART_RECV_BYTE(rx_buf[3], timeout_ms);

    frame_len = ((uint16_t)rx_buf[2] << 8) | rx_buf[3];

    /* 长度合法性检查 */
    if(frame_len < FRAME_MIN_SIZE || frame_len > FRAME_BUF_SIZE){
        return 0;
    }

    /* 第3步：读取剩余数据（已读4字节，还需读 frame_len-4 字节） */
    for(i = 4; i < frame_len; i++){
        USART_RECV_BYTE(rx_buf[i], timeout_ms);
    }

    /* 第4步：验证帧尾 0xFEEF */
    if(rx_buf[frame_len - 2] != FRAME_TAIL_HIGH ||
       rx_buf[frame_len - 1] != FRAME_TAIL_LOW){
        return 0;
    }

    /* 第5步：验证CRC32 */
    /* CRC计算范围：从帧头到数据域末尾 = frame_len - 4(CRC) - 2(帧尾) */
    calc_crc = crc32_calc(rx_buf, frame_len - FRAME_CRC_SIZE - FRAME_TAIL_SIZE);
    recv_crc = ((uint32_t)rx_buf[frame_len - 6] << 24) |
               ((uint32_t)rx_buf[frame_len - 5] << 16) |
               ((uint32_t)rx_buf[frame_len - 4] << 8)  |
               ((uint32_t)rx_buf[frame_len - 3]);
    if(calc_crc != recv_crc){
        return 0;
    }

    /* 第6步：解析各字段 */
    frame->length    = frame_len;
    frame->version   = rx_buf[4];
    frame->type_high = rx_buf[5];
    frame->type_low  = rx_buf[6];
    frame->seq_num   = ((uint16_t)rx_buf[7] << 8) | rx_buf[8];
    frame->channel   = rx_buf[9];

    /* 数据域：从偏移10开始，长度 = frame_len - 16(开销) */
    data_len = frame_len - FRAME_OVERHEAD;
    frame->command = ((uint16_t)rx_buf[10] << 8) | rx_buf[11];
    frame->data = &rx_buf[12];
    frame->data_len = data_len - 2;

    frame->crc32 = recv_crc;

    return 1;
}

/* ============================================================
 *  发送应答帧
 * 用于应答传感器0x03类型的上报，帧类型为0x04
 *  数据域：命令码(2字节) + 状态(1字节)
 *  seq_num: 序列号，通过serial_next_seq_num()函数递增获取
 *  channel: 通道号，默认0x00
 * ============================================================ */
void serial_send_response(uint16_t seq_num, uint8_t channel,
                          uint16_t command, uint8_t status){
    uint8_t arg[3];
    arg[0] = (uint8_t)(command >> 8);
    arg[1] = (uint8_t)(command & 0xFF);
    arg[2] = status;

    serial_send_frame(FRAME_TYPE_HOST_RESP, FRAME_STATUS_NORMAL,
                      seq_num, channel, arg, 3);
}

/* ============================================================
 *  便捷命令函数实现
 *  以下函数封装了各命令的参数格式，方便调用
 * ============================================================ */

/* 查询设备通道数（命令码0x0100，通道0x00）
 * seq_num: 序列号，通过serial_next_seq_num()函数递增获取
 */
void serial_query_channels(uint16_t seq_num){
    uint8_t arg[3];
    arg[0] = 0x01;
    arg[1] = 0x00;
    arg[2] = 0x00;
    serial_send_frame(FRAME_TYPE_HOST_REQ_ACK, FRAME_STATUS_NORMAL,
                      seq_num, CHANNEL_GENERAL, arg, 3);
}

/* 查询设备SN（命令码0x0101，通道0x00）
 * seq_num: 序列号，通过serial_next_seq_num()函数递增获取
 */
void serial_query_sn(uint16_t seq_num){
    uint8_t arg[3];
    arg[0] = 0x01;
    arg[1] = 0x01;
    arg[2] = 0x00;
    serial_send_frame(FRAME_TYPE_HOST_REQ_ACK, FRAME_STATUS_NORMAL,
                      seq_num, CHANNEL_GENERAL, arg, 3);
}

/* 查询固件版本号（命令码0x0102，通道0x00）
 * seq_num: 序列号，通过serial_next_seq_num()函数递增获取
 */
void serial_query_fw_version(uint16_t seq_num){
    uint8_t arg[3];
    arg[0] = 0x01;
    arg[1] = 0x02;
    arg[2] = 0x00;
    serial_send_frame(FRAME_TYPE_HOST_REQ_ACK, FRAME_STATUS_NORMAL,
                      seq_num, CHANNEL_GENERAL, arg, 3);
}

/* 设置系统时间（命令码0x0103，通道0x00）
 * ARG: 保留(1B) + 年(2B大端) + 月(1B) + 日(1B) + 时(1B) + 分(1B) + 秒(1B)
 * seq_num: 序列号，通过serial_next_seq_num()函数递增获取
 */
void serial_set_system_time(uint16_t seq_num,
                            uint16_t year, uint8_t month, uint8_t day,
                            uint8_t hour, uint8_t minute, uint8_t second){
    uint8_t arg[10];
    arg[0] = 0x01;
    arg[1] = 0x03;
    arg[2] = 0x00;
    arg[3] = (uint8_t)(year >> 8);
    arg[4] = (uint8_t)(year & 0xFF);
    arg[5] = month;
    arg[6] = day;
    arg[7] = hour;
    arg[8] = minute;
    arg[9] = second;
    serial_send_frame(FRAME_TYPE_HOST_REQ_ACK, FRAME_STATUS_NORMAL,
                      seq_num, CHANNEL_GENERAL, arg, 10);
}

/* 应答传感器时间请求（帧类型0x04，命令码0x0104）
 * ARG: 保留(1B) + 年(2B大端) + 月(1B) + 日(1B) + 时(1B) + 分(1B) + 秒(1B)
 * seq_num: 序列号，通过serial_next_seq_num()函数递增获取
 */
void serial_resp_system_time(uint16_t seq_num,
                             uint16_t year, uint8_t month, uint8_t day,
                             uint8_t hour, uint8_t minute, uint8_t second){
    uint8_t arg[10];
    arg[0] = 0x01;
    arg[1] = 0x04;
    arg[2] = 0x00;
    arg[3] = (uint8_t)(year >> 8);
    arg[4] = (uint8_t)(year & 0xFF);
    arg[5] = month;
    arg[6] = day;
    arg[7] = hour;
    arg[8] = minute;
    arg[9] = second;
    serial_send_frame(FRAME_TYPE_HOST_RESP, FRAME_STATUS_NORMAL,
                      seq_num, CHANNEL_GENERAL, arg, 10);
}

/* 查询心率/呼吸率/在离床/体动（命令码0x0200）
 * seq_num: 序列号，通过serial_next_seq_num()函数递增获取
 * channel: 通道号，默认0x00
 */
void serial_query_hr_rr_bed(uint16_t seq_num, uint8_t channel){
    uint8_t arg[3];
    arg[0] = 0x02;
    arg[1] = 0x00;
    arg[2] = 0x00;
    serial_send_frame(FRAME_TYPE_HOST_REQ_ACK, FRAME_STATUS_NORMAL,
                      seq_num, channel, arg, 3);
}

/* 设置心率呼吸率等自动上报开关（命令码0x0201）
 * ARG: 命令码(2B) + on(1B)，0x00关 0x01开
 * seq_num: 序列号，通过serial_next_seq_num()函数递增获取
 * channel: 通道号，默认0x00
 */
void serial_set_hr_rr_bed_report(uint16_t seq_num, uint8_t channel, uint8_t on){
    uint8_t arg[3];
    arg[0] = 0x02;
    arg[1] = 0x01;
    arg[2] = on;
    serial_send_frame(FRAME_TYPE_HOST_REQ_ACK, FRAME_STATUS_NORMAL,
                      seq_num, channel, arg, 3);
}

/* 查询睡眠状态（命令码0x0206）
 * seq_num: 序列号，通过serial_next_seq_num()函数递增获取
 * channel: 通道号，默认0x00
 */
void serial_query_sleep_state(uint16_t seq_num, uint8_t channel){
    uint8_t arg[3];
    arg[0] = 0x02;
    arg[1] = 0x06;
    arg[2] = 0x00;
    serial_send_frame(FRAME_TYPE_HOST_REQ_ACK, FRAME_STATUS_NORMAL,
                      seq_num, channel, arg, 3);
}

/* 查询疲劳度（命令码0x0207）
 * seq_num: 序列号，通过serial_next_seq_num()函数递增获取
 * channel: 通道号，默认0x00
 */
void serial_query_fatigue(uint16_t seq_num, uint8_t channel){
    uint8_t arg[3];
    arg[0] = 0x02;
    arg[1] = 0x07;
    arg[2] = 0x00;
    serial_send_frame(FRAME_TYPE_HOST_REQ_ACK, FRAME_STATUS_NORMAL,
                      seq_num, channel, arg, 3);
}

/* 查询憋气状态（命令码0x0208）
 * seq_num: 序列号，通过serial_next_seq_num()函数递增获取
 * channel: 通道号，默认0x00
 */
void serial_query_breath_hold(uint16_t seq_num, uint8_t channel){
    uint8_t arg[3];
    arg[0] = 0x02;
    arg[1] = 0x08;
    arg[2] = 0x00;
    serial_send_frame(FRAME_TYPE_HOST_REQ_ACK, FRAME_STATUS_NORMAL,
                      seq_num, channel, arg, 3);
}

/* 查询情绪压力值（命令码0x0209）
 * seq_num: 序列号，通过serial_next_seq_num()函数递增获取
 * channel: 通道号，默认0x00
 */
void serial_query_stress(uint16_t seq_num, uint8_t channel){
    uint8_t arg[3];
    arg[0] = 0x02;
    arg[1] = 0x09;
    arg[2] = 0x00;
    serial_send_frame(FRAME_TYPE_HOST_REQ_ACK, FRAME_STATUS_NORMAL,
                      seq_num, channel, arg, 3);
}

/* 设置睡眠/疲劳/憋气/压力组数据上报开关（命令码0x020A）
 * ARG: 命令码(2B) + on(1B)，0x00关 0x01开
 * seq_num: 序列号，通过serial_next_seq_num()函数递增获取
 * channel: 通道号，默认0x00
 */
void serial_set_group_report(uint16_t seq_num, uint8_t channel, uint8_t on){
    uint8_t arg[3];
    arg[0] = 0x02;
    arg[1] = 0x0A;
    arg[2] = on;
    serial_send_frame(FRAME_TYPE_HOST_REQ_ACK, FRAME_STATUS_NORMAL,
                      seq_num, channel, arg, 3);
}

/* 设置透传协议开关（命令码0x020C）
 * ARG: 命令码(2B) + on(1B)，0x00关 0x01开
 * seq_num: 序列号，通过serial_next_seq_num()函数递增获取
 * channel: 通道号，默认0x00
 */
void serial_set_transparent(uint16_t seq_num, uint8_t channel, uint8_t on){
    uint8_t arg[3];
    arg[0] = 0x02;
    arg[1] = 0x0C;
    arg[2] = on;
    serial_send_frame(FRAME_TYPE_HOST_REQ_ACK, FRAME_STATUS_NORMAL,
                      seq_num, channel, arg, 3);
}

/* 查询自动上报开关状态（命令码0x020D）
 * seq_num: 序列号，通过serial_next_seq_num()函数递增获取
 * channel: 通道号，默认0x00
 */
void serial_query_switch_status(uint16_t seq_num, uint8_t channel){
    uint8_t arg[3];
    arg[0] = 0x02;
    arg[1] = 0x0D;
    arg[2] = 0x00;
    serial_send_frame(FRAME_TYPE_HOST_REQ_ACK, FRAME_STATUS_NORMAL,
                      seq_num, channel, arg, 3);
}

/* 开启OTA升级（命令码0x0105，通道0x00）
 * ARG: 命令码(2B) + 保留(1B) + 固件总长度(4B大端序)
 * seq_num: 序列号，通过serial_next_seq_num()函数递增获取
 */
void serial_ota_start(uint16_t seq_num, uint32_t fw_size){
    uint8_t arg[7];
    arg[0] = 0x01;
    arg[1] = 0x05;
    arg[2] = 0x00;
    arg[3] = (uint8_t)(fw_size >> 24);
    arg[4] = (uint8_t)(fw_size >> 16);
    arg[5] = (uint8_t)(fw_size >> 8);
    arg[6] = (uint8_t)(fw_size & 0xFF);
    serial_send_frame(FRAME_TYPE_HOST_REQ_ACK, FRAME_STATUS_NORMAL,
                      seq_num, CHANNEL_GENERAL, arg, 7);
}

/* 传输OTA固件分包（命令码0x0106，通道0x00）
 * ARG: 命令码(2B) + 保留(1B) + 固件数据块(packet_len字节)
 * 每帧最大OTA数据长度为1024字节
 * seq_num: 序列号，通过serial_next_seq_num()函数递增获取
 */
void serial_ota_transfer(uint16_t seq_num, const uint8_t *packet_data, uint16_t packet_len){
    uint16_t arg_len;
    uint16_t i;

    if(packet_len > OTA_PACKET_MAX_SIZE){
        return;
    }

    arg_len = 3 + packet_len;
    tx_buf[0] = 0x01;
    tx_buf[1] = 0x06;
    tx_buf[2] = 0x00;

    for(i = 0; i < packet_len; i++){
        tx_buf[3 + i] = packet_data[i];
    }

    serial_send_frame(FRAME_TYPE_HOST_REQ_ACK, FRAME_STATUS_NORMAL,
                      seq_num, CHANNEL_GENERAL, tx_buf, arg_len);
}

/* 结束OTA升级（命令码0x0107，通道0x00）
 * ARG: 命令码(2B) + 保留(1B) + 固件总长度(4B大端) + 固件CRC32(4B大端)
 * seq_num: 序列号，通过serial_next_seq_num()函数递增获取
 */
void serial_ota_end(uint16_t seq_num, uint32_t fw_size, uint32_t fw_crc){
    uint8_t arg[11];
    arg[0] = 0x01;
    arg[1] = 0x07;
    arg[2] = 0x00;
    arg[3] = (uint8_t)(fw_size >> 24);
    arg[4] = (uint8_t)(fw_size >> 16);
    arg[5] = (uint8_t)(fw_size >> 8);
    arg[6] = (uint8_t)(fw_size & 0xFF);
    arg[7] = (uint8_t)(fw_crc >> 24);
    arg[8] = (uint8_t)(fw_crc >> 16);
    arg[9] = (uint8_t)(fw_crc >> 8);
    arg[10] = (uint8_t)(fw_crc & 0xFF);
    serial_send_frame(FRAME_TYPE_HOST_REQ_ACK, FRAME_STATUS_NORMAL,
                      seq_num, CHANNEL_GENERAL, arg, 11);
}