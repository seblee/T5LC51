/******************************************************************************
          版权所有 (C), 2020，DFX，Write by Food(181 1266 9427)
    本程序是基于DWin的4寸480*480温控器C语言例子程序，去除了不需要的按键检测、RTC等
不需要的命令、函数。
   配置如下：
     1. UART2 作为log的输出，用于监控程序的状态
     2. UART4 作为ModBus的通讯串口，处理发送和接收命令，
     3. 其他为迪文的DGUS基本配置，
     功能如下：
     1. 实现Modbus RTU主站命令，03读取命令，06写寄存器命令，控制、显示modbus从站状态
     2. 实现UI的显示，控制，状态的更新，

     说明：程序可自由传播，但请仔细阅读后再应用，对引起的任何问题无任何的责任。使用
过程中，如果有疑问，可以加V(181 1266 9427)共同探讨。
******************************************************************************/
#include "modbus.h"
#include "sys.h"
#include "crc16.h"
#include "uart.h"
#include "ui.h"

void Modbus_RX_Reset(void);
// void Modbus_TX_Reset(void);
void Modbus_Write_Register06H(modbosCmd_t *CmdNow, u16 value);
void Modbus_Read_Register(modbosCmd_t *CmdNow);
void modbus_process_command(u8 *pstr, u16 strlen);

u8 modbus_rx_count = 0;                 //接收到的字符串的长度
u8 modbus_rx_flag  = 0;                 //接收到的字符串的标志，为1表示有收到数据
u8 modbus_rx_buf[UART_RX_BUF_MAX_LEN];  //接收到的字符串的内容

// extern process_struct process_flag;  //命令状态标志
extern u32 data SysTick;        //每隔1ms+1
u32 uart_rx_check_tick    = 0;  //检查串口是否接收结束
u8 modbus_rx_count_before = 0;  //接收串口的数据

u8 read_status_interval_times = 0;  //控制读取状态03H的间隔时间
u32 modbus_tx_process_tick    = 0;  // modbus发送命令的时间间隔

const modbosCmd_t modbusCmdlib[CMD_NUMBER] = {
    // en         id         fun           len  timeout mod    modP     VP  slaveAddr feedback
    {MODBUS_EN, SLAVE_ID, MODBUS_READ_03H, 0x04, 0xc8, 0x01, 0x000e, 0x5000, 0x0300, 0x00ff},   // 00
    {MODBUS_EN, SLAVE_ID, MODBUS_WRITE_06H, 0x01, 0xc8, 0x02, 0x8001, 0x8000, 0x0304, 0x00ff},  // 01
    {MODBUS_DIS, SLAVE_ID, MODBUS_READ_03H, 0x02, 0xc8, 0x01, 0x0000, 0x5000, 0x0300, 0x00ff},  // 02
    {MODBUS_DIS, SLAVE_ID, MODBUS_READ_03H, 0x02, 0xc8, 0x01, 0x0000, 0x5000, 0x0300, 0x00ff},  // 03
    {MODBUS_DIS, SLAVE_ID, MODBUS_READ_03H, 0x02, 0xc8, 0x01, 0x0000, 0x5000, 0x0300, 0x00ff},  // 04
    {MODBUS_DIS, SLAVE_ID, MODBUS_READ_03H, 0x02, 0xc8, 0x01, 0x0000, 0x5000, 0x0300, 0x00ff},  // 05
    {MODBUS_DIS, SLAVE_ID, MODBUS_READ_03H, 0x02, 0xc8, 0x01, 0x0000, 0x5000, 0x0300, 0x00ff},  // 06
    {MODBUS_DIS, SLAVE_ID, MODBUS_READ_03H, 0x02, 0xc8, 0x01, 0x0000, 0x5000, 0x0300, 0x00ff},  // 07
    {MODBUS_DIS, SLAVE_ID, MODBUS_READ_03H, 0x02, 0xc8, 0x01, 0x0000, 0x5000, 0x0300, 0x00ff},  // 08
    {MODBUS_DIS, SLAVE_ID, MODBUS_READ_03H, 0x02, 0xc8, 0x01, 0x0000, 0x5000, 0x0300, 0x00ff},  // 09
    {MODBUS_DIS, SLAVE_ID, MODBUS_READ_03H, 0x02, 0xc8, 0x01, 0x0000, 0x5000, 0x0300, 0x00ff},  // 10
};
modbosCmd_t modbusCmdNow = {0};
u8 CmdIndex              = 0;

_TKS_FLAGA_type modbusFlag = {0};
/******************************************************************************
          版权所有 (C), 2020，DFX，Write by Food(181 1266 9427)
 ******************************************************************************
modbus 命令解析处理程序，实现：
1. 03H的回送命令解析
2. 06H的回送命令解析，如果回送命令正确，则停止UI的触发发送命令
******************************************************************************/

