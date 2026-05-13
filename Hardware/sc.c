#include "sc.h"
#include "gd32f30x_rcu.h"
#include "gd32f30x_gpio.h"
#include "gd32f30x_usart.h"

static uint16_t sc_seq_num = 0;

void sc_init(void){
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_USART1);
    rcu_periph_clock_enable(RCU_AF);
    gpio_pin_remap_config(GPIO_USART1_REMAP, ENABLE);
    gpio_init(GPIOD, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5);
    gpio_init(GPIOD, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_6);
    usart_deinit(USART1);
    usart_baudrate_set(USART1, SC_USART_BAUDRATE);
    usart_word_length_set(USART1, USART_WL_8BIT);
    usart_parity_config(USART1, USART_PM_NONE);
    usart_stop_bit_set(USART1, USART_STB_1BIT);
    usart_hardware_flow_cts_config(USART1, USART_CTS_DISABLE);
    usart_hardware_flow_rts_config(USART1, USART_RTS_DISABLE);
    usart_receive_config(USART1, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART1, USART_TRANSMIT_ENABLE);
    usart_enable(USART1);
    sc_seq_num = 0;
}

uint32_t sc_crc32_calculate(const uint8_t *data, uint16_t length){
    uint32_t crc = SC_CRC32_INIT;
    for(uint16_t i = 0; i < length; i++){
        crc ^= data[i];
        for(uint8_t j = 0; j < 8; j++){
            if(crc & 0x80000000){
                crc = (crc << 1) ^ SC_CRC32_POLY;
            }else{
                crc <<= 1;
            }
        }
    }
    return crc ^ SC_CRC32_XOROUT;
}

void sc_build_packet(SC_Packet *packet, uint16_t type, uint16_t seq, uint16_t command, const uint8_t *data, uint8_t data_len){
    packet->header = SC_FRAME_HEADER;
    packet->version = SC_PROTOCOL_VERSION;
    packet->type = type;
    packet->seq = seq;
    packet->channel = SC_PROTOCOL_CHANNEL;
    packet->data[0] = (command >> 8) & 0xFF;
    packet->data[1] = command & 0xFF;
    if(data != NULL && data_len > 0){
        for(uint8_t i = 0; i < data_len; i++){
            packet->data[2 + i] = data[i];
        }
    }
    packet->length = 14 + data_len;
    uint32_t crc = sc_crc32_calculate((uint8_t *)packet, 10 + data_len);
    packet->crc32 = ((crc & 0xFF000000) >> 24) | ((crc & 0x00FF0000) >> 8) |
                   ((crc & 0x0000FF00) << 8) | ((crc & 0x000000FF) << 24);
    packet->tail = SC_FRAME_TAIL;
}

uint8_t sc_parse_packet(SC_Packet *packet, uint16_t *command, uint8_t *data, uint8_t *data_len){
    if(packet->header != SC_FRAME_HEADER || packet->tail != SC_FRAME_TAIL){
        return 0;
    }
    uint16_t data_field_len = packet->length - 14;
    if(data_field_len < 2){
        return 0;
    }
    *command = (packet->data[0] << 8) | packet->data[1];
    *data_len = data_field_len - 2;
    if(*data_len > 0 && data != NULL){
        for(uint8_t i = 0; i < *data_len; i++){
            data[i] = packet->data[2 + i];
        }
    }
    uint32_t crc_calc = sc_crc32_calculate((uint8_t *)packet, 10 + data_field_len);
    uint32_t crc_received = ((packet->crc32 & 0xFF000000) >> 24) | ((packet->crc32 & 0x00FF0000) >> 8) |
                          ((packet->crc32 & 0x0000FF00) << 8) | ((packet->crc32 & 0x000000FF) << 24);
    if(crc_calc != crc_received){
        return 0;
    }
    return 1;
}

