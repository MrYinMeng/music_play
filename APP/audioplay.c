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

//音乐播放控制器
__audiodev audiodev;	  
 
//开始音频播放
void audio_start(void)
{
	audiodev.status=3<<0;//开始播放+非暂停
	I2S_Play_Start();
} 
//关闭音频播放
void audio_stop(void)
{
	audiodev.status=0;
	I2S_Play_Stop();
}  
//得到path路径下,目标文件的总个数
//path:路径		    
//返回值:总有效文件数
u16 audio_get_tnum(u8 *path)
{ 
	u8 res;
	u16 rval=0;
 	DIR tdir;	 		//临时目录
	FILINFO* tfileinfo;	//临时文件信息	 	
	tfileinfo=(FILINFO*)mymalloc(SRAMIN,sizeof(FILINFO));//申请内存
    res=f_opendir(&tdir,(const TCHAR*)path); //打开目录 
	if(res==FR_OK&&tfileinfo)
	{
		while(1)//查询总的有效文件数
		{
	        res=f_readdir(&tdir,tfileinfo);       			//读取目录下的一个文件
	        if(res!=FR_OK||tfileinfo->fname[0]==0)break;	//错误了/到末尾了,退出	 		 
			res=f_typetell((u8*)tfileinfo->fname);	
			if((res&0XF0)==0X40)//取高四位,看看是不是音乐文件	
			{
				rval++;//有效文件数增加1
			}
		}  
	}  
	myfree(SRAMIN,tfileinfo);//释放内存
	return rval;
}
//显示曲目索引
//index:当前索引
//total:总文件数
void audio_index_show(u16 index,u16 total)
{
	//显示当前曲目的索引,及总曲目数
	LCD_ShowxNum(60+0,240,index,3,16,0X80,BLACK);		//索引
	LCD_ShowChar(60+24,240,'/',16,0,BLACK);
	LCD_ShowxNum(60+32,240,total,3,16,0X80,BLACK); 	//总曲目				  	  
}
 