void modbus_process_command(u8 *pstr, u16 strlen)
{
    u16 num;
    u16 crc_data;
    u16 len;

    // printf("Modbus string:");
    for (num = 0; num < strlen; num++)
    {
        // printf("%02X ", (u16)(*(pstr + num)));
    }
    // printf(",length:%d\r\n", strlen);

    if (strlen < 5)
    {
        return;
    }
    num = 0;
    do
    {
        if ((*(pstr + num)) == SLAVE_ID)
        {
            switch (*(pstr + num + 1))  //判读下一个字节是modbus的哪个命令
            {
                case MODBUS_READ_03H:
                    len = *(pstr + num + 2);
                    if ((len + num + 5) > strlen)  //长度超过最大长度
                    {
                        num = strlen;  //非modbus命令
                        break;
                    }
                    crc_data = crc16table(pstr + num, 3 + len);
                    // printf("num:%d,len:%d,crc data:%02X,%02X,", num, len, (u16)((crc_data >> 8) &
                    // 0xFF),(u16)(crc_data & 0xFF));
                    if ((*(pstr + num + len + 3) != ((crc_data >> 8) & 0xFF)) ||
                        (*(pstr + num + len + 4) != (crc_data & 0xFF)))  // CRC
                    {
                        break;
                    }
                    WriteDGUS(modbusCmdNow.VPAddr, (pstr + 3), *(pstr + 2));
                    memset(&modbusCmdNow, 0, sizeof(modbosCmd_t));
                    num       = len + 5;
                    cmdRxFlag = 1;
                    break;
                case MODBUS_WRITE_06H:
                    if ((num + 8) > strlen)
                    {
                        num = strlen;  //非modbus命令
                        break;
                    }
                    crc_data = crc16table(pstr + num, 6);
                    if ((*(pstr + num + 6) != ((crc_data >> 8) & 0xFF)) ||
                        (*(pstr + num + 7) != (crc_data & 0xFF)))  // CRC
                    {
                        break;
                    }
                    num += 8;
                    memset(&modbusCmdNow, 0, sizeof(modbosCmd_t));
                    cmdRxFlag = 1;
                    break;
                default:
                    break;
            }
        }
        num++;
    } while (num < (strlen - 5));  // addre,command,data,crch,crcl,至少需要有5个字节
}
/******************************************************************************
          版权所有 (C), 2020，DFX，Write by Food(181 1266 9427)
 ******************************************************************************
modbus 发送和接收任务处理程序，实现：
1. 监控串口接收，当判断接收结束后，调用处理函数，
2. 监控UI的触发命令，当有检测到发送命令时，发送modbus写命令
3. 每隔1秒钟触发一次查询modbus寄存器状态的命令
******************************************************************************/
void Modbus_Process_Task(void)
{
    modbosCmd_t *cmdTemp_t = NULL;
    if (modbus_rx_flag == 1)  //接收数据
    {
        if (modbus_rx_count > modbus_rx_count_before)
        {
            modbus_rx_count_before = modbus_rx_count;
            uart_rx_check_tick     = 0;
        }
        else if (modbus_rx_count == modbus_rx_count_before)
        {
            if (uart_rx_check_tick > 0)
            {
                if ((SysTick - uart_rx_check_tick) > RX_CHECK_TICK_TIME)
                {
                    modbus_process_command(modbus_rx_buf, modbus_rx_count);
                    Modbus_RX_Reset();
                }
            }
            else
            {
                uart_rx_check_tick = SysTick;
            }
        }
    }

    if (cmdTxFlag)
    {
        if ((cmdRxFlag) || ((SysTick - modbus_tx_process_tick) >= modbusCmdNow.timeout))
        {
            CmdIndex++;
            goto processCMDLib;
        }
        return;
    }

    if ((SysTick - modbus_tx_process_tick) < MODBUS_SEND_TIME_PERIOD)  //间隔固定时间后再处理UI的设置命令，
    {
        return;
    }
processCMDLib:
    modbus_tx_process_tick = SysTick;
    read_status_interval_times++;
    cmdRxFlag = 0;
    cmdTxFlag = 0;
    if ((read_status_interval_times % 2) == 0)  //一秒钟执行一次，
    {
        getCmd(&CmdIndex);
        if (CmdIndex < CMD_NUMBER)
        {
            memcpy(&modbusCmdNow, &modbusCmdlib[CmdIndex], sizeof(modbosCmd_t));
            if (modbusCmdNow.funCode == MODBUS_READ_03H)
            {
                Modbus_Read_Register(&modbusCmdNow);
                cmdTxFlag = 1;
            }
            else if (modbusCmdNow.funCode == MODBUS_WRITE_06H)
            {
                u16 value;
                ReadDGUS(modbusCmdNow.VPAddr, (u8 *)(&value), 2);
                Modbus_Write_Register06H(&modbusCmdNow, value);
                cmdTxFlag = 1;
            }
        }
        else
        {
            CmdIndex = 0;
        }
    }
}
// modbus 03H 读取寄存器
void Modbus_Read_Register(modbosCmd_t *CmdNow)
{
    u16 crc_data;
    u8 len;
    u8 modbus_tx_buf[20];

    len                  = 0;
    modbus_tx_buf[len++] = CmdNow->slaveID;
    modbus_tx_buf[len++] = MODBUS_READ_03H;                  // command
    modbus_tx_buf[len++] = (CmdNow->slaveAddr >> 8) & 0xFF;  // register
    modbus_tx_buf[len++] = CmdNow->slaveAddr & 0xFF;
    modbus_tx_buf[len++] = (CmdNow->length >> 8) & 0xFF;  // register number
    modbus_tx_buf[len++] = CmdNow->length & 0xFF;
    crc_data             = crc16table(modbus_tx_buf, len);
    modbus_tx_buf[len++] = (crc_data >> 8) & 0xFF;
    modbus_tx_buf[len++] = crc_data & 0xFF;
    Uart2SendStr(modbus_tx_buf, len);
}

