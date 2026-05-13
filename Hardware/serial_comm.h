#ifndef __SERIAL_COMM_H
#define __SERIAL_COMM_H

#include "gd32f30x.h"

/* ============================================================
 *  睡眠监护仪串口通信协议 V1.2
 *  串口参数：115200 bps, 8N1
 *  帧格式：帧头 + 长度 + 版本 + 类型 + 序列号 + 通道 + 数据 + CRC32 + 帧尾
 * ============================================================ */

/* -------------------- 帧固定字段 -------------------- */
#define FRAME_HEADER_HIGH       0xAA        // 帧头高字节
#define FRAME_HEADER_LOW        0x55        // 帧头低字节
#define FRAME_TAIL_HIGH         0xFE        // 帧尾高字节
#define FRAME_TAIL_LOW          0xEF        // 帧尾低字节
#define FRAME_VERSION           0x01        // 当前协议版本

/* -------------------- 帧各字段长度 -------------------- */
#define FRAME_HEADER_SIZE       2           // 帧头
#define FRAME_LENGTH_SIZE       2           // 长度字段
#define FRAME_VERSION_SIZE      1           // 版本字段
#define FRAME_TYPE_SIZE         2           // 类型字段
#define FRAME_SEQNUM_SIZE       2           // 序列号
#define FRAME_CHANNEL_SIZE      1           // 通道字段
#define FRAME_CRC_SIZE          4           // CRC32
#define FRAME_TAIL_SIZE         2           // 帧尾

/* 帧固定开销 = 2+2+1+2+2+1+4+2 = 16字节 */
#define FRAME_OVERHEAD          16
/* 数据域最小长度（命令码2字节） */
#define FRAME_DATA_MIN_SIZE     2
/* 最小帧长度 = 16 + 2 = 18字节 */
#define FRAME_MIN_SIZE          (FRAME_OVERHEAD + FRAME_DATA_MIN_SIZE)
/* OTA分包最大数据长度 */
#define OTA_PACKET_MAX_SIZE     1024
/* 数据域最大长度（OTA 1024 + 命令码2字节） */
#define FRAME_MAX_DATA_SIZE     (OTA_PACKET_MAX_SIZE + 2)
/* 最大帧长度 */
#define FRAME_MAX_SIZE          (FRAME_OVERHEAD + FRAME_MAX_DATA_SIZE)
/* 收发缓冲区大小 */
#define FRAME_BUF_SIZE          (FRAME_MAX_SIZE + 16)

/* -------------------- 帧类型 - 高字节（方向/应答） -------------------- */
#define FRAME_TYPE_HOST_REQ_ACK     0x01    // 上位机→下位机，需要应答
#define FRAME_TYPE_DEV_RESP         0x02    // 下位机→上位机，应答0x01的请求
#define FRAME_TYPE_DEV_REPORT_ACK   0x03    // 下位机主动上报，需要应答
#define FRAME_TYPE_HOST_RESP        0x04    // 上位机→下位机，应答0x03的上报
#define FRAME_TYPE_HOST_REQ_NOACK   0x05    // 上位机→下位机，不需要应答
#define FRAME_TYPE_DEV_REPORT_NOACK 0x06    // 下位机主动上报，不需要应答

/* -------------------- 帧类型 - 低字节（执行状态） -------------------- */
#define FRAME_STATUS_NORMAL         0x00    // 正常/成功
#define FRAME_STATUS_LEN_ERR        0x01    // 数据长度异常
#define FRAME_STATUS_CRC_ERR        0x02    // CRC32校验错误

/* -------------------- 通道定义 -------------------- */
#define CHANNEL_GENERAL     0x00            // 特殊通道，设备基本信息
#define CHANNEL_LEFT        0x01            // 业务通道1（左床/单人）
#define CHANNEL_RIGHT       0x02            // 业务通道2（右床）

