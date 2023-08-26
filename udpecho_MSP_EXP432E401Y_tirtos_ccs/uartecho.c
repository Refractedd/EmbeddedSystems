/*
 *  ======== uartecho.c ========
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/Timer.h>
#include <ti/drivers/SPI.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Task.h>

#include "ti_drivers_config.h"

#include "p100.h"



#define UIF_MAX_LINE    10
#define UIF_MAX_ARGS    2

#define BKSP    0x0
#define SPACE   0x20
#define ENTER   0x0D
#define DLT     0x7F
extern Globals Glo;
extern void TSKMsgAddByte(char input[]);
extern void MsgAddByte(char *input);
extern void MsgParse();
extern void TSKPayloadLowP1();
//Messages Msg;
/*
 *  ======== mainThread ========
 */
void mainThread(void *arg0)
{
    UART_Params uartParams;
    /* Create a UART with data processing off. */
    UART_Params_init(&uartParams);
    uartParams.readDataMode = UART_DATA_BINARY;
    uartParams.writeDataMode = UART_DATA_BINARY;
    uartParams.readReturnMode = UART_RETURN_FULL;
    uartParams.baudRate = 115200;
    //uartParams.readEcho = UART_ECHO_OFF;

    GPIO_enableInt(CONFIG_GPIO_6);
    GPIO_enableInt(CONFIG_GPIO_7);

    Glo.uart = UART_open(CONFIG_UART_0, &uartParams);
    if (Glo.uart == NULL)
        while (1);

    UART_Params_init(&uartParams);
    uartParams.writeDataMode = UART_DATA_BINARY;
    uartParams.readDataMode = UART_DATA_TEXT;
    uartParams.readReturnMode = UART_RETURN_NEWLINE;
    uartParams.baudRate = 115200;
    uartParams.readEcho = UART_ECHO_OFF;

    Glo.uart7 = UART_open(CONFIG_UART_1, &uartParams);
    if(Glo.uart7 == NULL)
        while(1);

    Timer_Params TimerParams;
    Timer_Params_init(&TimerParams);
    TimerParams.periodUnits = Timer_PERIOD_US;
    TimerParams.period = 1000000;
    Glo.Timer1Period = TimerParams.period;
    TimerParams.timerMode = Timer_CONTINUOUS_CALLBACK;
    TimerParams.timerCallback = timer0Callback;


    Glo.timer1 = Timer_open(CONFIG_TIMER_1, &TimerParams);

    if(Glo.timer1 == NULL){
        //UARTWrite("TIMER NULL\r\n");
        while(1);
    }
    if(Timer_start(Glo.timer1) == Timer_STATUS_ERROR){
        //UARTWrite("Timer Failed to start\r\n");
        while(1);
    }
    Timer_stop(Glo.timer1);
    Glo.Timer1Period = 1000000;

    TimerParams.periodUnits = Timer_PERIOD_US;
    TimerParams.period = 10000;
    Glo.Timer2Period = 10000;
    TimerParams.timerMode = Timer_CONTINUOUS_CALLBACK;
    TimerParams.timerCallback = timer1Callback;

    Glo.timer2 = Timer_open(CONFIG_TIMER_2, &TimerParams);
    if(Glo.timer2 == NULL){
        //UARTWrite("TIMER 2 NULL\r\n");
        while(1);
    }
    if(Timer_start(Glo.timer2) == Timer_STATUS_ERROR){
        //UARTWrite("Timer 2 Failed to start\r\n");
        while(1);
    }
    Timer_start(Glo.timer2);

    SPI_Params spiParams;
    SPI_Params_init(&spiParams);
    spiParams.dataSize = 16;
    spiParams.frameFormat = SPI_POL0_PHA1;
    Glo.spi = SPI_open(CONFIG_SPI_0, &spiParams);

    if(Glo.spi == NULL){
        //UARTWrite("SPI Failed To initialize");
        while(1);
    }
    char input[2] = "-0";
    char TITLE[] = "\r\nMSP432E401Y UART Interface\r\n";
    MPPrint(TITLE);
    MPAbout();

    memset(Glo.MsgBuff, 0, 128);
    //clearBuffer(input);
    input[1] = 0;

    while (1)
    {
        UART_read(Glo.uart, &input[0], 1);
        MsgAddByte(&input[0]);
        //clearBuffer(input);
    }
}