//显示播放时间,比特率 信息  
//totsec;音频文件总时间长度
//cursec:当前播放时间
//bitrate:比特率(位速)
void audio_msg_show(u32 totsec,u32 cursec,u32 bitrate)
{
	static u16 playtime=0XFFFF;//播放时间标记	      
	if(playtime!=cursec)					//需要更新显示时间
	{
		z_time = totsec;
		p_time = cursec;
		playtime=cursec;
		//显示播放时间			 
		LCD_ShowxNum(60,190,playtime/60,2,16,0X80,BLACK);		//分钟
		LCD_ShowChar(60+16,190,':',16,0,BLACK);
		LCD_ShowxNum(60+24,190,playtime%60,2,16,0X80,BLACK);	//秒钟		
 		LCD_ShowChar(60+40,190,'/',16,0,BLACK); 
		//显示总时间    	   
 		LCD_ShowxNum(60+48,190,totsec/60,2,16,0X80,BLACK);	//分钟
		LCD_ShowChar(60+64,190,':',16,0,BLACK);
		LCD_ShowxNum(60+72,190,totsec%60,2,16,0X80,BLACK);	//秒钟	  		    
		//显示位率			   
   	LCD_ShowxNum(60+110,190,bitrate/1000,4,16,0X80,BLACK);//显示位率	 
		LCD_ShowString(60+110+32,190,200,16,16,"Kbps",BLACK);	 
		
	} 		 
}
//播放音乐
void audio_play(void)
{
	u8 res;
 	DIR wavdir;	 			//目录
	FILINFO *wavfileinfo;	//文件信息 
	u8 *pname;				//带路径的文件名
	u16 totwavnum; 			//音乐文件总数
	u16 curindex;			//当前索引
	u8 key;					//键值		  
 	u32 temp;
	u32 *wavoffsettbl;		//音乐offset索引表
	
	WM8978_ADDA_Cfg(1,0);	//开启DAC
	WM8978_Input_Cfg(0,0,0);//关闭输入通道
	WM8978_Output_Cfg(1,0);	//开启DAC输出   
	
 	while(f_opendir(&wavdir,"0:/MUSIC"))//打开音乐文件夹
 	{	    
		Show_Str(60,190,240,16,"MUSIC文件夹错误!",16,0,RED);
		delay_ms(200);				  
		LCD_Fill(60,190,240,206,WHITE);//清除显示	     
		delay_ms(200);				  
	} 									  
	totwavnum=audio_get_tnum("0:/MUSIC"); //得到总有效文件数
  	while(totwavnum==NULL)//音乐文件总数为0		
 	{	    
		Show_Str(60,190,240,16,"没有音乐文件!",16,0,RED);
		delay_ms(200);				  
		LCD_Fill(60,190,240,146,WHITE);//清除显示	     
		delay_ms(200);				  
	}										   
	wavfileinfo=(FILINFO*)mymalloc(SRAMIN,sizeof(FILINFO));	//申请内存
  	pname=mymalloc(SRAMIN,_MAX_LFN*2+1);					//为带路径的文件名分配内存
 	wavoffsettbl=mymalloc(SRAMIN,4*totwavnum);				//申请4*totwavnum个字节的内存,用于存放音乐文件off block索引
 	while(!wavfileinfo||!pname||!wavoffsettbl)//内存分配出错
 	{	    
		Show_Str(60,190,240,16,"内存分配失败!",16,0,RED);
		delay_ms(200);				  
		LCD_Fill(60,190,240,146,WHITE);//清除显示	     
		delay_ms(200);				  
	}  	 
 	//记录索引
    res=f_opendir(&wavdir,"0:/MUSIC"); //打开目录
	if(res==FR_OK)
	{
		curindex=0;//当前索引为0
		while(1)//全部查询一遍
		{
			temp=wavdir.dptr;								//记录当前index 
	    res=f_readdir(&wavdir,wavfileinfo);       		//读取目录下的一个文件
	    if(res!=FR_OK||wavfileinfo->fname[0]==0)break;	//错误了/到末尾了,退出 		 
			res=f_typetell((u8*)wavfileinfo->fname);	
			if((res&0XF0)==0X40)//取高四位,看看是不是音乐文件	
			{
				wavoffsettbl[curindex]=temp;//记录索引
				curindex++;
			}
		} 
	}   
   	curindex=0;											//从0开始显示
   	res=f_opendir(&wavdir,(const TCHAR*)"0:/MUSIC"); 	//打开目录
	while(res==FR_OK)//打开成功
	{	
			dir_sdi(&wavdir,wavoffsettbl[curindex]);				//改变当前目录索引	   
					res=f_readdir(&wavdir,wavfileinfo);       				//读取目录下的一个文件
					if(res!=FR_OK||wavfileinfo->fname[0]==0)break;			//错误了/到末尾了,退出		 
			strcpy((char*)pname,"0:/MUSIC/");						//复制路径(目录)
			strcat((char*)pname,(const char*)wavfileinfo->fname);	//将文件名接在后面
			LCD_Fill(60,170,lcddev.width-1,190+16,WHITE);			//清除之前的显示
			Show_Str(60,170,lcddev.width-60,16,(u8*)wavfileinfo->fname,16,0,BLACK);//显示歌曲名字 
			audio_index_show(curindex+1,totwavnum);
			key=audio_play_song(pname); 			 		//播放这个音频文件
			if(key==KEY2_PRES && key_state == 1)		//上一曲if(key==KEY2_PRES)
			{
				
				if(curindex)curindex--;
				else curindex=totwavnum-1;
			}else if(key==KEY0_PRES && key_state == 1)//下一曲if(key==KEY0_PRES)
			{
				
				curindex++;		   	
				if(curindex>=totwavnum)curindex=0;//到末尾的时候,自动从头开始
			}else break;	//产生了错误 	 
	} 
	myfree(SRAMIN,wavfileinfo);			//释放内存			    
	myfree(SRAMIN,pname);				//释放内存			    
	myfree(SRAMIN,wavoffsettbl);		//释放内存 
} 
//播放某个音频文件
u8 audio_play_song(u8* fname)
{
		u8 res;  
		res=f_typetell(fname); 
		switch(res)
		{
			case T_WAV:
				res=wav_play_song(fname);
				break;
			default://其他文件,自动跳转到下一曲
				printf("can't play:%s\r\n",fname);
				res=KEY0_PRES;
				break;
		}
		return res;
}



