// modbus 06H 发送
void Modbus_Write_Register06H(modbosCmd_t *CmdNow, u16 value)
{
    u16 crc_data;
    u8 len;
    u8 modbus_tx_buf[20];

    len                  = 0;
    modbus_tx_buf[len++] = CmdNow->slaveID;
    modbus_tx_buf[len++] = MODBUS_WRITE_06H;                 // command
    modbus_tx_buf[len++] = (CmdNow->slaveAddr >> 8) & 0xFF;  // register
    modbus_tx_buf[len++] = CmdNow->slaveAddr & 0xFF;
    modbus_tx_buf[len++] = (value >> 8) & 0xFF;  // register value
    modbus_tx_buf[len++] = value & 0xFF;
    crc_data             = crc16table(modbus_tx_buf, len);
    modbus_tx_buf[len++] = (crc_data >> 8) & 0xFF;
    modbus_tx_buf[len++] = crc_data & 0xFF;
    Uart2SendStr(modbus_tx_buf, len);
}  // modbus 06H 发送
void Modbus_Write_Register10H(modbosCmd_t *CmdNow, u16 value)
{
    u16 crc_data;
    u8 len;
    u8 modbus_tx_buf[20];

    len                  = 0;
    modbus_tx_buf[len++] = CmdNow->slaveID;
    modbus_tx_buf[len++] = MODBUS_WRITE_06H;                 // command
    modbus_tx_buf[len++] = (CmdNow->slaveAddr >> 8) & 0xFF;  // register
    modbus_tx_buf[len++] = CmdNow->slaveAddr & 0xFF;
    modbus_tx_buf[len++] = (value >> 8) & 0xFF;  // register value
    modbus_tx_buf[len++] = value & 0xFF;
    crc_data             = crc16table(modbus_tx_buf, len);
    modbus_tx_buf[len++] = (crc_data >> 8) & 0xFF;
    modbus_tx_buf[len++] = crc_data & 0xFF;
    Uart2SendStr(modbus_tx_buf, len);
}
//清除modbus RX的相关参数
void Modbus_RX_Reset(void)
{
    modbus_rx_count = 0;
    modbus_rx_flag  = 0;
    memset(modbus_rx_buf, '\0', UART_RX_BUF_MAX_LEN);
    modbus_rx_count_before = 0;
    uart_rx_check_tick     = 0;
}
//初始化modbus 相关参数
void Modbus_UART_Init(void)
{
    //	Modbus_TX_Reset();
    Modbus_RX_Reset();
    modbus_tx_process_tick = 0;  //初始化 0
    // process_flag.run_set   = 0;  //初始化0
    // process_flag.speed_set = 0;  //初始化0
    // process_flag.temp_set  = 0;  //初始化0
    // process_flag.hot_set   = 0;  //初始化0
}

void getCmd(u8 *index)
{
    u8 i;
    for (i = *index; i < CMD_NUMBER; i++)
    {
        if ((modbusCmdlib[i].modbusEn != MODBUS_EN) || (modbusCmdlib[i].length == 0))
        {
            continue;
        }
        if (modbusCmdlib[i].mode == 0)
        {
            goto getCmdExit;
        }
        else if (modbusCmdlib[i].mode == 1)
        {
            if (picNow == modbusCmdlib[i].moddPara)
            {
                goto getCmdExit;
            }
            continue;
        }
        else if (modbusCmdlib[i].mode == 2)
        {
            u16 paraTemp;
            ReadDGUS(modbusCmdlib[i].moddPara, (u8 *)(&paraTemp), 2);
            if ((paraTemp & 0xff) == 0x5a)
            {
                paraTemp = 0;
                WriteDGUS(modbusCmdlib[i].moddPara, (u8 *)(&paraTemp), 2);
                goto getCmdExit;
            }
            continue;
        }
    }
getCmdExit:
    *index = i;
}
