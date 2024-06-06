#include "audioplay.h"
#include "ff.h"
#include "malloc.h"
#include "usart.h"
#include "wm8978.h"
#include "i2s.h"
#include "led.h"
#include "lcd.h"
#include "delay.h"
#include "key.h"
#include "exfuns.h"  
#include "text.h"
#include "string.h"  
#include "timer.h"

u8 pmgressbar = 0;
u8 z_time;
u8 p_time;

//���ֲ��ſ�����
__audiodev audiodev;	  
 
//��ʼ��Ƶ����
void audio_start(void)
{
	audiodev.status=3<<0;//��ʼ����+����ͣ
	I2S_Play_Start();
} 
//�ر���Ƶ����
void audio_stop(void)
{
	audiodev.status=0;
	I2S_Play_Stop();
}  
//�õ�path·����,Ŀ���ļ����ܸ���
//path:·��		    
//����ֵ:����Ч�ļ���
u16 audio_get_tnum(u8 *path)
{ 
	u8 res;
	u16 rval=0;
 	DIR tdir;	 		//��ʱĿ¼
	FILINFO* tfileinfo;	//��ʱ�ļ���Ϣ	 	
	tfileinfo=(FILINFO*)mymalloc(SRAMIN,sizeof(FILINFO));//�����ڴ�
    res=f_opendir(&tdir,(const TCHAR*)path); //��Ŀ¼ 
	if(res==FR_OK&&tfileinfo)
	{
		while(1)//��ѯ�ܵ���Ч�ļ���
		{
	        res=f_readdir(&tdir,tfileinfo);       			//��ȡĿ¼�µ�һ���ļ�
	        if(res!=FR_OK||tfileinfo->fname[0]==0)break;	//������/��ĩβ��,�˳�	 		 
			res=f_typetell((u8*)tfileinfo->fname);	
			if((res&0XF0)==0X40)//ȡ����λ,�����ǲ��������ļ�	
			{
				rval++;//��Ч�ļ�������1
			}
		}  
	}  
	myfree(SRAMIN,tfileinfo);//�ͷ��ڴ�
	return rval;
}
//��ʾ��Ŀ����
//index:��ǰ����
//total:���ļ���
void audio_index_show(u16 index,u16 total)
{
	//��ʾ��ǰ��Ŀ������,������Ŀ��
	LCD_ShowxNum(60+0,240,index,3,16,0X80,BLACK);		//����
	LCD_ShowChar(60+24,240,'/',16,0,BLACK);
	LCD_ShowxNum(60+32,240,total,3,16,0X80,BLACK); 	//����Ŀ				  	  
}
 
