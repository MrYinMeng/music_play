#include "timer.h"
#include "delay.h"
#include "led.h"
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32F407������
//��ʱ���ж���������	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2017/4/7
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2014-2024
//All rights reserved	
////////////////////////////////////////////////////////////////////////////////// 	
//�޸�˵��
//V1.1 20170407
//����TIM14_PWM_Init����,����PWM���
////////////////////////////////////////////////////////////////////////////////// 

u8 ded = 1;
u16 led0pwmval = 0; 

TIM_HandleTypeDef TIM3_Handler;      	//��ʱ����� 
TIM_HandleTypeDef TIM14_Handler;      	//��ʱ����� 
TIM_HandleTypeDef TIM6_Handler;
TIM_HandleTypeDef TIM7_Handler;
TIM_OC_InitTypeDef TIM14_CH1Handler;	//��ʱ��14ͨ��1���

//ͨ�ö�ʱ��3�жϳ�ʼ��
//arr���Զ���װֵ��
//psc��ʱ��Ԥ��Ƶ��
//��ʱ�����ʱ����㷽��:Tout=((arr+1)*(psc+1))/Ft us.
//Ft=��ʱ������Ƶ��,��λ:Mhz
//����ʹ�õ��Ƕ�ʱ��3!(��ʱ��3����APB1�ϣ�ʱ��ΪHCLK/2)
void TIM3_Init(u16 arr,u16 psc)
{  
    TIM3_Handler.Instance=TIM3;                          //ͨ�ö�ʱ��3
    TIM3_Handler.Init.Prescaler=psc;                     //��Ƶϵ��
    TIM3_Handler.Init.CounterMode=TIM_COUNTERMODE_UP;    //���ϼ�����
    TIM3_Handler.Init.Period=arr;                        //�Զ�װ��ֵ
    TIM3_Handler.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;//ʱ�ӷ�Ƶ����
    HAL_TIM_Base_Init(&TIM3_Handler);
    
    HAL_TIM_Base_Start_IT(&TIM3_Handler); //ʹ�ܶ�ʱ��3�Ͷ�ʱ��3�����жϣ�TIM_IT_UPDATE   
}

void TIM6_Init(u16 arr,u16 psc)
{
	TIM6_Handler.Instance = TIM6;
	TIM6_Handler.Init.Prescaler = psc;
	TIM6_Handler.Init.CounterMode = TIM_COUNTERMODE_UP;
	TIM6_Handler.Init.Period = arr;
	TIM6_Handler.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	HAL_TIM_Base_Init(&TIM6_Handler);
	
	HAL_TIM_Base_Start_IT(&TIM6_Handler);
}

void TIM7_Init(u16 arr,u16 psc)
{
	TIM7_Handler.Instance = TIM7;
	TIM7_Handler.Init.Prescaler = psc;
	TIM7_Handler.Init.CounterMode = TIM_COUNTERMODE_UP;
	TIM7_Handler.Init.Period = arr;
	TIM7_Handler.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	HAL_TIM_Base_Init(&TIM7_Handler);
	
	HAL_TIM_Base_Start_IT(&TIM7_Handler);
}