/* -------------------- 通用协议命令码（通道0x00） -------------------- */
#define CMD_QUERY_CHANNELS      0x0100      // 查询设备通道数
#define CMD_QUERY_SN            0x0101      // 查询设备SN
#define CMD_QUERY_FW_VERSION    0x0102      // 查询固件版本号
#define CMD_SET_SYSTEM_TIME     0x0103      // 设置系统时间
#define CMD_GET_SYSTEM_TIME     0x0104      // 获取系统时间（下位机发起）
#define CMD_OTA_START           0x0105      // 开启OTA升级
#define CMD_OTA_TRANSFER       0x0106      // 传输固件分包
#define CMD_OTA_END             0x0107      // 结束OTA升级

/* -------------------- 检测参数命令码（通道0x01-0xFF） -------------------- */
#define CMD_QUERY_HR_RR_BED     0x0200      // 查询心率/呼吸率/在离床/体动
#define CMD_HR_RR_BED_SWITCH    0x0201      // 心率呼吸率等自动上报开关
#define CMD_HR_RR_BED_REPORT    0x0202      // 心率呼吸率等组数据上报（下位机主动）
#define CMD_QUERY_AD_DATA       0x0203      // 查询AD采样数据
#define CMD_AD_DATA_SWITCH      0x0204      // AD采样数据上报设置
#define CMD_AD_DATA_REPORT      0x0205      // AD采样数据上报（下位机主动）
#define CMD_QUERY_SLEEP_STATE   0x0206      // 查询睡眠状态
#define CMD_QUERY_FATIGUE       0x0207      // 查询疲劳度状态
#define CMD_QUERY_BREATH_HOLD   0x0208      // 查询憋气状态
#define CMD_QUERY_STRESS        0x0209      // 查询情绪压力值
#define CMD_GROUP_SWITCH        0x020A      // 睡眠/疲劳/憋气/压力组数据开关
#define CMD_GROUP_REPORT        0x020B      // 组数据上报（下位机主动）
#define CMD_TRANSPARENT_SWITCH  0x020C      // 透传协议开关
#define CMD_QUERY_SWITCH_STATUS 0x020D      // 查询自动上报开关状态

/* -------------------- 透传协议命令码 -------------------- */
#define CMD_TRANSPARENT_UP      0x6001      // 上行透传（下位机→上位机）
#define CMD_TRANSPARENT_DOWN    0x6002      // 下行透传（上位机→下位机）

/* -------------------- 睡眠状态枚举 -------------------- */
#define SLEEP_AWAKE     0x00                // 觉醒状态
#define SLEEP_LIGHT     0x01                // 浅睡期
#define SLEEP_DEEP      0x02                // 深睡期
#define SLEEP_REM       0x03                // 快速眼动期
#define SLEEP_LEAVE     0x04                // 离床状态

/* -------------------- 在床状态枚举 -------------------- */
#define BED_LEAVE       0x00                // 离开床
#define BED_STAY        0x01                // 在床无体动
#define BED_MOVE        0x02                // 在床有体动

/* -------------------- 憋气状态枚举 -------------------- */
#define BREATH_NORMAL   0x00                // 正常呼吸
#define BREATH_HOLD     0x01                // 憋气状态
#define BREATH_LEAVE    0x04                // 离床状态

/* ============================================================
 *  协议帧结构体（解析后的帧数据）
 * ============================================================ */
typedef struct {
    uint16_t length;                        // 帧总长度
    uint8_t  version;                       // 协议版本
    uint8_t  type_high;                     // 帧类型高字节（方向）
    uint8_t  type_low;                      // 帧类型低字节（状态）
    uint16_t seq_num;                       // 序列号
    uint8_t  channel;                       // 通道号
    uint16_t command;                       // 命令码
    uint8_t  *data;                         // 数据域指针（指向缓冲区中命令码之后的位置）
    uint16_t data_len;                      // 数据域中命令码之后的数据长度
    uint32_t crc32;                         // CRC32校验值
} ProtocolFrame;

