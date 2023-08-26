/*
 * p100.h
 *
 */

#ifndef P100_H_
#define P100_H_


#include <ti/drivers/Board.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/Timer.h>
#include <ti/drivers/SPI.h>


#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <ti/devices/msp432e4/driverlib/driverlib.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Swi.h>

#include "ti_drivers_config.h"

#include "sinlut.h"


#define MAXBUFFERSIZE       128
#define ERRORMSGSIZE        7
#define MSGCOUNT            8
#define MSGLENGTH           128
#define PAYLOADCOUNT        32
#define MAXCALLBACKS        3
#define TICKERCOUNT         16
#define REGCOUNT            32
#define SCRIPTCOUNT         32
#define SHADOWCOUNT         32
#define LUTSIZE             256
#define MSGLENGTH           128
#define CALLBACKCOUNT       3



typedef struct _tickers{
    int32_t index;
    int32_t delay;
    int32_t period;
    int32_t count;
    char payload[MSGLENGTH];
} Ticker;

typedef struct _callback
{
    int32_t index;
    int32_t count;
    char payload[MSGLENGTH];
} Callback;

typedef struct _Message
{
    int32_t msgCount;           //total letters in this message
    char msgBuf[MSGLENGTH];
} Message;

typedef struct _messageQueue
{
    int32_t msgReading;
    int32_t msgWriting;
    Message Messages[MSGCOUNT];
} MessageQ;

typedef struct _payload
{
    int32_t payloadCount;
    char payload[MSGLENGTH];
} Payload;

typedef struct _payloadQueue
{
    int32_t payloadReading;
    int32_t payloadWriting;
    Payload payloads[PAYLOADCOUNT];
} PayloadQ;

typedef struct _LUTControl{
     const uint16_t *sinlut14;
     float lutDelta;
     float lutPosition;
} LUTCtrl;

typedef struct _Network{
    int32_t             server;
    uint32_t            HostAddr;
    uint32_t            ClientAddr;
    uint32_t            ClientPort;
    uint16_t            UDPPort;
} Net;

typedef struct _BIOSList{

    Semaphore_Handle UARTSem;
    Semaphore_Handle UART0ReadSem;
    Semaphore_Handle PayloadLowP1WriteSem;
    Semaphore_Handle PayloadLowP1ReadSem;
    Semaphore_Handle PayloadLowP1Sem;
    Semaphore_Handle MsgReadSem;
    Semaphore_Handle CallbackSem;
    Semaphore_Handle TickerSem;
    Semaphore_Handle IfQWriteSem;
    Semaphore_Handle SerialSem;
    Semaphore_Handle CallbackGate;
    Semaphore_Handle UDPOutSem;
    Semaphore_Handle UDPOutWriteSem;
    Semaphore_Handle UDPOutReadSem;
    Semaphore_Handle UDPLaunchSem;
    Semaphore_Handle UDPInSem;

    Task_Handle task0;
    Task_Handle task1;
    Task_Handle task2;

    Swi_Handle Timer0SWI;
    //Swi_Handle Timer1SWI;
    //Swi_Handle GateSwi;
}BIOSList;

typedef struct _Globals{
    UART_Handle     uart;
    UART_Handle     uart7;

    Timer_Handle    timer1;
    Timer_Handle    timer2;
    SPI_Handle      spi;

    BIOSList        Bios;
    Net             Net;

    char            xmitbuff[MAXBUFFERSIZE];
    char            udpbuff[MAXBUFFERSIZE];
    char            print[MAXBUFFERSIZE];
    char            MsgBuff[MAXBUFFERSIZE];
    char            *ErrorMessages[ERRORMSGSIZE];
    char            Scripts[SCRIPTCOUNT][MSGLENGTH];

    int32_t         reg[REGCOUNT];
    int32_t         shadow[SHADOWCOUNT];

    int             ErrorCounts[ERRORMSGSIZE];
    int             gpioSel;
    int             Timer1Period;
    int             Timer2Period;

    Message         Msg;    //Msg being built
    MessageQ        MsgQ;
    MessageQ        NetInQ;
    MessageQ        IfQ;

    PayloadQ        PayQ;
    PayloadQ        NetOutQ;


    Callback        callbacks[MAXCALLBACKS];
    Ticker          tickers[TICKERCOUNT];

    LUTCtrl         LUTCtrl;

    //Network         Net;

}Globals;

Globals Glo;

void InitializeDrivers(void);
void UARTWrite(char *str);
void UARTWriteProtected(char str[]);
void UARTAddByte(char *input);
void getNextStr(char print[], char xmitbuf[]);
char *MPgetNextStr(char buf[], bool flag);
void clearBuffer(char buf[]);
uint16_t MatchString(char input[], char cmd[]);
void MPPrint(char *str);
void MPHelp();
void MPAbout();
void MPMemr();
void MPGpio();
void MPError();
void MPTimer();
void MPCallback();
void MPTickers(char *msgBuf);
void MPReg(char *msgBuf);
void MPScripts(char *msgBuf);
void MPIf(char *msgBuf);
void MPUart(char *msgBuf);
void MPSine(char *msgBuf);
void MPNetUDP(char *msgbuf);
void writeShadow(int32_t index, int32_t value);
void Initialize();
void TSKUART7Read();
void TSKPayloadLowP1();
void TSKMsgAddByte();
void MsgAddByte(char *input);
void MsgParse(char *payload);
int32_t AddPayload(char *payload);
void timer0Callback();
void timer1Callback();
int32_t AddCallback(int32_t index, int32_t count, char *payload);
int32_t AddTicker(int32_t index, int32_t delay, int32_t count, int32_t period, char *payload);
void gpioButtonSW1Callback(uint_least8_t index);
void gpioButtonSW2Callback(uint_least8_t index);
//int32_t MakeMessage(char *str);
void Initialize();
void TSKUART7Read();
void TSKPayloadLowP1();
void TSKMsgAddByte();
void MsgAddByte(char *input);
void MsgParse(char *payload);

#endif /* P100_H_ */
