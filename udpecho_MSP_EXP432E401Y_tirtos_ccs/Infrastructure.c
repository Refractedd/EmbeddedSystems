/*
 * Infrastructure.c
 *
 *  Created on: Sep 27, 2022
 *      Author: ninja
 */
#include "p100.h"

#define CALLBACKCOUNT       3
#define BKSP    0x0
#define SPACE   0x20
#define ENTER   0x0D
#define DLT     0x7F

//extern uint16_t sinlut13[LUTSIZE + 1];

extern Globals Glo;
extern Semaphore_Handle UARTSem;
extern Semaphore_Handle UART0ReadSem;
extern Semaphore_Handle PayloadLowP1Sem;
extern Semaphore_Handle PayloadLowP1WriteSem;
extern Semaphore_Handle PayloadLowP1ReadSem;
extern Semaphore_Handle IfQWriteSem;
extern Semaphore_Handle MsgReadSem;
extern Semaphore_Handle CallbackSem;
extern Semaphore_Handle TickerSem;
extern Semaphore_Handle SerialSem;
extern Semaphore_Handle CallbackGate;
extern Semaphore_Handle UDPOutSem;
extern Semaphore_Handle UDPLaunchSem;
extern Semaphore_Handle UDPOutWriteSem;
extern Semaphore_Handle UDPOutReadSem;
extern Semaphore_Handle UDPInSem;
extern Task_Handle task0;
extern Task_Handle task1;
extern Task_Handle task2;
extern Swi_Handle swi0;
extern Swi_Handle swi1;
//extern SPI_Handle spi;
//extern Swi_Handle GateSwi;

extern AddPayload(char *payload);
extern AddTicker(int32_t index, int32_t delay, int32_t count, int32_t period, char *payload);


