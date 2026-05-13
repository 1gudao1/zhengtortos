/**
  ******************************************************************************
  * @file    sw_pin.c
  * @brief   按钮引脚驱动（优化版本）
  * @author  Auto Generated
  * @version V1.1
  * @date    2026-05-06
  * @note    优化要点：
  *          1. 使用查找表替代 switch-case，消除分支预测失败
  *          2. 直接访问 GPIO_ISTAT 寄存器，消除函数调用开销
  *          3. sw_pin_monitor_all 中端口缓存，减少 64% 总线访问
  ******************************************************************************
  */

#include "gd32f30x.h"
#include "gd32f30x_rcu.h"
#include "sw_pin.h"
#include "systick.h"

/* 按钮状态数组 - 存储所有按钮的当前状态 */
/* 索引 0-10 对应按钮 SW1-SW11 */
uint8_t sw_pin_states[SW_PIN_NUM];

/* GPIO端口查找表 - 用于快速索引按钮对应的GPIO端口 */
/* 编译期常量，存储在 Flash 中，运行时直接读取 */
static const uint32_t sw_port_lut[SW_PIN_NUM] = {
    SW1_PIN_PORT,  SW2_PIN_PORT,  SW3_PIN_PORT,  SW4_PIN_PORT,
    SW5_PIN_PORT,  SW6_PIN_PORT,  SW7_PIN_PORT,  SW8_PIN_PORT,
    SW9_PIN_PORT,  SW10_PIN_PORT, SW11_PIN_PORT
};

/* GPIO引脚查找表 - 用于快速索引按钮对应的引脚掩码 */
static const uint32_t sw_pin_lut[SW_PIN_NUM] = {
    SW1_PIN_PIN,  SW2_PIN_PIN,  SW3_PIN_PIN,  SW4_PIN_PIN,
    SW5_PIN_PIN,  SW6_PIN_PIN,  SW7_PIN_PIN,  SW8_PIN_PIN,
    SW9_PIN_PIN,  SW10_PIN_PIN, SW11_PIN_PIN
};

/**
  * @brief  初始化所有按钮引脚
  * @param  无
  * @retval 无
  * @note   配置按钮引脚为上拉输入模式
  *         按钮为常开型，未按下时通过上拉电阻保持高电平
  *         按下时接地，引脚变为低电平
  */
void sw_pin_init(void){
    /* 使能相关 GPIO 端口时钟 */
    rcu_periph_clock_enable(RCU_GPIOE);
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_GPIOB);

    /* 使用查找表批量初始化按钮引脚 */
    for(uint8_t i = 0; i < SW_PIN_NUM; i++){
        gpio_init(sw_port_lut[i], GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, sw_pin_lut[i]);
    }

    /* 初始化按钮状态数组（初始值为索引值，便于调试） */
    for(uint8_t i = 0; i < SW_PIN_NUM; i++){
        sw_pin_states[i] = i;
    }
}

/**
  * @brief  获取指定按钮的状态
  * @param  sw_index: 按钮编号 (1-11)，对应 SW1-SW11
  * @retval 按钮状态：1=未按下（高电平），0=按下（低电平）
  * @note   使用查找表 + 直接寄存器访问实现快速读取
  *         相比原版本消除了 switch-case 和函数调用开销
  */
uint8_t sw_pin_get_state(uint8_t sw_index){
    /* 参数范围校验 */
    if(sw_index < 1 || sw_index > SW_PIN_NUM){
        return 1;  /* 无效编号返回未按下状态 */
    }
    
    /* 转换为 0-based 索引，直接读取 GPIO_ISTAT 寄存器 */
    uint8_t idx = sw_index - 1;
    return (GPIO_ISTAT(sw_port_lut[idx]) & sw_pin_lut[idx]) ? 1 : 0;
}

/**
  * @brief  批量监测所有按钮状态并更新状态数组
  * @param  无
  * @retval 无
  * @note   
  */
void sw_pin_monitor_all(void){
    /* 缓存各端口的输入状态寄存器值 */
    uint32_t pe_istat = GPIO_ISTAT(GPIOE);
    uint32_t pc_istat = GPIO_ISTAT(GPIOC);
    uint32_t pd_istat = GPIO_ISTAT(GPIOD);
    uint32_t pb_istat = GPIO_ISTAT(GPIOB);

    /* 从缓存中提取各按钮状态 */
    sw_pin_states[0]  = (pe_istat & SW1_PIN_PIN)  ? 1 : 0;   /* SW1: PE3 */
    sw_pin_states[1]  = (pc_istat & SW2_PIN_PIN)  ? 1 : 0;   /* SW2: PC13 */
    sw_pin_states[2]  = (pc_istat & SW3_PIN_PIN)  ? 1 : 0;   /* SW3: PC14 */
    sw_pin_states[3]  = (pc_istat & SW4_PIN_PIN)  ? 1 : 0;   /* SW4: PC15 */
    sw_pin_states[4]  = (pd_istat & SW5_PIN_PIN)  ? 1 : 0;   /* SW5: PD0 */
    sw_pin_states[5]  = (pd_istat & SW6_PIN_PIN)  ? 1 : 0;   /* SW6: PD1 */
    sw_pin_states[6]  = (pc_istat & SW7_PIN_PIN)  ? 1 : 0;   /* SW7: PC12 */
    sw_pin_states[7]  = (pb_istat & SW8_PIN_PIN)  ? 1 : 0;   /* SW8: PB2 */
    sw_pin_states[8]  = (pe_istat & SW9_PIN_PIN)  ? 1 : 0;   /* SW9: PE7 */
    sw_pin_states[9]  = (pe_istat & SW10_PIN_PIN) ? 1 : 0;   /* SW10: PE14 */
    sw_pin_states[10] = (pe_istat & SW11_PIN_PIN) ? 1 : 0;   /* SW11: PE15 */
}