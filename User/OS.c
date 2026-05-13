#include "OS.h"
#include "motor_pin.h"
#include "SW.h"


void freertos(void){
 xTaskCreate((TaskFunction_t )start_task,            //任务函数
                (const char*    )"start_task",          //任务名称
                (uint16_t       )START_STK_SIZE,        //任务堆栈大小
                (void*          )NULL,                  //传递给任务函数的参数
                (UBaseType_t    )START_TASK_PRIO,       //任务优先级
                (TaskHandle_t*  )&StartTask_Handler);   //任务句柄              
    vTaskStartScheduler();          //开启任务调度
}

void USART0_task_task(void *pvParameters)
{
    while (1);{
    }
}
void USART1_task_task(void *pvParameters)
{
    while (1);{
    }
}
void SW_task(void *pvParameters)
{
    while (1);{
        sw_pin_monitor_all();
    }
}
//开始任务任务函数
void start_task(void *pvParameters)
{
taskENTER_CRITICAL();//进入临界区
    xTaskCreate((TaskFunction_t )USART0_task_task,            //任务函数
                (const char*    )"USART0_task_task",          //任务名称
                (uint16_t       )USART0_TASK_STK_SIZE,        //任务堆栈大小
                (void*          )NULL,                  //传递给任务函数的参数
                (UBaseType_t    )USART0_TASK_PRIO,       //任务优先级
                (TaskHandle_t*  )&USART0_Task_Handler);   //任务句柄 
    xTaskCreate((TaskFunction_t )USART1_task_task,            //任务函数
                (const char*    )"USART1_task_task",          //任务名称
                (uint16_t       )USART1_TASK_STK_SIZE,        //任务堆栈大小
                (void*          )NULL,                  //传递给任务函数的参数
                (UBaseType_t    )USART1_TASK_PRIO,       //任务优先级
                (TaskHandle_t*  )&USART1_Task_Handler);   //任务句柄 
    xTaskCreate((TaskFunction_t )SW_task,            //任务函数
                (const char*    )"SW_task",          //任务名称
                (uint16_t       )SW_TASK_STK_SIZE,        //任务堆栈大小
                (void*          )NULL,                  //传递给任务函数的参数
                (UBaseType_t    )SW_TASK_PRIO,       //任务优先级
                (TaskHandle_t*  )&SW_Task_Handler);   //任务句柄 
    vTaskDelete(StartTask_Handler); //删除开始任务
taskEXIT_CRITICAL();//退出临界区
}
                

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    while (1);
}