void sc_send_packet(SC_Packet *packet){
    uint8_t *data_ptr = (uint8_t *)packet;
    for(uint16_t i = 0; i < packet->length; i++){
        while(RESET == usart_flag_get(USART1, USART_FLAG_TBE)){}
        usart_data_transmit(USART1, data_ptr[i]);
    }
    while(RESET == usart_flag_get(USART1, USART_FLAG_TC)){}
}

uint8_t sc_receive_packet(SC_Packet *packet){
    uint8_t byte;
    uint16_t index = 0;
    uint8_t header_found = 0;
    while(1){
        if(usart_flag_get(USART1, USART_FLAG_RBNE)){
            byte = usart_data_receive(USART1);
            if(!header_found){
                if(byte == 0xAA){
                    if(usart_flag_get(USART1, USART_FLAG_RBNE)){
                        byte = usart_data_receive(USART1);
                        if(byte == 0x55){
                            packet->header = SC_FRAME_HEADER;
                            index = 2;
                            header_found = 1;
                        }
                    }
                }
            }else{
                if(index == 2){
                    packet->length = byte << 8;
                }else if(index == 3){
                    packet->length |= byte;
                }else if(index == 4){
                    packet->version = byte;
                }else if(index == 5){
                    packet->type = byte << 8;
                }else if(index == 6){
                    packet->type |= byte;
                }else if(index == 7){
                    packet->seq = byte << 8;
                }else if(index == 8){
                    packet->seq |= byte;
                }else if(index == 9){
                    packet->channel = byte;
                }else if(index < packet->length - 6){
                    packet->data[index - 10] = byte;
                }else if(index == packet->length - 6){
                    packet->crc32 = byte << 24;
                }else if(index == packet->length - 5){
                    packet->crc32 |= byte << 16;
                }else if(index == packet->length - 4){
                    packet->crc32 |= byte << 8;
                }else if(index == packet->length - 3){
                    packet->crc32 |= byte;
                }else if(index == packet->length - 2){
                    packet->tail = byte << 8;
                }else if(index == packet->length - 1){
                    packet->tail |= byte;
                    if(packet->tail == SC_FRAME_TAIL){
                        return 1;
                    }else{
                        header_found = 0;
                        index = 0;
                        continue;
                    }
                }
                index++;
            }
        }
    }
}

void sc_send_vital_signs_switch(uint16_t seq, uint8_t on){
    SC_Packet packet;
    uint8_t data[1] = {on};
    sc_build_packet(&packet, SC_TYPE_DEVICE_TO_HOST_ACK, seq, SC_CMD_VITAL_SIGNS_SWITCH, data, 1);
    sc_send_packet(&packet);
}

void sc_send_vital_signs_report(uint16_t seq, uint8_t heart_rate, uint8_t respiratory_rate, uint8_t bed_state){
    SC_Packet packet;
    SC_VitalSignsReport report = {0, heart_rate, respiratory_rate, bed_state};
    sc_build_packet(&packet, SC_TYPE_DEVICE_TO_HOST_NO_ACK, seq, SC_CMD_VITAL_SIGNS_REPORT, (uint8_t *)&report, sizeof(SC_VitalSignsReport));
    sc_send_packet(&packet);
}

void sc_send_ad_switch(uint16_t seq, uint8_t on){
    SC_Packet packet;
    uint8_t data[2] = {on, on};
    sc_build_packet(&packet, SC_TYPE_DEVICE_TO_HOST_ACK, seq, SC_CMD_AD_SWITCH, data, 2);
    sc_send_packet(&packet);
}

void sc_send_ad_report(uint16_t seq, uint32_t sample_time, const int16_t *ad_data){
    SC_Packet packet;
    SC_ADReport report;
    report.reserved = 0;
    report.sample_time = sample_time;
    report.reserved2[0] = 0;
    report.reserved2[1] = 0;
    for(uint16_t i = 0; i < 500; i++){
        report.ad_data[i] = ad_data[i];
    }
    sc_build_packet(&packet, SC_TYPE_DEVICE_TO_HOST_NO_ACK, seq, SC_CMD_AD_REPORT, (uint8_t *)&report, sizeof(SC_ADReport));
    sc_send_packet(&packet);
}

