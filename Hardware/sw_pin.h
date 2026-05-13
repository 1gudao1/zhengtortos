#ifndef __SW_PIN_H__
#define __SW_PIN_H__

// sw1到sw11引脚定义
// 按钮逻辑：按钮是常开的，按下时接地
// 未按下时：高电平（通过上拉电阻）
// 按下时：低电平（接地）
#define SW1_PIN_PORT GPIOE
#define SW1_PIN_PIN GPIO_PIN_3
#define SW2_PIN_PORT GPIOC
#define SW2_PIN_PIN GPIO_PIN_13 
#define SW3_PIN_PORT GPIOC
#define SW3_PIN_PIN GPIO_PIN_14
#define SW4_PIN_PORT GPIOC
#define SW4_PIN_PIN GPIO_PIN_15
#define SW5_PIN_PORT GPIOD
#define SW5_PIN_PIN GPIO_PIN_0  
#define SW6_PIN_PORT GPIOD
#define SW6_PIN_PIN GPIO_PIN_1
#define SW7_PIN_PORT GPIOC
#define SW7_PIN_PIN GPIO_PIN_12
#define SW8_PIN_PORT GPIOB
#define SW8_PIN_PIN GPIO_PIN_2
#define SW9_PIN_PORT GPIOE
#define SW9_PIN_PIN GPIO_PIN_7
#define SW10_PIN_PORT GPIOE
#define SW10_PIN_PIN GPIO_PIN_14
#define SW11_PIN_PORT GPIOE
#define SW11_PIN_PIN GPIO_PIN_15 

//按钮数量定义
#define SW_PIN_NUM 11

void sw_pin_init(void);//初始化所有引脚为输入模式，上拉电阻使能
void sw_pin_monitor_all(void);//监控所有按钮状态
uint8_t sw_pin_get_state(uint8_t sw_index);//获取指定按钮状态

// 外部声明按钮状态数组
extern uint8_t sw_pin_states[SW_PIN_NUM];


#endif /* __SW_PIN_H__ */