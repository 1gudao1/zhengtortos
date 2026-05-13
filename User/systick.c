/*!
    \file    systick.c
    \brief   SysTick 配置文件

    \version 2026-2-6, V3.0.3, GD32F30x 固件库
*/

/*
    Copyright (c) 2025, GigaDevice Semiconductor Inc.

    允许在遵守以下条件的前提下，以源代码和二进制形式重新分发和使用，无论是否修改：

    1. 源代码的重新分发必须保留上述版权声明、本条件列表和以下免责声明。
    2. 二进制形式的重新分发必须在随分发提供的文档和/或其他材料中复制上述版权声明、本条件列表和以下免责声明。
    3. 未经版权所有者或其贡献者的事先书面许可，不得使用版权所有者或其贡献者的名称来支持或推广衍生产品。

    本软件由版权所有者和贡献者"按原样"提供，不提供任何明示或暗示的保证，包括但不限于对适销性和特定用途适用性的暗示保证。在任何情况下，版权所有者或贡献者均不对任何直接、间接、偶然、特殊、示范性或后果性损害（包括但不限于采购替代商品或服务；使用、数据或利润损失；或业务中断）承担责任，无论是基于合同、严格责任或其他侵权行为理论，即使已被告知发生此类损害的可能性。
*/

#include "gd32f30x.h"
#include "systick.h"

volatile static uint32_t delay;

/*!
    \brief      配置 SysTick 定时器
    \param[in]  无
    \param[out]  无
    \retval     无
*/
void systick_config(void)
{
    /* 配置 SysTick 定时器产生 1000Hz 中断 */
    if (SysTick_Config(SystemCoreClock / 1000U)){
        /* 捕获错误 */
        while (1){
        }
    }
    /* 配置 SysTick 中断优先级 */
    NVIC_SetPriority(SysTick_IRQn, 0x00U);
}

/*!
    \brief      毫秒级延时函数
    \param[in]  count: 延时的毫秒数
    \param[out]  无
    \retval     无
*/
void delay_1ms(uint32_t count)
{
    delay = count;

    while(0U != delay){
    }
}

/*!
    \brief      延时计数器递减
    \param[in]  无
    \param[out]  无
    \retval     无
*/
void delay_decrement(void)
{
    if (0U != delay){
        delay--;
    }
}
