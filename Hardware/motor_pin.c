#include "gd32f30x_rcu.h"
#include "gd32f30x_gpio.h"
#include "gd32f30x.h"
#include "motor_pin.h"
#include "systick.h"
#include "sw_pin.h"
#include "FreeRTOS.h"
#include "task.h"

// 外部引用按钮状态数组
extern uint8_t sw_pin_states[SW_PIN_NUM];
 int motor_times[MOTOR_COUNT];// 电机时间数组
// 电机引脚配置数组
static const MotorPinConfig motor_pins[MOTOR_COUNT] = {
    // 电机1
    {
        .en_port = MOTOR1_EN_PORT,
        .en_pin = MOTOR1_EN_PIN,
        .in1_port = MOTOR1_IN1_PORT,
        .in1_pin = MOTOR1_IN1_PIN,
        .in2_port = MOTOR1_IN2_PORT,
        .in2_pin = MOTOR1_IN2_PIN
    },
    // 电机2
    {
        .en_port = MOTOR2_EN_PORT,
        .en_pin = MOTOR2_EN_PIN,
        .in1_port = MOTOR2_IN1_PORT,
        .in1_pin = MOTOR2_IN1_PIN,
        .in2_port = MOTOR2_IN2_PORT,
        .in2_pin = MOTOR2_IN2_PIN
    },
    // 电机3
    {
        .en_port = MOTOR3_EN_PORT,
        .en_pin = MOTOR3_EN_PIN,
        .in1_port = MOTOR3_IN1_PORT,
        .in1_pin = MOTOR3_IN1_PIN,
        .in2_port = MOTOR3_IN2_PORT,
        .in2_pin = MOTOR3_IN2_PIN
    },
    // 电机4
    {
        .en_port = MOTOR4_EN_PORT,
        .en_pin = MOTOR4_EN_PIN,
        .in1_port = MOTOR4_IN1_PORT,
        .in1_pin = MOTOR4_IN1_PIN,
        .in2_port = MOTOR4_IN2_PORT,
        .in2_pin = MOTOR4_IN2_PIN
    },
    // 电机5
    {
        .en_port = MOTOR5_EN_PORT,
        .en_pin = MOTOR5_EN_PIN,
        .in1_port = MOTOR5_IN1_PORT,
        .in1_pin = MOTOR5_IN1_PIN,
        .in2_port = MOTOR5_IN2_PORT,
        .in2_pin = MOTOR5_IN2_PIN
    },
    // 电机6
    {
        .en_port = MOTOR6_EN_PORT,
        .en_pin = MOTOR6_EN_PIN,
        .in1_port = MOTOR6_IN1_PORT,
        .in1_pin = MOTOR6_IN1_PIN,
        .in2_port = MOTOR6_IN2_PORT,
        .in2_pin = MOTOR6_IN2_PIN
    },
    // 电机7
    {
        .en_port = MOTOR7_EN_PORT,
        .en_pin = MOTOR7_EN_PIN,
        .in1_port = MOTOR7_IN1_PORT,
        .in1_pin = MOTOR7_IN1_PIN,
        .in2_port = MOTOR7_IN2_PORT,
        .in2_pin = MOTOR7_IN2_PIN
    },
    // 电机8
    {
        .en_port = MOTOR8_EN_PORT,
        .en_pin = MOTOR8_EN_PIN,
        .in1_port = MOTOR8_IN1_PORT,
        .in1_pin = MOTOR8_IN1_PIN,
        .in2_port = MOTOR8_IN2_PORT,
        .in2_pin = MOTOR8_IN2_PIN
    },
    // 电机9
    {
        .en_port = MOTOR9_EN_PORT,
        .en_pin = MOTOR9_EN_PIN,
        .in1_port = MOTOR9_IN1_PORT,
        .in1_pin = MOTOR9_IN1_PIN,
        .in2_port = MOTOR9_IN2_PORT,
        .in2_pin = MOTOR9_IN2_PIN
    },
    // 电机10
    {
        .en_port = MOTOR10_EN_PORT,
        .en_pin = MOTOR10_EN_PIN,
        .in1_port = MOTOR10_IN1_PORT,
        .in1_pin = MOTOR10_IN1_PIN,
        .in2_port = MOTOR10_IN2_PORT,
        .in2_pin = MOTOR10_IN2_PIN
    },
    // 电机11
    {
        .en_port = MOTOR11_EN_PORT,
        .en_pin = MOTOR11_EN_PIN,
        .in1_port = MOTOR11_IN1_PORT,
        .in1_pin = MOTOR11_IN1_PIN,
        .in2_port = MOTOR11_IN2_PORT,
        .in2_pin = MOTOR11_IN2_PIN
    }
};

