#ifndef __MOTOR_PIN_H
#define __MOTOR_PIN_H

// 电机引脚结构体定义
typedef struct {
    uint32_t en_port;
    uint32_t en_pin;
    uint32_t in1_port;
    uint32_t in1_pin;
    uint32_t in2_port;
    uint32_t in2_pin;
} MotorPinConfig;

//motor1
#define MOTOR1_EN_PORT GPIOE
#define MOTOR1_EN_PIN GPIO_PIN_4
#define MOTOR1_IN1_PORT GPIOE
#define MOTOR1_IN1_PIN GPIO_PIN_5
#define MOTOR1_IN2_PORT GPIOE
#define MOTOR1_IN2_PIN GPIO_PIN_6   
// motor2
#define MOTOR2_EN_PORT GPIOC
#define MOTOR2_EN_PIN GPIO_PIN_3
#define MOTOR2_IN1_PORT GPIOA
#define MOTOR2_IN1_PIN GPIO_PIN_0
#define MOTOR2_IN2_PORT GPIOA
#define MOTOR2_IN2_PIN GPIO_PIN_1
// motor3   
#define MOTOR3_EN_PORT GPIOA
#define MOTOR3_EN_PIN GPIO_PIN_4
#define MOTOR3_IN1_PORT GPIOA
#define MOTOR3_IN1_PIN GPIO_PIN_2
#define MOTOR3_IN2_PORT GPIOA
#define MOTOR3_IN2_PIN GPIO_PIN_3
// motor4
#define MOTOR4_EN_PORT GPIOA
#define MOTOR4_EN_PIN GPIO_PIN_5
#define MOTOR4_IN1_PORT GPIOA
#define MOTOR4_IN1_PIN GPIO_PIN_6
#define MOTOR4_IN2_PORT GPIOA
#define MOTOR4_IN2_PIN GPIO_PIN_7
// motor5
#define MOTOR5_EN_PORT GPIOC
#define MOTOR5_EN_PIN GPIO_PIN_5
#define MOTOR5_IN1_PORT GPIOB
#define MOTOR5_IN1_PIN GPIO_PIN_0
#define MOTOR5_IN2_PORT GPIOB
#define MOTOR5_IN2_PIN GPIO_PIN_1
// motor6
#define MOTOR6_EN_PORT GPIOE
#define MOTOR6_EN_PIN GPIO_PIN_10
#define MOTOR6_IN1_PORT GPIOE
#define MOTOR6_IN1_PIN GPIO_PIN_11
#define MOTOR6_IN2_PORT GPIOE
#define MOTOR6_IN2_PIN GPIO_PIN_13
// motor7
#define MOTOR7_EN_PORT GPIOE
#define MOTOR7_EN_PIN GPIO_PIN_0
#define MOTOR7_IN1_PORT GPIOB
#define MOTOR7_IN1_PIN GPIO_PIN_9
#define MOTOR7_IN2_PORT GPIOB
#define MOTOR7_IN2_PIN GPIO_PIN_8
// motor8
#define MOTOR8_EN_PORT GPIOB
#define MOTOR8_EN_PIN GPIO_PIN_13
#define MOTOR8_IN1_PORT GPIOB
#define MOTOR8_IN1_PIN GPIO_PIN_14
#define MOTOR8_IN2_PORT GPIOB
#define MOTOR8_IN2_PIN GPIO_PIN_15
// motor9
#define MOTOR9_EN_PORT GPIOD
#define MOTOR9_EN_PIN GPIO_PIN_9
#define MOTOR9_IN1_PORT GPIOD
#define MOTOR9_IN1_PIN GPIO_PIN_12
#define MOTOR9_IN2_PORT GPIOD
#define MOTOR9_IN2_PIN GPIO_PIN_13
// motor10
#define MOTOR10_EN_PORT GPIOD
#define MOTOR10_EN_PIN GPIO_PIN_10
#define MOTOR10_IN1_PORT GPIOD
#define MOTOR10_IN1_PIN GPIO_PIN_14
#define MOTOR10_IN2_PORT GPIOD
#define MOTOR10_IN2_PIN GPIO_PIN_15
// motor11
#define MOTOR11_EN_PORT GPIOD
#define MOTOR11_EN_PIN GPIO_PIN_11
#define MOTOR11_IN1_PORT GPIOC
#define MOTOR11_IN1_PIN GPIO_PIN_6
#define MOTOR11_IN2_PORT GPIOC
#define MOTOR11_IN2_PIN GPIO_PIN_7

//电机标号
#define MOTOR1 1
#define MOTOR2 2
#define MOTOR3 3
#define MOTOR4 4
#define MOTOR5 5
#define MOTOR6 6
#define MOTOR7 7
#define MOTOR8 8
#define MOTOR9 9
#define MOTOR10 10
#define MOTOR11 11

// 电机数量
#define MOTOR_COUNT 11

void motor_pin_init(void);//初始化电机引脚
void motor_control(int motor_num, int time);//控制电机角度
void motor_stop(int motor_num);//电机停止转动
void motor_control_all(int time);//同时控制所有电机
void motor_stop_all(void);//同时停止所有电机
void motor_control_array(int times[]);//使用数组控制所有电机，每个电机可以有不同动作
extern int motor_times[MOTOR_COUNT];
#endif