//TIM14 PWM���ֳ�ʼ�� 
//arr���Զ���װֵ��
//psc��ʱ��Ԥ��Ƶ��
//��ʱ�����ʱ����㷽��:Tout=((arr+1)*(psc+1))/Ft us.
//Ft=��ʱ������Ƶ��,��λ:Mhz
void TIM14_PWM_Init(u16 arr,u16 psc)
{  
    TIM14_Handler.Instance=TIM14;          	//��ʱ��14
    TIM14_Handler.Init.Prescaler=psc;       //��ʱ����Ƶ
    TIM14_Handler.Init.CounterMode=TIM_COUNTERMODE_UP;//���ϼ���ģʽ
    TIM14_Handler.Init.Period=arr;          //�Զ���װ��ֵ
    TIM14_Handler.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_PWM_Init(&TIM14_Handler);       //��ʼ��PWM
    
    TIM14_CH1Handler.OCMode=TIM_OCMODE_PWM1; //ģʽѡ��PWM1
    TIM14_CH1Handler.Pulse=arr/2;            //���ñȽ�ֵ,��ֵ����ȷ��ռ�ձȣ�Ĭ�ϱȽ�ֵΪ�Զ���װ��ֵ��һ��,��ռ�ձ�Ϊ50%
    TIM14_CH1Handler.OCPolarity=TIM_OCPOLARITY_LOW; //����Ƚϼ���Ϊ�� 
    HAL_TIM_PWM_ConfigChannel(&TIM14_Handler,&TIM14_CH1Handler,TIM_CHANNEL_1);//����TIM14ͨ��1
	
    HAL_TIM_PWM_Start(&TIM14_Handler,TIM_CHANNEL_1);//����PWMͨ��1
}


//��ʱ���ײ�����������ʱ�ӣ������ж����ȼ�
//�˺����ᱻHAL_TIM_Base_Init()��������
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
  if(htim->Instance == TIM3)
	{
		__HAL_RCC_TIM3_CLK_ENABLE();            //ʹ��TIM3ʱ��
		HAL_NVIC_SetPriority(TIM3_IRQn,0,3);    //�����ж����ȼ�����ռ���ȼ�0�������ȼ�3
		HAL_NVIC_EnableIRQ(TIM3_IRQn);          //����ITM3�ж�   
	}
	else if(htim->Instance == TIM6)
	{
		__HAL_RCC_TIM6_CLK_ENABLE();            //ʹ��TIM6ʱ��
		HAL_NVIC_SetPriority(TIM6_DAC_IRQn,1,3);//�����ж����ȼ�����ռ���ȼ�1�������ȼ�3
		HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);      //����ITM6�ж�   
	}
	else if(htim->Instance == TIM7)
	{
		__HAL_RCC_TIM7_CLK_ENABLE();            //ʹ��TIM7ʱ��
		HAL_NVIC_SetPriority(TIM7_IRQn,2,3);    //�����ж����ȼ�����ռ���ȼ�2�������ȼ�3
		HAL_NVIC_EnableIRQ(TIM7_IRQn);          //����ITM7�ж�   
	}
}

//��ʱ���ײ�������ʱ��ʹ�ܣ���������
//�˺����ᱻHAL_TIM_PWM_Init()����
//htim:��ʱ�����
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
	GPIO_InitTypeDef GPIO_Initure;
	__HAL_RCC_TIM14_CLK_ENABLE();			//ʹ�ܶ�ʱ��14
	__HAL_RCC_GPIOF_CLK_ENABLE();			//����GPIOFʱ��
	
	GPIO_Initure.Pin=GPIO_PIN_9;           	//PF9
	GPIO_Initure.Mode=GPIO_MODE_AF_PP;  	//�����������
	GPIO_Initure.Pull=GPIO_PULLUP;          //����
	GPIO_Initure.Speed=GPIO_SPEED_HIGH;     //����
	GPIO_Initure.Alternate= GPIO_AF9_TIM14;	//PF9����ΪTIM14_CH1
	HAL_GPIO_Init(GPIOF,&GPIO_Initure);
}

//����TIMͨ��4��ռ�ձ�
//compare:�Ƚ�ֵ
void TIM_SetTIM14Compare1(u32 compare)
{
	TIM14->CCR1=compare; 
}


//��ʱ���жϷ�����
void TIM3_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&TIM3_Handler);
}

void TIM6_DAC_IRQHandler(void)
{
	HAL_TIM_IRQHandler(&TIM6_Handler);
}

void TIM7_IRQHandler(void)
{
	HAL_TIM_IRQHandler(&TIM7_Handler);
}

//�ص���������ʱ���жϷ���������
//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
//{
//    
//}
