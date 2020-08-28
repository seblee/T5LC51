/******************************************************************************
 * �� �� ��   : main.c
 * �� �� ��   : V1.0
 * ��    ��   : pinot
 * ��������   : 2019��11��01��
 * ��������   : ������������Ͳ�����ʼ������ѭ������Ҫ���ܺ�����ڡ�
 * �޸���ʷ   :
 * ��    ��   :
 * ��    ��   :
 * �޸�����   :
 ******************************************************************************/

/*****************************************************************************
ϵͳ��*/

/*****************************************************************************
�Զ���ͷ�ļ�*/
#include "T5LOS8051.H"
#include "sys.h"
// #include "handle.h"
//#include "password.h"
#include "uart.h"
#include "timer.h"
#include "rtc.h"
#include "modbus.h"
#include "ui.h"
#include "curve.h"
#include "string.h"
/*****************************************************************************
������*/
const u8 test[] = "���Ժ���";

void main(void)
{
    InitSys();
    init_rtc();  //Ӳ��RTC��ʼ��
    Modbus_UART_Init();
    curveInit();
    WriteDGUS(0x7500, test, sizeof(test));
    while (1)
    {
        WDT_RST();               //ι��
        if (SysTick_RTC >= 500)  //ԭ����500�����Ը�Ϊ50
        {
            rdtime();  //����Ӳ��RTCʱ��
            // Uart4SendStr(test, sizeof(test));
            SysTick_RTC = 0;
        }
        // HandleProc();
        Modbus_Process_Task();  // Modbus���ڴ�������
        RTC_Set_CMD();
        ui();
        curveProcess();
        if (timer1msFlag)
        {
            timer1msFlag   = 0;
            timer100msFlag = 0;
        }
    }
}