void Initialize(){

    char Error0[] = "Unrecognized Command - Message not recognized.";
    char Error1[] = "Message Overflow - Too many characters for buffer.";
    char Error2[] = "Memory Read Error - Attempt to read invalid address.";
    char Error3[] = "Timer Configuration Failed.";
    char Error4[] = "Reg Operation failed.";
    char Error5[] = "If Conditional Operation failed.";
    char Error6[] = "Script Index Out of Range";

    Glo.ErrorMessages[0] = Error0; // unknown
    Glo.ErrorMessages[1] = Error1; // Overflow
    Glo.ErrorMessages[2] = Error2; // MEMR
    Glo.ErrorMessages[3] = Error3; // Timer Config
    Glo.ErrorMessages[4] = Error4; // Reg Fail
    Glo.ErrorMessages[5] = Error5; // If Fail
    Glo.ErrorMessages[6] = Error6;

    Glo.ErrorCounts[0] = 0;
    Glo.ErrorCounts[1] = 0;
    Glo.ErrorCounts[2] = 0;
    Glo.ErrorCounts[3] = 0;
    Glo.ErrorCounts[4] = 0;
    Glo.ErrorCounts[5] = 0;
    Glo.ErrorCounts[5] = 0;
    Glo.Bios.UARTSem = UARTSem;
    Glo.Bios.UART0ReadSem = UART0ReadSem;
    Glo.Bios.PayloadLowP1Sem = PayloadLowP1Sem;
    Glo.Bios.PayloadLowP1WriteSem = PayloadLowP1WriteSem;
    Glo.Bios.PayloadLowP1WriteSem = PayloadLowP1ReadSem;
    Glo.Bios.UDPOutSem = UDPOutSem;
    Glo.Bios.UDPOutReadSem = UDPOutReadSem;
    Glo.Bios.UDPOutWriteSem = UDPOutWriteSem;
    Glo.Bios.UDPLaunchSem = UDPLaunchSem;
    Glo.Bios.UDPInSem = UDPInSem;

    Glo.Bios.MsgReadSem = MsgReadSem;
    Glo.Bios.CallbackSem = CallbackSem;
    Glo.Bios.TickerSem = TickerSem;
    Glo.Bios.IfQWriteSem = IfQWriteSem;
    Glo.Bios.SerialSem = SerialSem;
    Glo.Bios.task0 = task0;
    Glo.Bios.task1 = task1;
    Glo.Bios.task2 = task2;
    //Glo.spi = spi;
    Glo.Bios.Timer0SWI = swi0;
    //Glo.Bios.Timer1SWI = swi1;
    //Glo.Bios.GateSwi = GateSwi;
    //char buf[MAXBUFFERSIZE];

    //Glo.LUTCtrl.sinlut14 = sinlut12;
    Glo.LUTCtrl.lutPosition = 0;

    int i;
    for(i = 0; i < CALLBACKCOUNT; i++){
        Glo.callbacks[i].index = -1;
        Glo.callbacks[i].count = 100;
        strcpy(Glo.callbacks[i].payload, "");
    }
    for(i = 0; i < TICKERCOUNT; i++){
        Glo.tickers[i].index = i;
        Glo.tickers[i].count = 0;
        Glo.tickers[i].period = 0;
        Glo.tickers[i].delay = 0;
        Glo.tickers[i].payload[0] = 0;
    }

    for(i = 0; i < MSGCOUNT; i++){
        Glo.MsgQ.msgReading = 0;
        Glo.MsgQ.msgWriting = 0;
        Glo.MsgQ.Messages[i].msgBuf[0] = 0;
        Glo.MsgQ.Messages[i].msgCount = 0;
        Glo.PayQ.payloadReading = 0;
        Glo.PayQ.payloadWriting = 0;
        Glo.PayQ.payloads[i].payload[0] = 0;
        Glo.PayQ.payloads[i].payloadCount = 0;
    }
    for(i = 0; i < REGCOUNT; i++)
    {
        Glo.reg[i] = i * 10;
    }
    for(i = 0; i < SCRIPTCOUNT; i++){
        strcpy(Glo.Scripts[i], "");
        Glo.Scripts[i][0] = 0;
    }
    strcpy(Glo.Scripts[1], "-ticker 0 100 700 1 -gpio 0 t");
    strcpy(Glo.Scripts[2], "-ticker 1 200 500 1 -gpio 1 t");
    strcpy(Glo.Scripts[3], "-ticker 2 300 300 1 -gpio 2 t");
    strcpy(Glo.Scripts[4], "-ticker 3 400 100 1 -gpio 3 t");

    strcpy(Glo.Scripts[6], "-timer 125");
    strcpy(Glo.Scripts[7], "-gpio 4 w 0");
    strcpy(Glo.Scripts[8], "-gpio 5 w 1");
    strcpy(Glo.Scripts[9], "-sine 200");
    strcpy(Glo.Scripts[10], "-callback 0 -1 -sine");

    strcpy(Glo.Scripts[12], "-script 6");
    strcpy(Glo.Scripts[13], "-ticker 4 100 500 -1 -sine 200");
    strcpy(Glo.Scripts[14], "-ticker 5 200 500 -1 -sine 400");
    strcpy(Glo.Scripts[15], "-ticker 6 300 500 -1 -sine 600");
    strcpy(Glo.Scripts[16], "-ticker 7 1000 0 1 -timer stop");
    strcpy(Glo.Scripts[17], "-timer start");

}

void TSKMsgAddByte(){
    int32_t msgnext;
    for(;;){
        Semaphore_pend(Glo.Bios.UART0ReadSem, BIOS_WAIT_FOREVER);
        AddPayload(Glo.MsgQ.Messages[Glo.MsgQ.msgReading].msgBuf);
        Semaphore_pend(Glo.Bios.MsgReadSem, BIOS_WAIT_FOREVER);

        msgnext = Glo.MsgQ.msgReading + 1;
        if(msgnext >= MSGCOUNT)
            msgnext = 0;
        Glo.MsgQ.msgReading = msgnext;
        Semaphore_post(Glo.Bios.MsgReadSem);
    }
}

void TSKUart0Read(){
    char        input[2];
    char  echoPrompt[] = "\r\nMSP432E401Y UART Interface\r\n";

    input[1] = 0;

    UARTWrite(echoPrompt);

    for(;;)
    {
        UART_read(Glo.uart, &input[0], 1);
        UARTAddByte(input);
        MsgAddByte(&input[0]);
        if(Glo.MsgQ.msgReading != Glo.MsgQ.msgWriting)
            Semaphore_post(Glo.Bios.UART0ReadSem);
    }
}

void TSKUART7Read(){
    char rxbuf[MSGLENGTH];
    int_fast32_t len;
    clearBuffer(rxbuf);
    for(;;){
        len = UART_read(Glo.uart7, &rxbuf, sizeof(rxbuf));
        rxbuf[MSGLENGTH - 1] = 0;
        if(len > 1 && len < MSGLENGTH){
            rxbuf[len-1] = 0;
            UARTWriteProtected("\r\n");
            UARTWriteProtected("UART7> ");
            UARTWriteProtected(rxbuf);
            AddPayload(rxbuf);
        }
    }
}

