#ifndef __SC_H__
#define __SC_H__
#include <stdint.h>

#define SC_USART_BAUDRATE      115200
#define SC_PROTOCOL_VERSION    0x01
#define SC_PROTOCOL_CHANNEL    0x01
#define SC_FRAME_HEADER        0xAA55
#define SC_FRAME_TAIL          0xFEEF
#define SC_MAX_DATA_LEN        1024
#define SC_CRC32_POLY          0x04C11DB7
#define SC_CRC32_INIT          0xFFFFFFFF
#define SC_CRC32_XOROUT         0xFFFFFFFF

typedef enum {
    SC_TYPE_HOST_TO_DEVICE_REQ = 0x0100,
    SC_TYPE_DEVICE_TO_HOST_ACK = 0x0200,
    SC_TYPE_DEVICE_TO_HOST_REQ = 0x0300,
    SC_TYPE_HOST_TO_DEVICE_ACK = 0x0400,
    SC_TYPE_HOST_TO_DEVICE_NO_ACK = 0x0500,
    SC_TYPE_DEVICE_TO_HOST_NO_ACK = 0x0600
} SC_PacketType;

typedef enum {
    SC_ERR_NONE = 0x00,
    SC_ERR_LENGTH = 0x01,
    SC_ERR_CRC = 0x02
} SC_ErrorCode;

typedef struct {
    uint16_t header;
    uint16_t length;
    uint8_t  version;
    uint16_t type;
    uint16_t seq;
    uint8_t  channel;
    uint8_t  data[SC_MAX_DATA_LEN];
    uint32_t crc32;
    uint16_t tail;
} SC_Packet;

typedef struct {
    uint8_t  heart_rate;
    uint8_t  respiratory_rate;
    uint8_t  bed_state;
    uint8_t  reserved;
} SC_VitalSignsData;

typedef struct {
    uint8_t  reserved;
    uint8_t  heart_rate;
    uint8_t  respiratory_rate;
    uint8_t  bed_state;
} SC_VitalSignsReport;

typedef struct {
    uint8_t  reserved;
    uint32_t sample_time;
    uint16_t reserved2[2];
    int16_t  ad_data[500];
} SC_ADReport;

typedef struct {
    uint8_t  reserved;
    uint8_t  sleep_state;
    uint8_t  fatigue_level;
    uint8_t  breath_hold;
    uint16_t stress_state;
    uint8_t  bed_state;
    uint8_t  reserved2;
} SC_StatusReport;

typedef struct {
    uint8_t  control_index;
    int16_t  motor_distance[11];
} SC_MotorControl;

typedef struct {
    uint8_t  reserved;
    uint8_t  vital_signs_switch;
    uint8_t  ad_switch;
    uint8_t  status_switch;
} SC_SwitchStatus;

typedef struct {
    uint16_t command;
    uint8_t  data_len;
    uint8_t  data[SC_MAX_DATA_LEN - 2];
} SC_DataField;

#define SC_CMD_VITAL_SIGNS_SWITCH    0x0201
#define SC_CMD_VITAL_SIGNS_REPORT    0x0202
#define SC_CMD_AD_SWITCH             0x0204
#define SC_CMD_AD_REPORT             0x0205
#define SC_CMD_STATUS_SWITCH         0x020A
#define SC_CMD_STATUS_REPORT         0x020B
#define SC_CMD_MOTOR_CONTROL         0x020C
#define SC_CMD_SWITCH_QUERY          0x020D
#define SC_CMD_ZERO_CALIBRATION      0x02AA
#define SC_CMD_INIT_REPORT           0x02FF

void sc_init(void);
void sc_send_packet(SC_Packet *packet);
uint8_t sc_receive_packet(SC_Packet *packet);
uint32_t sc_crc32_calculate(const uint8_t *data, uint16_t length);
void sc_build_packet(SC_Packet *packet, uint16_t type, uint16_t seq, uint16_t command, const uint8_t *data, uint8_t data_len);
uint8_t sc_parse_packet(SC_Packet *packet, uint16_t *command, uint8_t *data, uint8_t *data_len);

void sc_send_vital_signs_switch(uint16_t seq, uint8_t on);
void sc_send_vital_signs_report(uint16_t seq, uint8_t heart_rate, uint8_t respiratory_rate, uint8_t bed_state);
void sc_send_ad_switch(uint16_t seq, uint8_t on);
void sc_send_ad_report(uint16_t seq, uint32_t sample_time, const int16_t *ad_data);
void sc_send_status_switch(uint16_t seq, uint8_t on);
void sc_send_status_report(uint16_t seq, uint8_t sleep_state, uint8_t fatigue_level, uint8_t breath_hold, uint16_t stress_state, uint8_t bed_state);
void sc_send_motor_control_ack(uint16_t seq, uint8_t err);
void sc_send_switch_query(uint16_t seq);
void sc_send_switch_query_ack(uint16_t seq, uint8_t vital_signs_switch, uint8_t ad_switch, uint8_t status_switch);
void sc_send_zero_calibration_ack(uint16_t seq, uint8_t err);
void sc_send_init_report(uint16_t seq);

uint8_t sc_process_vital_signs_switch(const uint8_t *data, uint8_t data_len);
uint8_t sc_process_ad_switch(const uint8_t *data, uint8_t data_len);
uint8_t sc_process_status_switch(const uint8_t *data, uint8_t data_len);
uint8_t sc_process_motor_control(const uint8_t *data, uint8_t data_len);
uint8_t sc_process_switch_query(const uint8_t *data, uint8_t data_len);
uint8_t sc_process_zero_calibration(const uint8_t *data, uint8_t data_len);

#endif