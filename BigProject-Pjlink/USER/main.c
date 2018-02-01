#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "lcd.h"
#include "sdram.h"
#include "lan8720.h"
#include "timer.h"
#include "pcf8574.h"
#include "usmart.h"
#include "malloc.h"
#include "lwip/netif.h"
#include "lwip_comm.h"
#include "lwipopts.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include "tcp_client.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
 

//在LCD上显示地址信息任务
//任务优先级
#define DISPLAY_TASK_PRIO	8
//任务堆栈大小
#define DISPLAY_STK_SIZE	128
//任务堆栈
OS_STK	DISPLAY_TASK_STK[DISPLAY_STK_SIZE];
//任务函数
void display_task(void *pdata);

#define UDP_DEMO_PORT			8089	



//LED任务
//任务优先级
#define LED_TASK_PRIO		9
//任务堆栈大小
#define LED_STK_SIZE		1024
//任务堆栈
OS_STK	LED_TASK_STK[LED_STK_SIZE];
//任务函数
//void led_task(void *pdata);  
void pjlink_task(void *pdata);

//START任务
//任务优先级
#define START_TASK_PRIO		10
//任务堆栈大小
#define START_STK_SIZE		1024
//任务堆栈
OS_STK START_TASK_STK[START_STK_SIZE];
//任务函数
void start_task(void *pdata); 

void Tcp_Client_Init(void);
static err_t tcp_client_connected(void *arg, struct tcp_pcb *pcb, err_t err);
static err_t tcp_client_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *tcp_recv_pbuf, err_t err);

void Udp_Client_Init(void);

	u8 key; 
	int count =0;
	