void TSKPayloadLowP1(){
    int32_t payloadnext;
    int32_t index;
    char *payload;

    for(;;)
    {
        Semaphore_pend(Glo.Bios.PayloadLowP1Sem, BIOS_WAIT_FOREVER);

        index = Glo.PayQ.payloadReading;
        payload = Glo.PayQ.payloads[index].payload;

        MsgParse(payload);

        Glo.PayQ.payloads[index].payloadCount = 0;
        clearBuffer(Glo.PayQ.payloads[index].payload);

        payloadnext = Glo.PayQ.payloadReading + 1;
        if(payloadnext >= PAYLOADCOUNT)
            payloadnext = 0;
        Glo.PayQ.payloadReading = payloadnext;
    }

}

void MsgParse(char *payload){

    int32_t index = Glo.PayQ.payloadReading;
    char processBuff[MSGLENGTH];
    if(MatchString(Glo.PayQ.payloads[index].payload, payload) == 0)
        strcpy(processBuff, payload);
    else if(MatchString(Glo.Msg.msgBuf, payload) == 0)
        strcpy(processBuff, Glo.Msg.msgBuf);
    else
        return;

    if(processBuff[0] != 0)
    {
        if(MatchString(processBuff,         "-about")   == 0)
            MPAbout();
        else if(MatchString(processBuff,    "-help")    == 0)
            MPHelp(processBuff);
        else if(MatchString(processBuff,    "-print")   == 0)
            MPPrint(processBuff);
        else if(MatchString(processBuff,    "-memr")    == 0)
            MPMemr(processBuff);
        else if(MatchString(processBuff,    "-gpio")    == 0)
            MPGpio(processBuff);
        else if(MatchString(processBuff,    "-error")   == 0)
            MPError(processBuff);
        else if(MatchString(processBuff,    "-timer")   == 0)
            MPTimer(processBuff);
        else if(MatchString(processBuff,    "-callback")== 0)
            MPCallback(processBuff);
        else if(MatchString(processBuff,    "-ticker")  == 0)
            MPTickers(processBuff);
        else if(MatchString(processBuff,    "-reg")     == 0)
            MPReg(processBuff);
        else if(MatchString(processBuff,    "-script")  == 0)
            MPScripts(processBuff);
        else if(MatchString(processBuff,    "-if")      == 0)
            MPIf(processBuff);
        else if(MatchString(processBuff,    "-uart")    == 0)
            MPUart(processBuff);
        else if(MatchString(processBuff,    "-sine")    == 0)
            MPSine(processBuff);
        else if(MatchString(processBuff,    "-netudp")   == 0)
            MPNetUDP(processBuff);
        else
        {
            Glo.ErrorCounts[0]++;
            UARTWrite(Glo.ErrorMessages[0]);
        }
    }
    clearBuffer(processBuff);
}


void MsgAddByte(char *input){
    input[1] = 0;
    char RETURN[] = "\r\n";
    if(input[0] == '\r' || input[0] == '\n')
    {
        MsgParse(Glo.Msg.msgBuf);
        UARTWriteProtected(RETURN);
        clearBuffer(Glo.Msg.msgBuf);
        Glo.Msg.msgCount = 0;
    }
    else if(input[0] == '\b' || input[0] == 0x7f)
    {
        if(Glo.Msg.msgCount > 0)
        {
            Glo.Msg.msgBuf[Glo.Msg.msgCount--] = 0;
            UARTAddByte(&input[0]);
        }
    }
    else
    {
        if(Glo.Msg.msgCount == MSGLENGTH-1)
        {
            Glo.ErrorCounts[0]++;
            UARTWrite(Glo.ErrorMessages[0]);
            Glo.Msg.msgCount = 0;
            Glo.Msg.msgBuf[Glo.Msg.msgCount] = 0;
        }
        else
        {
            Glo.Msg.msgBuf[Glo.Msg.msgCount++] = input[0];
            if(Glo.Msg.msgCount >= MSGLENGTH-1)
                Glo.Msg.msgBuf[Glo.Msg.msgCount] = 0;
            UARTAddByte(&input[0]);
        }
    }
    return;
}




