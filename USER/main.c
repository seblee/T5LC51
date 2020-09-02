/*******************************************************************
 * @Warning      : Without permission from the author,Not for commercial use
 * @File         : main.c
 * @Author       : xiaowine
 * @Date         : 2020-08-14 18:29:09
 * @version      : v01.00
 * @ **************************************************************
 * @LastEditTime : 2020-09-02 17:28:55
 * @LastEditors  : xiaowine
 * @ **************************************************************
 * @brief        : ������������Ͳ�����ʼ������ѭ������Ҫ���ܺ�����ڡ�
 * @Description  :
 * @FilePath     : \T5L_C51_PRJ\USER\main.c
 * @ **************************************************************
 * @attention    :
 * @Powered By xiaowine
 * @<h2><center>&copy;  Copyright(C) cee0.com 2020</center></h2>
 * @All rights reserved
 ******************************************************************/

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
#include "alarm.h"
#include "ChineseCharacter.h"
#include "control.h"
/*****************************************************************************
������*/
void main(void)
{
    InitSys();
    init_rtc();  //Ӳ��RTC��ʼ��
    Modbus_UART_Init();
    curveInit();
    alarmInit();
    while (1)
    {
        WDT_RST();               //ι��
        if (SysTick_RTC >= 500)  //ԭ����500�����Ը�Ϊ50
        {
            rdtime();  //����Ӳ��RTCʱ��
            // Uart2SendStr(test, strlen(test));
            SysTick_RTC = 0;
            RTC_Set_CMD();
        }
        HandleProc();
        Modbus_Process_Task();  // Modbus���ڴ�������
        ui();
        curveProcess();
        alarmTask();
        touchHandler();
        if (timer1msFlag)
        {
            timer1msFlag   = 0;
            timer100msFlag = 0;
        }
    }
}
