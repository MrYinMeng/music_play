#ifndef _TIMER_H
#define _TIMER_H
#include "sys.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32F407开发板
//定时器驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2017/4/7
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	

extern u8 ded;
extern u16 led0pwmval;

extern TIM_HandleTypeDef TIM3_Handler;      //定时器句柄 
extern TIM_HandleTypeDef TIM14_Handler; 
extern TIM_HandleTypeDef TIM6_Handler;
extern TIM_HandleTypeDef TIM7_Handler;

void TIM3_Init(u16 arr,u16 psc);
void TIM6_Init(u16 arr,u16 psc);
void TIM7_Init(u16 arr,u16 psc);
void TIM14_PWM_Init(u16 arr,u16 psc);
void TIM_SetTIM14Compare1(u32 compare);
#endif

