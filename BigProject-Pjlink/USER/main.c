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
 

//��LCD����ʾ��ַ��Ϣ����
//�������ȼ�
#define DISPLAY_TASK_PRIO	8
//�����ջ��С
#define DISPLAY_STK_SIZE	128
//�����ջ
OS_STK	DISPLAY_TASK_STK[DISPLAY_STK_SIZE];
//������
void display_task(void *pdata);

#define UDP_DEMO_PORT			8089	



//LED����
//�������ȼ�
#define LED_TASK_PRIO		9
//�����ջ��С
#define LED_STK_SIZE		1024
//�����ջ
OS_STK	LED_TASK_STK[LED_STK_SIZE];
//������
//void led_task(void *pdata);  
void pjlink_task(void *pdata);

//START����
//�������ȼ�
#define START_TASK_PRIO		10
//�����ջ��С
#define START_STK_SIZE		1024
//�����ջ
OS_STK START_TASK_STK[START_STK_SIZE];
//������
void start_task(void *pdata); 

void Tcp_Client_Init(void);
static err_t tcp_client_connected(void *arg, struct tcp_pcb *pcb, err_t err);
static err_t tcp_client_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *tcp_recv_pbuf, err_t err);

void Udp_Client_Init(void);

	u8 key; 
	int count =0;
	

