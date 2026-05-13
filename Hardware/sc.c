#include "sc.h"
#include "gd32f30x_rcu.h"
#include "gd32f30x_gpio.h"
#include "gd32f30x_usart.h"
#include "sw_pin.h"

//定义一个SC_PACK结构体变量，用于存储发送的数据包数据
SC_PACK sc_pack; 
 //定义一个SC_RECV_PACK结构体变量，用于存储接收的数据包数据
SC_RECV_PACK sc_recv_pack;
// 外部声明按钮状态数组
extern uint8_t sw_pin_states[SW_PIN_NUM];
//初始化串口1函数
//PD5 为 USART1_TX
//PD6 为 USART1_RX
void sc_init(void){
    // 使能 GPIOD 时钟
    rcu_periph_clock_enable(RCU_GPIOD);
    // 使能 USART1 时钟
    rcu_periph_clock_enable(RCU_USART1);
    rcu_periph_clock_enable(RCU_AF); 
    gpio_pin_remap_config(GPIO_USART1_REMAP, ENABLE);
    // 配置 PD5 为 USART1_TX
    gpio_init(GPIOD, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5);
    // 配置 PD6 为 USART1_RX
    gpio_init(GPIOD, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_6); 
    // 配置 USART1，波特率 115200，8位数据位，无校验位，1位停止位
    usart_deinit(USART1);
    usart_baudrate_set(USART1, 115200);
    usart_word_length_set(USART1, USART_WL_8BIT);
    usart_parity_config(USART1, USART_PM_NONE);
    usart_stop_bit_set(USART1, USART_STB_1BIT);
    usart_hardware_flow_cts_config(USART1, USART_CTS_DISABLE);
    usart_hardware_flow_rts_config(USART1, USART_RTS_DISABLE);
    usart_receive_config(USART1, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART1, USART_TRANSMIT_ENABLE);
    usart_enable(USART1);
       
    //初始化这个SC_PACK结构体变量，将所有字段设为0xFF
    sc_pack.header = 0xFF;
    sc_pack.heartrate = 0x01;
    sc_pack.rp = 0x02;
    sc_pack.ActivityLevel = 0xFF;
    sc_pack.BedExit = 0xFF;
    sc_pack.pr = 0xFF;
    for(uint16_t i = 0; i <128*3; i++){
        sc_pack.BCGdata[i] = 0xFF;
    }
    for(uint8_t i = 0; i < 11; i++){
        sc_pack.sw[i] = 0xFF;
    }
    for(uint8_t i = 0; i < 8; i++){
        sc_pack.rsv[i] = 0xFF;
    }
   
    //初始化这个SC_RECV_PACK结构体变量，将所有字段设为0xFF
    sc_recv_pack.header = 0xFF;
    sc_recv_pack.cmd = 0xFF;
    for(uint8_t i = 0; i < 11; i++){
        sc_recv_pack.motor[i] = 0xFF;
    }
    for(uint8_t i = 0; i < 8; i++){
        sc_recv_pack.rsv[i] = 0xFF;
    }
}




// 发送数据包函数
void sc_send_packet(SC_PACK *packet){
    // 等待发送缓冲区为空
    while(RESET == usart_flag_get(USART1, USART_FLAG_TBE)){
    }
    // 发送数据包
    /*usart_data_transmit(USART1, packet->header);
    for(uint16_t i = 0; i <128*3; i++){
        usart_data_transmit(USART1, packet->BCGdata[i]);
    }*/
    usart_data_transmit(USART1, packet->heartrate);
    usart_data_transmit(USART1, packet->rp);
    usart_data_transmit(USART1, packet->ActivityLevel);
    usart_data_transmit(USART1, packet->BedExit);
    usart_data_transmit(USART1, packet->pr);
    
    for(uint8_t i = 0; i < 11; i++){
        usart_data_transmit(USART1, packet->sw[i]);
    }
    for(uint8_t i = 0; i < 8; i++){
        usart_data_transmit(USART1, packet->rsv[i]);
    }
    // 等待发送完成
    while(RESET == usart_flag_get(USART1, USART_FLAG_TC)){
    }
}
// 接收数据包函数
uint8_t sc_receive_packet(SC_RECV_PACK *packet){
    uint16_t i = 0;
    uint8_t *data_ptr = (uint8_t *)packet;
    
    // 查找数据包头部 0xFF
    while(1){
        if(usart_flag_get(USART1, USART_FLAG_RBNE)){
            uint8_t byte = usart_data_receive(USART1);
            if(byte == 0xFF){
                data_ptr[0] = byte;
                i = 1;
                break;
            }
        }
    }
    
    // 接收数据包剩余部分
    while(i < sizeof(SC_RECV_PACK)){
        if(usart_flag_get(USART1, USART_FLAG_RBNE)){
            data_ptr[i] = usart_data_receive(USART1);
            i++;
        }
    }
    
    // 验证数据包头部是否正确
    if(packet->header == 0xFF){
        return 1;
    }else{
        return 0;
    }
}

