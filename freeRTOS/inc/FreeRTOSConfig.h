#define configENABLE_FPU 1
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "gd32f30x.h"

// ==================== 基础配置 ====================
#define configUSE_PREEMPTION                            1
#define configUSE_IDLE_HOOK                             0
#define configUSE_TICK_HOOK                             0
#define configCPU_CLOCK_HZ                              (SystemCoreClock)
#define configTICK_RATE_HZ                              (1000)      // 1ms 滴答
#define configMAX_PRIORITIES                            (5)         // 优先级数量
#define configMINIMAL_STACK_SIZE                        ((unsigned short)128)
#define configTOTAL_HEAP_SIZE                           ((size_t)(64 * 1024))  // 64KB → 96KB 最优
#define configMAX_TASK_NAME_LEN                         (16)
#define configUSE_TRACE_FACILITY                        0
#define configUSE_16_BIT_TICKS                          0
#define configIDLE_SHOULD_YIELD                         1
#define configUSE_MUTEXES                               1
#define configUSE_RECURSIVE_MUTEXES                     1
#define configUSE_COUNTING_SEMAPHORES                   1

// ==================== GD32 中断优先级（关键！） ====================
// GD32 使用 4 位抢占优先级，必须这样配置
#define configPRIO_BITS                                 4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5
#define configKERNEL_INTERRUPT_PRIORITY                 (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY            (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

// ==================== 中断函数映射（必须） ====================
#define vPortSVCHandler        SVC_Handler
#define xPortPendSVHandler     PendSV_Handler
#define xPortSysTickHandler    SysTick_Handler

// ==================== 其他 ====================
#define configCHECK_FOR_STACK_OVERFLOW                  2    // 开启栈溢出检测（开发必备）
#define configASSERT(x)                                 if((x) == 0) {taskDISABLE_INTERRUPTS(); for(;;);}
#define INCLUDE_vTaskDelay                              1
#define INCLUDE_vTaskDelayUntil                         1
#define INCLUDE_vTaskDelete                             1
#define INCLUDE_xTaskGetIdleTaskHandle                  0

#endif