//��LCD����ʾ��ַ��Ϣ
//mode:1 ��ʾDHCP��ȡ���ĵ�ַ
//	  ���� ��ʾ��̬��ַ
void show_address(u8 mode)
{
	u8 buf[30];
	if(mode==2)
	{
		sprintf((char*)buf,"DHCP IP :%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);						//��ӡ��̬IP��ַ
		LCD_ShowString(30,130,210,16,16,buf); 
		sprintf((char*)buf,"DHCP GW :%d.%d.%d.%d",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);	//��ӡ���ص�ַ
		LCD_ShowString(30,150,210,16,16,buf); 
		sprintf((char*)buf,"NET MASK:%d.%d.%d.%d",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);	//��ӡ���������ַ
		LCD_ShowString(30,170,210,16,16,buf); 
	}
	else 
	{
		sprintf((char*)buf,"Static IP:%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);						//��ӡ��̬IP��ַ
		LCD_ShowString(30,130,210,16,16,buf); 
		sprintf((char*)buf,"Static GW:%d.%d.%d.%d",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);	//��ӡ���ص�ַ
		LCD_ShowString(30,150,210,16,16,buf); 
		sprintf((char*)buf,"NET MASK :%d.%d.%d.%d",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);	//��ӡ���������ַ
		LCD_ShowString(30,170,210,16,16,buf); 
	}	
}

int main(void)
{   
    Stm32_Clock_Init(360,25,2,8);   //����ʱ��,180Mhz   
    HAL_Init();                     //��ʼ��HAL��
    delay_init(180);                //��ʼ����ʱ����
    uart_init(115200);              //��ʼ��USART
    usmart_dev.init(90); 		    //��ʼ��USMART	
    LED_Init();                     //��ʼ��LED 
    KEY_Init();                     //��ʼ������
    SDRAM_Init();                   //��ʼ��SDRAM
    LCD_Init();                     //��ʼ��LCD
    PCF8574_Init();                 //��ʼ��PCF8574
    my_mem_init(SRAMIN);		    //��ʼ���ڲ��ڴ��
	my_mem_init(SRAMEX);		    //��ʼ���ⲿ�ڴ��
	my_mem_init(SRAMCCM);		    //��ʼ��CCM�ڴ��
	POINT_COLOR = RED; 		        //��ɫ����
	LCD_ShowString(30,30,200,20,16,"Apollo STM32F4/F7");
	LCD_ShowString(30,50,200,20,16,"LWIP+UCOSII Test");
	LCD_ShowString(30,70,200,20,16,"ATOM@ALIENTEK");
	LCD_ShowString(30,90,200,20,16,"2016/1/14");
    
	OSInit(); 					    //UCOS��ʼ��

	while(lwip_comm_init()) 	    //lwip��ʼ��
	{
		LCD_ShowString(30,110,200,20,16,"Lwip Init failed!"); 	//lwip��ʼ��ʧ��
		delay_ms(500);
		LCD_Fill(30,110,230,150,WHITE);
		delay_ms(500);
	}
		//Tcp_Client_Init();
	LCD_ShowString(30,110,200,20,16,"Lwip Init Success!"); 		//lwip��ʼ���ɹ�
	OSTaskCreate(start_task,(void*)0,(OS_STK*)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO);
	OSStart(); //����UCOS
	
}

//start����
void start_task(void *pdata)
{
	OS_CPU_SR cpu_sr;
	pdata = pdata ;
	
	OSStatInit();  			//��ʼ��ͳ������
	OS_ENTER_CRITICAL();  	//���ж�
#if LWIP_DHCP
	lwip_comm_dhcp_creat(); //����DHCP����
#endif
	
	// *****************����PJlink����
	OSTaskCreate(pjlink_task,(void*)0,(OS_STK*)&LED_TASK_STK[LED_STK_SIZE-1],LED_TASK_PRIO);
	
	OSTaskSuspend(OS_PRIO_SELF); //����start_task����
	OS_EXIT_CRITICAL();  		//���ж�
}

//��ʾ��ַ����Ϣ
void display_task(void *pdata)
{
	while(1)
	{ 
#if LWIP_DHCP									//������DHCP��ʱ��
		if(lwipdev.dhcpstatus != 0) 			//����DHCP
		{
			show_address(lwipdev.dhcpstatus );	//��ʾ��ַ��Ϣ
			OSTaskSuspend(OS_PRIO_SELF); 		//��ʾ���ַ��Ϣ�������������
		}
#else
		show_address(0); 						//��ʾ��̬��ַ
		OSTaskSuspend(OS_PRIO_SELF); 		 	//��ʾ���ַ��Ϣ�������������
#endif //LWIP_DHCP
		OSTimeDlyHMSM(0,0,0,500);
	}
}


//led����
void led_task(void *pdata)
{
	while(1)
	{
		LED0 = !LED0;
		OSTimeDlyHMSM(0,0,0,500);  //��ʱ500ms
 	}
}



// ************************* pjlink���� 
void pjlink_task(void *pdata)
{
  while(1){
	key = KEY_Scan(0);
		if(key==KEY0_PRES) 		// �������KEY0����ʵ��ѯ��ָ���(TCP)
		{
			Tcp_Client_Init();	
		}	
		else if(key==KEY2_PRES)		// �������KEY2����ʵ�ֿ���ָ���(UDP)
			{
			Udp_Client_Init();
		}		
}
}  

void Tcp_Client_Init(void)
{
        struct tcp_pcb *tcp_client_pcb;
        struct ip_addr ipaddr;     
        IP4_ADDR(&ipaddr, 192, 168, 100, 123); 		// ͶӰ��ip��ַ       
        tcp_client_pcb = tcp_new();     
        tcp_bind(tcp_client_pcb, IP_ADDR_ANY, 4352);		// ͶӰ��PJLINK�˿ں�4352
        if (tcp_client_pcb != NULL)
        {                
            tcp_connect(tcp_client_pcb, &ipaddr, 4352, tcp_client_connected);
        }
}

// ִ��tcp_connect()ʱ��Ļص�����������������з���ѯ����Ϣ
static err_t tcp_client_connected(void *arg, struct tcp_pcb *pcb, err_t err)
{      
    tcp_arg(pcb, mem_calloc(sizeof(struct name), 1));
    tcp_write(pcb, "%1NAME ?\r\n", strlen("%1NAME ?\r\n"), 0);
    tcp_recv(pcb, tcp_client_recv);
    return ERR_OK;
}

// tcp_recv()�Ļص����������ܷ�����Ϣ�����н������
static err_t tcp_client_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *tcp_recv_pbuf, err_t err)
{
    struct pbuf *tcp_send_pbuf;
    struct name *name = (struct name *)arg;
				
        if (tcp_recv_pbuf != NULL)
        {             
            tcp_recved(pcb, tcp_recv_pbuf->tot_len);
					// countֵ���ڹ��˷�����Ϣ��PJLINK0ͷ��Ϣ
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



// ���Ϳ���ָ����
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