// 初始化电机引脚
void motor_pin_init(void){
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_GPIOE);
    rcu_periph_clock_enable(RCU_GPIOD);
     int i;
    // 遍历配置所有电机引脚
    for(i = 0; i < MOTOR_COUNT; i++){
        gpio_init(motor_pins[i].en_port, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, motor_pins[i].en_pin);
        gpio_init(motor_pins[i].in1_port, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, motor_pins[i].in1_pin);
        gpio_init(motor_pins[i].in2_port, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, motor_pins[i].in2_pin);
    }
}

// 配置电机引脚为推挽输出模式

//电机停止转动
void motor_stop(int motor_num){
    gpio_bit_reset(motor_pins[motor_num - 1].en_port, motor_pins[motor_num - 1].en_pin);
}

// 同时停止所有电机
void motor_stop_all(void){
    int i;
    for(i = 0; i < MOTOR_COUNT; i++){
        gpio_bit_reset(motor_pins[i].en_port, motor_pins[i].en_pin);
    }
}

// 使用数组控制所有电机，每个电机可以有不同动作
// 参数说明：
// times: 包含11个整数的数组，每个整数代表对应电机的动作
//        正数表示正转，负数表示反转，数值表示时间(ms)
//        0表示该电机不动作
// 注意：当对应按钮按下时，该电机停止且不能反转
void motor_control_array(int times[]){
    int i, j;
    int max_time = 0;
    int directions[MOTOR_COUNT];
    int actual_times[MOTOR_COUNT];
    int active_motors = 0;
    
    // 处理每个电机的参数
    for(i = 0; i < MOTOR_COUNT; i++){
        if(times[i] == 0){
            // 该电机不动作
            directions[i] = 0;
            actual_times[i] = 0;
            gpio_bit_reset(motor_pins[i].en_port, motor_pins[i].en_pin);
        }else{
            active_motors++;
            if(times[i] > 0){
                // 正转：允许执行
                directions[i] = 1;
                actual_times[i] = times[i];
            }else{
                // 反转：检查按钮是否按下
                if(sw_pin_states[i] == 0){
                    // 按钮按下，不能反转
                    directions[i] = 0; // 停止
                    actual_times[i] = 0;
                    gpio_bit_reset(motor_pins[i].en_port, motor_pins[i].en_pin);
                }else{
                    directions[i] = -1; // 反转
                    actual_times[i] = -times[i];
                }
            }
            
            // 更新最大时间
            if(actual_times[i] > max_time){
                max_time = actual_times[i];
            }
        }
    }
    
    // 如果没有电机需要动作，直接返回
    if(max_time == 0){
        return;
    }
    
    // 同时启动所有需要动作的电机
    for(i = 0; i < MOTOR_COUNT; i++){
        if(actual_times[i] > 0){
            const MotorPinConfig *motor = &motor_pins[i];
            
            // 使能电机（EN高电平开启）
            gpio_bit_set(motor->en_port, motor->en_pin);
            
            // 设置方向
            if(directions[i] == 1){
                // 正转：IN1=0, IN2=0
                gpio_bit_reset(motor->in1_port, motor->in1_pin);
                gpio_bit_reset(motor->in2_port, motor->in2_pin);
            }else if(directions[i] == -1){
                // 反转：IN1=1, IN2=1
                gpio_bit_set(motor->in1_port, motor->in1_pin);
                gpio_bit_set(motor->in2_port, motor->in2_pin);
            }
        }
    }
    
    // 按毫秒延时，并在每个时间点检查是否有电机需要停止
    for(j = 0; j < max_time; j++){
        vTaskDelay(1);
       
        // 检查是否有电机到达指定时间，需要停止
        for(i = 0; i < MOTOR_COUNT; i++){
            if(actual_times[i] > 0){
                // 检查对应按钮是否按下（反转时按钮按下会停止）
                if(directions[i] == -1 && sw_pin_states[i] == 0){
                    // 反转且按钮按下，立即停止该电机
                    gpio_bit_reset(motor_pins[i].en_port, motor_pins[i].en_pin);
                    actual_times[i] = 0; // 标记为已停止
                }else if(j + 1 >= actual_times[i]){
                    // 该电机已完成动作，停止它
                    gpio_bit_reset(motor_pins[i].en_port, motor_pins[i].en_pin);
                    actual_times[i] = 0; // 标记为已停止
                }
            }
        }
    }
    
    // 确保所有电机都已停止
    motor_stop_all();
}

