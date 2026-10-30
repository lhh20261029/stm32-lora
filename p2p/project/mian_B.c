/*头文件声明区*/
#include <string.h>
#include "board.h"
#include "tim-board.h"
#include "timer_handles.h"
#include "oledfont.h"
#include "NS_Radio.h"//点对点底层
#include "oledfont.h"//屏幕底层
#include "hal_oled.h"//屏幕底层

/*变量声明区*/
uint8_t Data[255];//接收数组长度最好写255

/**********************************************************************************************
*函数：void Init( void )
*功能：平台初始化
*输入：无
*输出：无
*特殊说明：无
**********************************************************************************************/
void Init() {
    // 开发板平台初始化
    BoardInitMcu();
    BoardInitPeriph();
    setTimer2Callback(Time2Handler);
    Tim2McuInit(1);//定时器初始化，设置定时中断1ms中断一次
	  NS_RadioEventsInit( );//点对点初始化
    NS_RadioInit( 433300000, 20, 1000, 1000);//配置载波频率为433Mhz，发送功率为20，发送超时时间为1000，接收超时时间为1000
	  OLED_Init(); //初始化屏幕
}




/**
 * Main application entry point.
 */
int main( void )
{  
  	Init();  

 
 
	
	while(1)  
		{    
  
	    ReadRadioRxBuffer( Data);//接收A模块发送过来的数值
		OLED_ShowNum(0,0,Data[0],1,10);	//显示光照度第十位
		OLED_ShowNum(8,0,Data[1],1,10);	//显示光照度第个位
		OLED_ShowString(16,0,(uint8_t *)".");//显示小数点
		OLED_ShowNum(24,0,Data[2],1,10);	//显示光照度第一位小数
		OLED_ShowNum(32,0,Data[3],1,10);	//显示光照度第二位小数
		
		OLED_ShowNum(0,2,Data[4],1,10);	//显示温度值第十位
		OLED_ShowNum(8,2,Data[5],1,10);	//显示温度值第个位
		OLED_ShowString(16,2,(uint8_t *)".");//显示小数点
		OLED_ShowNum(24,2,Data[6],1,10);	//显示温度值第一位小数
		OLED_ShowNum(32,2,Data[7],1,10);	//显示温度值第二位小数
			
		OLED_ShowNum(0,4,Data[8],1,10);	//显示湿度值第十位
		OLED_ShowNum(8,4,Data[9],1,10);	//显示湿度值第个位
		OLED_ShowString(16,4,(uint8_t *)".");//显示小数点
		OLED_ShowNum(24,4,Data[10],1,10);	//显示湿度值第一位小数
		OLED_ShowNum(32,4,Data[11],1,10);	//显示湿度值第二位小数
			
			
			
}
		
			
		}