void sc_send_status_switch(uint16_t seq, uint8_t on){
    SC_Packet packet;
    uint8_t data[2] = {on, on};
    sc_build_packet(&packet, SC_TYPE_DEVICE_TO_HOST_ACK, seq, SC_CMD_STATUS_SWITCH, data, 2);
    sc_send_packet(&packet);
}

void sc_send_status_report(uint16_t seq, uint8_t sleep_state, uint8_t fatigue_level, uint8_t breath_hold, uint16_t stress_state, uint8_t bed_state){
    SC_Packet packet;
    SC_StatusReport report = {0, sleep_state, fatigue_level, breath_hold, stress_state, bed_state, 0};
    sc_build_packet(&packet, SC_TYPE_DEVICE_TO_HOST_NO_ACK, seq, SC_CMD_STATUS_REPORT, (uint8_t *)&report, sizeof(SC_StatusReport));
    sc_send_packet(&packet);
}

void sc_send_motor_control_ack(uint16_t seq, uint8_t err){
    SC_Packet packet;
    uint8_t data[1] = {err};
    sc_build_packet(&packet, SC_TYPE_DEVICE_TO_HOST_ACK, seq, SC_CMD_MOTOR_CONTROL, data, 1);
    sc_send_packet(&packet);
}

void sc_send_switch_query(uint16_t seq){
    SC_Packet packet;
    uint8_t data[1] = {0};
    sc_build_packet(&packet, SC_TYPE_DEVICE_TO_HOST_ACK, seq, SC_CMD_SWITCH_QUERY, data, 1);
    sc_send_packet(&packet);
}

void sc_send_switch_query_ack(uint16_t seq, uint8_t vital_signs_switch, uint8_t ad_switch, uint8_t status_switch){
    SC_Packet packet;
    SC_SwitchStatus status = {0, vital_signs_switch, ad_switch, status_switch};
    sc_build_packet(&packet, SC_TYPE_DEVICE_TO_HOST_ACK, seq, SC_CMD_SWITCH_QUERY, (uint8_t *)&status, sizeof(SC_SwitchStatus));
    sc_send_packet(&packet);
}

void sc_send_zero_calibration_ack(uint16_t seq, uint8_t err){
    SC_Packet packet;
    uint8_t data[1] = {err};
    sc_build_packet(&packet, SC_TYPE_DEVICE_TO_HOST_ACK, seq, SC_CMD_ZERO_CALIBRATION, data, 1);
    sc_send_packet(&packet);
}

void sc_send_init_report(uint16_t seq){
    SC_Packet packet;
    uint8_t data[1] = {0x01};
    sc_build_packet(&packet, SC_TYPE_DEVICE_TO_HOST_NO_ACK, seq, SC_CMD_INIT_REPORT, data, 1);
    sc_send_packet(&packet);
}

uint8_t sc_process_vital_signs_switch(const uint8_t *data, uint8_t data_len){
    if(data_len < 1) return 0;
    uint8_t on = data[0];
    return 0;
}

uint8_t sc_process_ad_switch(const uint8_t *data, uint8_t data_len){
    if(data_len < 1) return 0;
    uint8_t on = data[0];
    return 0;
}

uint8_t sc_process_status_switch(const uint8_t *data, uint8_t data_len){
    if(data_len < 1) return 0;
    uint8_t on = data[0];
    return 0;
}

uint8_t sc_process_motor_control(const uint8_t *data, uint8_t data_len){
    if(data_len < 23) return 0;
    uint8_t control_index = data[0];
    int16_t motor_distance[11];
    for(uint8_t i = 0; i < 11; i++){
        motor_distance[i] = (data[1 + i * 2] << 8) | data[2 + i * 2];
    }
    return 0;
}

uint8_t sc_process_switch_query(const uint8_t *data, uint8_t data_len){
    if(data_len < 1) return 0;
    return 0;
}

uint8_t sc_process_zero_calibration(const uint8_t *data, uint8_t data_len){
    if(data_len < 1) return 0;
    return 0;
}