/* ============================================================
 *  函数声明
 * ============================================================ */

/* 初始化串口（USART0, PA9=TX, PA10=RX, 115200 8N1） */
void serial_comm_init(void);

/* CRC32计算（多项式0x04C11DB7，初始值0xFFFFFFFF，结果异或0xFFFFFFFF） */
uint32_t crc32_calc(const uint8_t *data, uint16_t length);

/* 组帧：将各字段打包到out_buf，返回帧总长度 */
uint16_t serial_build_frame(uint8_t type_high, uint8_t type_low,
                            uint16_t seq_num, uint8_t channel,
                            const uint8_t *arg_data, uint16_t arg_len,
                            uint8_t *out_buf);

/* 发送帧：组帧并通过串口发送 */
void serial_send_frame(uint8_t type_high, uint8_t type_low,
                       uint16_t seq_num, uint8_t channel,
                       const uint8_t *arg_data, uint16_t arg_len);

/* 接收帧：从串口接收并解析一帧数据，返回0失败1成功 */
uint8_t serial_receive_frame(ProtocolFrame *frame, uint32_t timeout_ms);

/* 发送应答帧（用于应答下位机0x03类型的上报） */
void serial_send_response(uint16_t seq_num, uint8_t channel,
                          uint16_t command, uint8_t status);

/* 获取并自增序列号 */
uint16_t serial_next_seq_num(void);

/* -------------------- 便捷命令函数 -------------------- */

/* 查询设备通道数 */
void serial_query_channels(uint16_t seq_num);

/* 查询设备SN */
void serial_query_sn(uint16_t seq_num);

/* 查询固件版本号 */
void serial_query_fw_version(uint16_t seq_num);

/* 设置系统时间 */
void serial_set_system_time(uint16_t seq_num,
                            uint16_t year, uint8_t month, uint8_t day,
                            uint8_t hour, uint8_t minute, uint8_t second);

/* 应答下位机时间请求（帧类型0x04） */
void serial_resp_system_time(uint16_t seq_num,
                             uint16_t year, uint8_t month, uint8_t day,
                             uint8_t hour, uint8_t minute, uint8_t second);

/* 查询心率/呼吸率/在离床/体动 */
void serial_query_hr_rr_bed(uint16_t seq_num, uint8_t channel);

/* 设置心率呼吸率等自动上报开关 */
void serial_set_hr_rr_bed_report(uint16_t seq_num, uint8_t channel, uint8_t on);

/* 查询睡眠状态 */
void serial_query_sleep_state(uint16_t seq_num, uint8_t channel);

/* 查询疲劳度 */
void serial_query_fatigue(uint16_t seq_num, uint8_t channel);

/* 查询憋气状态 */
void serial_query_breath_hold(uint16_t seq_num, uint8_t channel);

/* 查询情绪压力值 */
void serial_query_stress(uint16_t seq_num, uint8_t channel);

/* 设置睡眠/疲劳/憋气/压力组数据上报开关 */
void serial_set_group_report(uint16_t seq_num, uint8_t channel, uint8_t on);

/* 设置透传协议开关 */
void serial_set_transparent(uint16_t seq_num, uint8_t channel, uint8_t on);

/* 查询自动上报开关状态 */
void serial_query_switch_status(uint16_t seq_num, uint8_t channel);

/* 开启OTA升级，fw_size为固件总长度 */
void serial_ota_start(uint16_t seq_num, uint32_t fw_size);

/* 传输OTA固件分包，packet_data为分包数据，packet_len为分包长度 */
void serial_ota_transfer(uint16_t seq_num, const uint8_t *packet_data, uint16_t packet_len);

/* 结束OTA升级，fw_size为固件总长度，fw_crc为固件CRC32 */
void serial_ota_end(uint16_t seq_num, uint32_t fw_size, uint32_t fw_crc);

#endif
