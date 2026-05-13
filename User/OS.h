#ifndef OS_H
#define OS_H    
//开始任务优先级
#define START_TASK_PRIO		1
//任务堆栈大小	
#define START_STK_SIZE 		128  
//任务句柄
TaskHandle_t StartTask_Handler;
//开始任务任务函数
void start_task(void *pvParameters);

#define USART0_TASK_PRIO		1
//任务堆栈大小	
#define USART0_TASK_STK_SIZE 		256
//任务句柄
TaskHandle_t USART0_Task_Handler;
//USART口0任务函数
void USART0_task_task(void *pvParameters);



#define USART1_TASK_PRIO		1
//任务堆栈大小	
#define USART1_TASK_STK_SIZE 		256  
//任务句柄
TaskHandle_t USART1_Task_Handler;
//USART口1任务函数
void USART1_task_task(void *pvParameters);

//SW任务优先级
#define SW_TASK_PRIO		1
//任务堆栈大小	
#define SW_TASK_STK_SIZE 		64  
//任务句柄
TaskHandle_t SW_Task_Handler;
//SW任务任务函数
void SW_task(void *pvParameters);



void freertos(void);
#endif
