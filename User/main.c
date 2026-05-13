#include "gd32f30x.h"
#include "gd32f30x_rcu.h"
#include "gd32f30x_gpio.h"
#include "systick.h"
#include "../Hardware/motor_pin.h"
#include "../Hardware/sw_pin.h"
#include "../Hardware/serial_comm.h"
#include "../Hardware/sc.h"
#include "stdio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "OS.h"



int main(void)
{ 
    /* 配置系统时钟 */
    systick_config();
    /* 使能GPIOE时钟 */
    rcu_periph_clock_enable(RCU_GPIOE);
    /* 配置PE2为推挽输出模式 */
    gpio_init(GPIOE, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);
    /* 设置PE2为低电平  启动12v电源*/
    gpio_bit_reset(GPIOE, GPIO_PIN_2);
    /* 配置PE12为推挽输出模式 */
    gpio_init(GPIOE, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_12);
    /* 设置PE12初始为低电平 */
    gpio_bit_reset(GPIOE, GPIO_PIN_12);
    /* 初始化电机引脚 */
    motor_pin_init();
    /* 初始化按钮引脚 */
    sw_pin_init();
    /* 初始化串口0 */
    serial_comm_init();
    // 初始化串口1
    sc_init();
    //RTOS启动
    freertos();
}
  