// 同时控制所有电机
// 参数说明：
// time: 旋转时间(ms)，正数为正转，负数为反转
// 注意：反转时如果对应按钮按下，该电机不能反转
void motor_control_all(int time){
    int direction;
    int i, j;
    int motor_active[MOTOR_COUNT];
    
    // 计算方向
    if(time >= 0){
        direction = 1; // 正转
    }else{
        direction = -1; // 反转
        time = -time; // 取绝对值
    }
    
    // 边界检查
    if(time <= 0){
        return; // 无需旋转
    }
    
    // 同时使能所有电机并设置方向
    for(i = 0; i < MOTOR_COUNT; i++){
        const MotorPinConfig *motor = &motor_pins[i];
        
        // 检查反转时按钮是否按下
        if(direction == -1 && sw_pin_states[i] == 0){
            // 反转且按钮按下，该电机不动作
            motor_active[i] = 0;
            gpio_bit_reset(motor->en_port, motor->en_pin);
        }else{
            // 使能电机（EN高电平开启）
            gpio_bit_set(motor->en_port, motor->en_pin);
            motor_active[i] = 1;
            
            // 设置方向
            if(direction == 1){
                // 正转：IN1=1, IN2=1
                gpio_bit_set(motor->in1_port, motor->in1_pin);
                gpio_bit_set(motor->in2_port, motor->in2_pin);
            }else{
                // 反转：IN1=0, IN2=0
                gpio_bit_reset(motor->in1_port, motor->in1_pin);
                gpio_bit_reset(motor->in2_port, motor->in2_pin);
            }
        }
    }
    
    // 延时指定时间，并在运行时检查按钮状态
    for(j = 0; j < time; j++){
        vTaskDelay(1);
        
        // 检查反转时按钮是否按下
        if(direction == -1){
            for(i = 0; i < MOTOR_COUNT; i++){
                if(motor_active[i] && sw_pin_states[i] == 0){
                    // 反转且按钮按下，立即停止该电机
                    gpio_bit_reset(motor_pins[i].en_port, motor_pins[i].en_pin);
                    motor_active[i] = 0;
                }
            }
        }
    }
    
    // 同时停止所有电机
    motor_stop_all();
}

// 控制电机旋转
// 参数说明：
// motor_num: 电机编号 (1-11)
// time: 旋转时间(ms)，正数为正转，负数为反转
// 控制逻辑：
// EN脚控制启停（高电平启动，低电平停止）
// IN1高电平 IN2高电平 是正转
// IN1低电平 IN2低电平是反转
// 注意：反转时如果对应按钮按下，该电机不能反转
void motor_control(int motor_num, int time){
    int direction;
    int i;
    int motor_index = motor_num - 1;
    
    // 参数验证
    if(motor_num < 1 || motor_num > MOTOR_COUNT){
        return; // 无效电机编号
    }
    
    // 计算方向
    if(time >= 0){
        direction = 1; // 正转
    }else{
        direction = -1; // 反转
        time = -time; // 取绝对值
    }
    
    // 边界检查
    if(time <= 0){
        return; // 无需旋转
    }
    
    // 检查反转时按钮是否按下
    if(direction == -1 && sw_pin_states[motor_index] == 0){
        // 反转且按钮按下，不能反转
        return;
    }
    
    // 获取电机引脚配置
    const MotorPinConfig *motor = &motor_pins[motor_index];
    
    // 使能电机（EN高电平开启）
    gpio_bit_set(motor->en_port, motor->en_pin);
    
    // 设置方向
    if(direction == 1){
        // 正转：IN1=1, IN2=1
        gpio_bit_set(motor->in1_port, motor->in1_pin);
        gpio_bit_set(motor->in2_port, motor->in2_pin);
    }else{
        // 反转：IN1=0, IN2=0
        gpio_bit_reset(motor->in1_port, motor->in1_pin);
        gpio_bit_reset(motor->in2_port, motor->in2_pin);
    }
    
    // 延时指定时间，并在运行时检查按钮状态
    for(i = 0; i < time; i++){
        vTaskDelay(1);
        
        // 检查反转时按钮是否按下
        if(direction == -1 && sw_pin_states[motor_index] == 0){
            // 反转且按钮按下，立即停止该电机
            gpio_bit_reset(motor->en_port, motor->en_pin);
            return;
        }
    }
    
    // 电机保持使能状态（EN为低电平）
    gpio_bit_reset(motor->en_port, motor->en_pin);
}