#ifndef __SC_H__
#define __SC_H__
#include <stdint.h>




#define LOW_ON  0x8A    // 采集开（工作）
#define LOW_OFF 0x88    // 采集关（待机）

//发送数据包结构
typedef struct {
    uint8_t header;              // 数据头，固定为0xFF
    int8_t  BCGdata[128 * 3];    // BCG 体震波形数据
    uint8_t heartrate;           // 心率【每分钟】
    uint8_t rp;                  // 呼吸率【每分钟】
    uint8_t ActivityLevel;       // 体动程度【1-10，越高体动越激烈】
    uint8_t BedExit;             // 在离床判断【0/1 输出】
    uint8_t pr;                  // 压力数据
    uint8_t sw[11];               //电机零位状态
    uint8_t rsv[8];              // 保留位
} SC_PACK;

// 接收数据包结构
typedef struct {
    uint8_t header;              // 数据头，固定为0xFF
    uint8_t cmd;                 // 指令
    int8_t motor[11];           // 电机转动距离    
    uint8_t rsv[8];              // 保留位
} SC_RECV_PACK;
 // 函数声明
void sc_init(void);                           // 初始化串口1
void sc_send_packet(SC_PACK *sc_pack);  // 发送SC_PACK数据包
uint8_t sc_receive_packet(SC_RECV_PACK *packet); // 接收SC_RECV_PACK数据包
extern SC_PACK sc_pack;
extern SC_RECV_PACK sc_recv_pack;

#endif