//在LCD上显示地址信息
//mode:1 显示DHCP获取到的地址
//	  其他 显示静态地址
void show_address(u8 mode)
{
	u8 buf[30];
	if(mode==2)
	{
		sprintf((char*)buf,"DHCP IP :%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);						//打印动态IP地址
		LCD_ShowString(30,130,210,16,16,buf); 
		sprintf((char*)buf,"DHCP GW :%d.%d.%d.%d",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);	//打印网关地址
		LCD_ShowString(30,150,210,16,16,buf); 
		sprintf((char*)buf,"NET MASK:%d.%d.%d.%d",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);	//打印子网掩码地址
		LCD_ShowString(30,170,210,16,16,buf); 
	}
	else 
	{
		sprintf((char*)buf,"Static IP:%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);						//打印动态IP地址
		LCD_ShowString(30,130,210,16,16,buf); 
		sprintf((char*)buf,"Static GW:%d.%d.%d.%d",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);	//打印网关地址
		LCD_ShowString(30,150,210,16,16,buf); 
		sprintf((char*)buf,"NET MASK :%d.%d.%d.%d",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);	//打印子网掩码地址
		LCD_ShowString(30,170,210,16,16,buf); 
	}	
}

int main(void)
{   
    Stm32_Clock_Init(360,25,2,8);   //设置时钟,180Mhz   
    HAL_Init();                     //初始化HAL库
    delay_init(180);                //初始化延时函数
    uart_init(115200);              //初始化USART
    usmart_dev.init(90); 		    //初始化USMART	
    LED_Init();                     //初始化LED 
    KEY_Init();                     //初始化按键
    SDRAM_Init();                   //初始化SDRAM
    LCD_Init();                     //初始化LCD
    PCF8574_Init();                 //初始化PCF8574
    my_mem_init(SRAMIN);		    //初始化内部内存池
	my_mem_init(SRAMEX);		    //初始化外部内存池
	my_mem_init(SRAMCCM);		    //初始化CCM内存池
	POINT_COLOR = RED; 		        //红色字体
	LCD_ShowString(30,30,200,20,16,"Apollo STM32F4/F7");
	LCD_ShowString(30,50,200,20,16,"LWIP+UCOSII Test");
	LCD_ShowString(30,70,200,20,16,"ATOM@ALIENTEK");
	LCD_ShowString(30,90,200,20,16,"2016/1/14");
    
	OSInit(); 					    //UCOS初始化

	while(lwip_comm_init()) 	    //lwip初始化
	{
		LCD_ShowString(30,110,200,20,16,"Lwip Init failed!"); 	//lwip初始化失败
		delay_ms(500);
		LCD_Fill(30,110,230,150,WHITE);
		delay_ms(500);
	}
		//Tcp_Client_Init();
	LCD_ShowString(30,110,200,20,16,"Lwip Init Success!"); 		//lwip初始化成功
	OSTaskCreate(start_task,(void*)0,(OS_STK*)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO);
	OSStart(); //开启UCOS
	
}

//start任务
void start_task(void *pdata)
{
	OS_CPU_SR cpu_sr;
	pdata = pdata ;
	
	OSStatInit();  			//初始化统计任务
	OS_ENTER_CRITICAL();  	//关中断
#if LWIP_DHCP
	lwip_comm_dhcp_creat(); //创建DHCP任务
#endif
	
	// *****************创建PJlink任务
	OSTaskCreate(pjlink_task,(void*)0,(OS_STK*)&LED_TASK_STK[LED_STK_SIZE-1],LED_TASK_PRIO);
	
	OSTaskSuspend(OS_PRIO_SELF); //挂起start_task任务
	OS_EXIT_CRITICAL();  		//开中断
}

//显示地址等信息
void display_task(void *pdata)
{
	while(1)
	{ 
#if LWIP_DHCP									//当开启DHCP的时候
		if(lwipdev.dhcpstatus != 0) 			//开启DHCP
		{
			show_address(lwipdev.dhcpstatus );	//显示地址信息
			OSTaskSuspend(OS_PRIO_SELF); 		//显示完地址信息后挂起自身任务
		}
#else
		show_address(0); 						//显示静态地址
		OSTaskSuspend(OS_PRIO_SELF); 		 	//显示完地址信息后挂起自身任务
#endif //LWIP_DHCP
		OSTimeDlyHMSM(0,0,0,500);
	}
}


//led任务
void led_task(void *pdata)
{
	while(1)
	{
		LED0 = !LED0;
		OSTimeDlyHMSM(0,0,0,500);  //延时500ms
 	}
}



// ************************* pjlink任务 
void pjlink_task(void *pdata)
{
  while(1){
	key = KEY_Scan(0);
		if(key==KEY0_PRES) 		// 如果按下KEY0键，实现询问指令发送(TCP)
		{
			Tcp_Client_Init();	
		}	
		else if(key==KEY2_PRES)		// 如果按下KEY2键，实现控制指令发送(UDP)
			{
			Udp_Client_Init();
		}		
}
}  

void Tcp_Client_Init(void)
{
        struct tcp_pcb *tcp_client_pcb;
        struct ip_addr ipaddr;     
        IP4_ADDR(&ipaddr, 192, 168, 100, 123); 		// 投影机ip地址       
        tcp_client_pcb = tcp_new();     
        tcp_bind(tcp_client_pcb, IP_ADDR_ANY, 4352);		// 投影仪PJLINK端口号4352
        if (tcp_client_pcb != NULL)
        {                
            tcp_connect(tcp_client_pcb, &ipaddr, 4352, tcp_client_connected);
        }
}

// 执行tcp_connect()时候的回调函数，在这个函数中发送询问信息
static err_t tcp_client_connected(void *arg, struct tcp_pcb *pcb, err_t err)
{      
    tcp_arg(pcb, mem_calloc(sizeof(struct name), 1));
    tcp_write(pcb, "%1NAME ?\r\n", strlen("%1NAME ?\r\n"), 0);
    tcp_recv(pcb, tcp_client_recv);
    return ERR_OK;
}

// tcp_recv()的回调函数，接受返回信息，进行解释输出
static err_t tcp_client_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *tcp_recv_pbuf, err_t err)
{
    struct pbuf *tcp_send_pbuf;
    struct name *name = (struct name *)arg;
				
        if (tcp_recv_pbuf != NULL)
        {             
            tcp_recved(pcb, tcp_recv_pbuf->tot_len);
					// count值用于过滤返回信息的PJLINK0头信息
					  if(count==1){
						 printf("return value is : %s\n",(char *)tcp_recv_pbuf->payload);
						 count=0;
						}
						else count++;				
        if (!name)
        {
             pbuf_free(tcp_recv_pbuf);
             return ERR_ARG;
           }             
            tcp_send_pbuf = tcp_recv_pbuf;             
            tcp_write(pcb, tcp_send_pbuf->payload, tcp_send_pbuf->len, 1);             
            tcp_write(pcb, "\r\n", strlen("\r\n"), 1);
            pbuf_free(tcp_recv_pbuf);
        }
        else if (err == ERR_OK)
        {                
            mem_free(name);
            return tcp_close(pcb);
        }
        return ERR_OK;
}



// 发送控制指令信
void Udp_Client_Init(void){
   struct pbuf *q = NULL;  
   const char* reply = "%1POWR 0\r\n";  
   struct udp_pcb *upcb;  
   struct ip_addr addr;  
   IP4_ADDR(&addr, 192,168,100,123);  
  
   upcb = udp_new();  
   udp_bind(upcb, IP_ADDR_ANY, UDP_DEMO_PORT);  
   q = pbuf_alloc(PBUF_TRANSPORT, strlen(reply)+1, PBUF_RAM);  
   if(!q)  
   {  
     printf("out of PBUF_RAM\n");  
     return;  
   }  
   memset(q->payload, 0 , q->len);  
   memcpy(q->payload, reply, strlen(reply));
		while(1){
		udp_sendto(upcb, q, &addr, 4352);
		}			
   pbuf_free(q); 
}