//��ʾ����ʱ��,������ ��Ϣ  
//totsec;��Ƶ�ļ���ʱ�䳤��
//cursec:��ǰ����ʱ��
//bitrate:������(λ��)
void audio_msg_show(u32 totsec,u32 cursec,u32 bitrate)
{
	static u16 playtime=0XFFFF;//����ʱ����	      
	if(playtime!=cursec)					//��Ҫ������ʾʱ��
	{
		z_time = totsec;
		p_time = cursec;
		playtime=cursec;
		//��ʾ����ʱ��			 
		LCD_ShowxNum(60,190,playtime/60,2,16,0X80,BLACK);		//����
		LCD_ShowChar(60+16,190,':',16,0,BLACK);
		LCD_ShowxNum(60+24,190,playtime%60,2,16,0X80,BLACK);	//����		
 		LCD_ShowChar(60+40,190,'/',16,0,BLACK); 
		//��ʾ��ʱ��    	   
 		LCD_ShowxNum(60+48,190,totsec/60,2,16,0X80,BLACK);	//����
		LCD_ShowChar(60+64,190,':',16,0,BLACK);
		LCD_ShowxNum(60+72,190,totsec%60,2,16,0X80,BLACK);	//����	  		    
		//��ʾλ��			   
   	LCD_ShowxNum(60+110,190,bitrate/1000,4,16,0X80,BLACK);//��ʾλ��	 
		LCD_ShowString(60+110+32,190,200,16,16,"Kbps",BLACK);	 
		
	} 		 
}
//��������
void audio_play(void)
{
	u8 res;
 	DIR wavdir;	 			//Ŀ¼
	FILINFO *wavfileinfo;	//�ļ���Ϣ 
	u8 *pname;				//��·�����ļ���
	u16 totwavnum; 			//�����ļ�����
	u16 curindex;			//��ǰ����
	u8 key;					//��ֵ		  
 	u32 temp;
	u32 *wavoffsettbl;		//����offset������
	
	WM8978_ADDA_Cfg(1,0);	//����DAC
	WM8978_Input_Cfg(0,0,0);//�ر�����ͨ��
	WM8978_Output_Cfg(1,0);	//����DAC���   
	
 	while(f_opendir(&wavdir,"0:/MUSIC"))//�������ļ���
 	{	    
		Show_Str(60,190,240,16,"MUSIC�ļ��д���!",16,0,RED);
		delay_ms(200);				  
		LCD_Fill(60,190,240,206,WHITE);//�����ʾ	     
		delay_ms(200);				  
	} 									  
	totwavnum=audio_get_tnum("0:/MUSIC"); //�õ�����Ч�ļ���
  	while(totwavnum==NULL)//�����ļ�����Ϊ0		
 	{	    
		Show_Str(60,190,240,16,"û�������ļ�!",16,0,RED);
		delay_ms(200);				  
		LCD_Fill(60,190,240,146,WHITE);//�����ʾ	     
		delay_ms(200);				  
	}										   
	wavfileinfo=(FILINFO*)mymalloc(SRAMIN,sizeof(FILINFO));	//�����ڴ�
  	pname=mymalloc(SRAMIN,_MAX_LFN*2+1);					//Ϊ��·�����ļ��������ڴ�
 	wavoffsettbl=mymalloc(SRAMIN,4*totwavnum);				//����4*totwavnum���ֽڵ��ڴ�,���ڴ�������ļ�off block����
 	while(!wavfileinfo||!pname||!wavoffsettbl)//�ڴ�������
 	{	    
		Show_Str(60,190,240,16,"�ڴ����ʧ��!",16,0,RED);
		delay_ms(200);				  
		LCD_Fill(60,190,240,146,WHITE);//�����ʾ	     
		delay_ms(200);				  
	}  	 
 	//��¼����
    res=f_opendir(&wavdir,"0:/MUSIC"); //��Ŀ¼
	if(res==FR_OK)
	{
		curindex=0;//��ǰ����Ϊ0
		while(1)//ȫ����ѯһ��
		{
			temp=wavdir.dptr;								//��¼��ǰindex 
	    res=f_readdir(&wavdir,wavfileinfo);       		//��ȡĿ¼�µ�һ���ļ�
	    if(res!=FR_OK||wavfileinfo->fname[0]==0)break;	//������/��ĩβ��,�˳� 		 
			res=f_typetell((u8*)wavfileinfo->fname);	
			if((res&0XF0)==0X40)//ȡ����λ,�����ǲ��������ļ�	
			{
				wavoffsettbl[curindex]=temp;//��¼����
				curindex++;
			}
		} 
	}   
   	curindex=0;											//��0��ʼ��ʾ
   	res=f_opendir(&wavdir,(const TCHAR*)"0:/MUSIC"); 	//��Ŀ¼
	while(res==FR_OK)//�򿪳ɹ�
	{	
			dir_sdi(&wavdir,wavoffsettbl[curindex]);				//�ı䵱ǰĿ¼����	   
					res=f_readdir(&wavdir,wavfileinfo);       				//��ȡĿ¼�µ�һ���ļ�
					if(res!=FR_OK||wavfileinfo->fname[0]==0)break;			//������/��ĩβ��,�˳�		 
			strcpy((char*)pname,"0:/MUSIC/");						//����·��(Ŀ¼)
			strcat((char*)pname,(const char*)wavfileinfo->fname);	//���ļ������ں���
			LCD_Fill(60,170,lcddev.width-1,190+16,WHITE);			//���֮ǰ����ʾ
			Show_Str(60,170,lcddev.width-60,16,(u8*)wavfileinfo->fname,16,0,BLACK);//��ʾ�������� 
			audio_index_show(curindex+1,totwavnum);
			key=audio_play_song(pname); 			 		//���������Ƶ�ļ�
			if(key==KEY2_PRES && key_state == 1)		//��һ��if(key==KEY2_PRES)
			{
				
				if(curindex)curindex--;
				else curindex=totwavnum-1;
			}else if(key==KEY0_PRES && key_state == 1)//��һ��if(key==KEY0_PRES)
			{
				
				curindex++;		   	
				if(curindex>=totwavnum)curindex=0;//��ĩβ��ʱ��,�Զ���ͷ��ʼ
			}else break;	//�����˴��� 	 
	} 
	myfree(SRAMIN,wavfileinfo);			//�ͷ��ڴ�			    
	myfree(SRAMIN,pname);				//�ͷ��ڴ�			    
	myfree(SRAMIN,wavoffsettbl);		//�ͷ��ڴ� 
} 
//����ĳ����Ƶ�ļ�
u8 audio_play_song(u8* fname)
{
		u8 res;  
		res=f_typetell(fname); 
		switch(res)
		{
			case T_WAV:
				res=wav_play_song(fname);
				break;
			default://�����ļ�,�Զ���ת����һ��
				printf("can't play:%s\r\n",fname);
				res=KEY0_PRES;
				break;
		}
		return res;
}


























