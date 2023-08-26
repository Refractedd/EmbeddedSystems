/*
 * p100.c
 *
 *  Created on: Sep 9, 2022
 *  Author: Benjamin Williams
 *
 */

#include "p100.h"


void InitializeDrivers(){
    Board_init();
    GPIO_init();
    UART_init();
    Timer_init();
    SPI_init();
}

void MPNetUDP(char *msgbuf){
    char *loc;
    int32_t payloadnext;

    loc= MPgetNextStr(msgbuf, true);
    if(!loc)
        return;

    Semaphore_pend(Glo.Bios.UDPOutWriteSem, BIOS_WAIT_FOREVER);
    strcpy(Glo.NetOutQ.payloads[Glo.NetOutQ.payloadWriting].payload, loc);
    payloadnext = Glo.NetOutQ.payloadWriting + 1;
    if(payloadnext >= PAYLOADCOUNT)
        payloadnext = 0;
    Glo.NetOutQ.payloadWriting = payloadnext;
    Semaphore_post(Glo.Bios.UDPOutWriteSem);
    Semaphore_post(Glo.Bios.UDPOutSem);
}

void MPSine(char *msgBuf){
    uint16_t sinlut14[LUTSIZE + 1] = {
      8192,8393,8593,8794,8994,9194,9393,9592,9790,9986,10182,10376,10569,10761,10951,11140,11326,11511,11694,11874,12053,12229,12403,12574,12742,12908,13071,13231,13388,13542,13693,13840,13984,14124,14261,14394,14524,14649,14771,14889,15002,15112,15218,15319,15416,15508,15597,15680,15759,15834,15904,15970,16030,16086,16138,16184,16226,16262,16294,16321,16344,16361,16373,16381,16383,16381,16373,16361,16344,16321,16294,16262,16226,16184,16138,16086,16030,15970,15904,15834,15759,15680,15597,15508,15416,15319,15218,15112,15002,14889,14771,14649,14524,14394,14261,14124,13984,13840,13693,13542,13388,13231,13071,12908,12742,12574,12403,12229,12053,11874,11694,11511,11326,11140,10951,10761,10569,10376,10182,9986,9790,9592,9393,9194,8994,8794,8593,8393,8192,7990,7790,7589,7389,7189,6990,6791,6593,6397,6201,6007,5814,5622,5432,5243,5057,4872,4689,4509,4330,4154,3980,3809,3641,3475,3312,3152,2995,2841,2690,2543,2399,2259,2122,1989,1859,1734,1612,1494,1381,1271,1165,1064,967,875,786,703,624,549,479,413,353,297,245,199,157,121,89,62,39,22,10,2,0,2,10,22,39,62,89,121,157,199,245,297,353,413,479,549,624,703,786,875,967,1064,1165,1271,1381,1494,1612,1734,1859,1989,2122,2259,2399,2543,2690,2841,2995,3152,3312,3475,3641,3809,3980,4154,4330,4509,4689,4872,5057,5243,5432,5622,5814,6007,6201,6397,6593,6791,6990,7189,7389,7589,7790,7990,8192,
     };


    char *loc;
    uint32_t freq;
    float lowerWeight, upperWeight;
    uint32_t lowerIndex, upperIndex;
    float answer;
    uint16_t outval;

    SPI_Transaction spiTransaction;
    bool            transferOK;

    loc = MPgetNextStr(msgBuf, true);

    if(Glo.Timer1Period == 0){
        AddPayload("-print Timer 0 is off");
        return;
    }
    if(loc && loc[0] != 0 && Glo.Timer1Period > 0)
    {
        freq = atoi(loc);
        Glo.LUTCtrl.lutDelta = (float) freq * (float) LUTSIZE * (float) Glo.Timer1Period / 1000000.;
    }
    if(Glo.LUTCtrl.lutDelta >= LUTSIZE / 2)
    {
        AddPayload("-print Mr.Nyquist Frowns upon you...");
        AddPayload("-callback 0");
        //MPTimer("-timer 0");
        return;
    }
    if(Glo.LUTCtrl.lutDelta <= 0)
    {
        if(Glo.Timer1Period > 0)
        {
            MPTimer("-timer 0");
            MPCallback("-callback 0");
        }
        AddPayload("-print Timer 0 Disabled");
        return;
    }
    if(loc)
        return;

    Semaphore_pend(Glo.Bios.SerialSem, BIOS_WAIT_FOREVER);

    lowerIndex = (uint32_t) Glo.LUTCtrl.lutPosition;
    upperIndex = lowerIndex + 1;
    upperWeight = Glo.LUTCtrl.lutPosition - (float) lowerIndex;
    lowerWeight = 1. - upperWeight;

    answer = (float) sinlut14[lowerIndex] * lowerWeight + (float) sinlut14[upperIndex] * upperWeight;
    outval = round(answer);
    //sprintf(Glo.xmitbuff, "-print MPSine Out Value: %d", outval);
    //AddPayload(Glo.xmitbuff);
    //clearBuffer(Glo.xmitbuff);

    spiTransaction.count = 1;
    spiTransaction.txBuf = (void *) &outval;
    spiTransaction.rxBuf = (void *) NULL;

    transferOK = SPI_transfer(Glo.spi, &spiTransaction);
    if(!transferOK){
        while(1);
    }
    Glo.LUTCtrl.lutPosition += Glo.LUTCtrl.lutDelta;
    while(Glo.LUTCtrl.lutPosition >= (float) LUTSIZE)
        Glo.LUTCtrl.lutPosition -= (float) LUTSIZE;

    Semaphore_post(Glo.Bios.SerialSem);
}

void MPUart(char *msgBuf){
    char *loc;
    char xmitbuf[MSGLENGTH];
    clearBuffer(xmitbuf);
    loc = MPgetNextStr(msgBuf, true);
    if(loc){
        strcpy(xmitbuf, loc);
        strcat(xmitbuf, "\r\n");
        //strcat(xmitbuf, '\n');
        UART_write(Glo.uart7, &xmitbuf, strlen(xmitbuf));
    }
}

void MPIf(char *msgBuf){
    char *locScript, *locColon, *locFalse;
    int32_t SA, SB, val1, val2, ftn;
    char payloadT[MSGLENGTH], *payloadF, *answer;
    int32_t msgNext;

    locScript = MPgetNextStr(msgBuf, true);
    if(!locScript)
        goto IFERROR;
    if(*locScript == '#')
    {
        locScript++;
        val1 = atoi(locScript);
    }
    else
    {
        if(locScript)
            SA = atoi(locScript);
        if(SA < 0 || SA >= REGCOUNT)
            goto IFERROR;
        val1 = Glo.reg[SA];
    }

    locScript = MPgetNextStr(locScript, true);
    if(!locScript)
        goto IFERROR;
    ftn = *locScript;

    locScript = MPgetNextStr(locScript, true);
    if(*locScript == '#')
    {
        locScript++;
        val2 = atoi(locScript);
    }
    else
    {
        if(locScript)
            SB = atoi(locScript);
        if(SB < 0 || SB >= REGCOUNT)
            goto IFERROR;
        val2 = Glo.reg[SB];
    }

    locScript = MPgetNextStr(locScript, true);
    if(!locScript)
       goto IFERROR;
    if(*locScript != '?')
       goto IFERROR;

    locScript = MPgetNextStr(locScript, true);
    if(!locScript)
        goto IFERROR;
    strcpy(payloadT, locScript);

    locColon = strchr(payloadT, ':');
    if(!locColon)
        payloadF = NULL;
    else
    {
        locFalse = MPgetNextStr(locColon, true);
        *locColon = 0;
        if(!locFalse)
            payloadF = NULL;
        else
            payloadF = locFalse;
    }

    if(strlen(payloadT) < 3)
        payloadT[0] = 0;
    switch(ftn)
    {
    case '>':
        if(val1 > val2)
            answer = payloadT;
        else
            answer = payloadF;
        break;
    case '=':
        if(val1 == val2)
            answer = payloadT;
        else
            answer = payloadF;
        break;
    case '<':
        if(val1 < val2)
            answer = payloadT;
        else
            answer = payloadF;
        break;
    default:
        goto IFERROR;
    }

    if(answer != NULL)
    {
        Semaphore_pend(Glo.Bios.IfQWriteSem, BIOS_WAIT_FOREVER);
        strcpy(Glo.IfQ.Messages[Glo.IfQ.msgWriting].msgBuf, answer);
        AddPayload(Glo.IfQ.Messages[Glo.IfQ.msgWriting].msgBuf);

        msgNext = Glo.IfQ.msgWriting + 1;
        if(msgNext >= MSGCOUNT)
            msgNext = 0;
        Glo.IfQ.msgWriting = msgNext;
        Semaphore_post(Glo.Bios.IfQWriteSem);
    }
    else
    {
IFERROR:
        UARTWrite(Glo.ErrorMessages[5]);
        Glo.ErrorCounts[5]++;
        return;
    }
}

void MPScripts(char *msgBuf){
    char *loc;
    int32_t i, index = -1;
    char xmitbuf[MSGLENGTH];

    loc = MPgetNextStr(msgBuf, true);
    if(loc)
        index = atoi(loc);

    if(index == -1){
        for(i = 0; i < SCRIPTCOUNT; i++){
            sprintf(xmitbuf, "\r\nSCRIPT %2d %s", i, Glo.Scripts[i]);
            UARTWriteProtected(xmitbuf);
        }
        UARTWriteProtected("\r\n");
        return;
    } else {
        if(index < 0 || index >= SCRIPTCOUNT){
            UARTWrite(Glo.ErrorMessages[6]);
            Glo.ErrorCounts[6]++;
        }
    }

    loc = MPgetNextStr(loc, true);
    if(!loc){
        while(Glo.Scripts[index][0] != 0 && index < SCRIPTCOUNT){
            if(MatchString("-if", Glo.Scripts[index]) == 0)
            {
                MPIf(Glo.Scripts[index]);
                return;
            }
            else
            {
                AddPayload(Glo.Scripts[index]);
                index++;
            }

        }
    }
    else
    {
        strcpy(Glo.Scripts[index], loc);
    }
}

void MPReg(char *msgBuf){

    int32_t i;
    int dest = -1;
    int32_t val1 = NULL, val2 = NULL;
    char *op;
    char *loc;
    op = NULL;
    loc = MPgetNextStr(msgBuf, true); // get dest
    if(!loc)
    {
        for(i = 0; i < REGCOUNT; i++){
            sprintf(Glo.xmitbuff, "\r\nREG %d  :  %d", i, Glo.reg[i]);
            UARTWriteProtected(Glo.xmitbuff);
        }
        UARTWriteProtected("\r\n");
        return;
    }else{
        dest = atoi(loc);
    }

    loc = MPgetNextStr(loc, true);
    if(!loc){
        sprintf(Glo.xmitbuff, "REG %d  :  %d", dest, Glo.reg[dest]);
        UARTWrite(Glo.xmitbuff);
        //UARTWrite("");
        return;
    }

    op = loc;

    loc = MPgetNextStr(loc, true);
    if(loc){
        if(*loc == '#'){
            loc++;
            val1 = (int32_t)atoi(loc);
        }else{
            if(atoi(loc) > -1 && atoi(loc) < REGCOUNT)
                val1 = (int32_t)Glo.reg[atoi(loc)];
            else{
                UARTWrite(Glo.ErrorMessages[4]);
                Glo.ErrorCounts[4]++;
                return;
            }
        }
    }
    loc = MPgetNextStr(loc, true);
    if(loc){
        if(*loc == '#'){
            loc++;
            val2 = (int32_t)atoi(loc);
        }else{
            if(atoi(loc) > -1 && atoi(loc) < REGCOUNT)
                val2 = (int32_t)Glo.reg[atoi(loc)];
            else{
REGERROR:       UARTWrite(Glo.ErrorMessages[4]);
                Glo.ErrorCounts[4]++;
                return;
            }
        }
    }

    if(dest >= 0 && dest < REGCOUNT)
    {
        switch(*op)
        {
        case 'r':
        case '?':       // read
            goto SHORT4E;
            //sprintf(Glo.xmitbuff, "REG %d : %d", dest, Glo.reg[dest]);
            //UARTWrite(Glo.xmitbuff);
            //break;
        case 'c':
        case 'w':
        case '=':       //write
            Glo.reg[dest] = val1;
            break;
        case '+':
            op++;
            if(*op == '+') Glo.reg[dest]++;
            else if(val1 && val2) Glo.reg[dest] = val1 + val2;
            else goto REGERROR;
            break;
        case '-':
            op++;
            if(*op == '-') Glo.reg[dest]--;
            else if(val1 && val2) Glo.reg[dest] = val1 - val2;
            else goto REGERROR;
            break;
        case '!':
            if(val1) Glo.reg[dest] = -Glo.reg[dest];
            else goto REGERROR;
            break;
        case '~':
            if(val1) Glo.reg[dest] = ~Glo.reg[dest];
            else goto REGERROR;
            break;
        case '&':
            if(val1 && val2) Glo.reg[dest] = val1 & val2;
            else goto REGERROR;
            break;
        case '|':
            if(val1 && val2) Glo.reg[dest] = val1 | val2;
            else goto REGERROR;
            break;
        case '^':
            if(val1 && val2) Glo.reg[dest] = val1 ^ val2;
            else goto REGERROR;
            break;
        case '*':
            if(val1 && val2) Glo.reg[dest] = val1 * val2;
            else goto REGERROR;
            break;
        case '/':
            //if(val1 && val2){
                Glo.reg[dest] = val1 / val2;
                if(val2 == 0){
                    if(val1 >= 0) Glo.reg[dest] =  (int32_t)(0x7FFFFFFF);
                    else          Glo.reg[dest] = -(int32_t)(0x7FFFFFFF);
                }
            //} else goto REGERROR;
            break;
        case '%':
            if(val1 && val2) Glo.reg[dest] = val1 % val2;
            else goto REGERROR;
            break;
        case '>':
            if(val1 && val2){
                if(val1 > val2) Glo.reg[dest] = val1;
                else Glo.reg[dest] = val2;
            } else goto REGERROR;
            break;
        case '<':
            if(val1 && val2){
                if(val1 < val2) Glo.reg[dest] = val1;
                else Glo.reg[dest] = val2;
            } else goto REGERROR;
            break;
        case 'x':
            if(val1 && val2){
                i = Glo.reg[dest];
                Glo.reg[dest] = Glo.reg[val1];
                Glo.reg[val1] = i;
            } else goto REGERROR;
            break;
        default:
            goto REGERROR;
        } //AND IOR XOR MUL DIV REM MAX MIN COPY XCG
    }

SHORT4E:
    sprintf(Glo.xmitbuff, "REG %d : %d", dest, Glo.reg[dest]);
    UARTWrite(Glo.xmitbuff);
    //UARTWrite("");
    return;
}

void MPTickers(char *msgBuf){
    char xmitbuf[MSGLENGTH];
    char payload[MSGLENGTH];
    int32_t index = -1, delay=0, period=0, count=0;
    char *loc;
    int32_t i;

    loc = MPgetNextStr(msgBuf, true);
    if(!loc){
        for(i = 0; i < TICKERCOUNT; i++){
            sprintf(xmitbuf, "\r\nTICKER %2d D:%d P:%d C:%d %s", i, Glo.tickers[i].delay, Glo.tickers[i].period, Glo.tickers[i].count, Glo.tickers[i].payload);
            UARTWriteProtected(xmitbuf);
        }
        UARTWriteProtected("\r\n");
        return;
    }
    if(atoi(loc) < 0 || atoi(loc) >= TICKERCOUNT){
        for(i = 0; i < TICKERCOUNT; i++){
            if(Glo.tickers[i].payload[0] == 0){
                index = i;
                break;
            }
        }
    }else
        index = atoi(loc);

    if(index < 0){
        for(i = 0; i < TICKERCOUNT; i++){
            index = AddTicker(i, 0, 0, 0, "");
            sprintf(xmitbuf, "\r\nTICKER %2d D:%d P:%d C:%d %s", i, Glo.tickers[i].delay, Glo.tickers[i].period, Glo.tickers[i].count, Glo.tickers[i].payload);
            UARTWriteProtected(xmitbuf);
        }
        UARTWriteProtected("\r\n");
        return;
    }

    if(index > TICKERCOUNT)
        index = TICKERCOUNT;

    loc = MPgetNextStr(loc, true);
    if(!loc){
        if(index >= TICKERCOUNT)
            index = TICKERCOUNT - 1;
        sprintf(xmitbuf, "TICKER %2d D:%d P:%d C:%d %s", index, Glo.tickers[index].delay, Glo.tickers[index].period, Glo.tickers[index].count, Glo.tickers[index].payload);
        UARTWrite(xmitbuf);
        return;
    }

    delay = atoi(loc);
    strcpy(payload, "");

    loc = MPgetNextStr(loc, true);
    if(!loc)
        goto SHORT3E;

    period = atoi(loc);

    loc = MPgetNextStr(loc, true);
    if(!loc)
        goto SHORT3E;

    count = atoi(loc);
    loc = MPgetNextStr(loc, true);
    if(loc)
        strcpy(payload, loc);

SHORT3E:
    index = AddTicker(index, delay, period, count, payload);

    sprintf(xmitbuf, "TICKER %2d D:%d P:%d C:%d %s\r\n", index, Glo.tickers[index].delay, Glo.tickers[index].period, Glo.tickers[index].count, Glo.tickers[index].payload);
    UARTWriteProtected(xmitbuf);

    return;
}

void MPCallback(char *str){
    char *loc, *payload;
    int32_t i, count = 0, index = 0;
    strcpy(Glo.MsgBuff, str);

    loc = MPgetNextStr(Glo.MsgBuff, true);
    if(!loc)    index = -1;
    else        index = atoi(loc);

    if(index < 0 || index >= CALLBACKCOUNT)
    {
        for(i = 0; i < CALLBACKCOUNT; i++){
            sprintf(Glo.xmitbuff, "\r\nCALLBACK %2d C: %d %s", i, Glo.callbacks[i].count, Glo.callbacks[i].payload);
            UARTWriteProtected(Glo.xmitbuff);
        }
        UARTWriteProtected("\r\n");
        return;
    }
    loc = MPgetNextStr(loc, true);
    if(*loc == NULL){
        AddCallback(index, 0, "");
        sprintf(Glo.xmitbuff, "CALLBACK %2d C: %d %s", index, Glo.callbacks[index].count, Glo.callbacks[index].payload);
        UARTWrite(Glo.xmitbuff);
        //UARTWrite("");
        return;
    }
    //loc = MPgetNextStr(loc, true);
    count = atoi(loc);

    loc = MPgetNextStr(loc, true);
    if(loc)     payload = loc;
    else        payload = "";

    AddCallback(index, count, payload);
    sprintf(Glo.xmitbuff, "CALLBACK %2d C: %d %s", index, Glo.callbacks[index].count, Glo.callbacks[index].payload);
    UARTWrite(Glo.xmitbuff);
    clearBuffer(Glo.MsgBuff);
    return;
}

void MPTimer(char *str){
    char start[] = "start";
    char stop[] = "stop";
    //int32_t index = Glo.PayQ.payloadReading;
    strcpy(Glo.MsgBuff, str);
    char *loc = MPgetNextStr(Glo.MsgBuff, true);
    if(strstr(Glo.MsgBuff, start) != 0)
        Timer_start(Glo.timer1);
    else if(strstr(Glo.MsgBuff, stop) != 0)
         Timer_stop(Glo.timer1);
    else if(loc)
    {
        int32_t T = 0;
        T = atoi(loc);
        //if (T < 10000) T = 10000;
        if(Timer_setPeriod(Glo.timer1, Timer_PERIOD_US, T) != Timer_STATUS_ERROR){
            Glo.Timer1Period = T;
            sprintf(Glo.xmitbuff, "Timer Period Set : %d", T);
            UARTWrite(Glo.xmitbuff);
        }else{
            UARTWrite("Timer Set Period Failed");
            UARTWrite(Glo.ErrorMessages[3]);
            Glo.ErrorCounts[3]++;
        }
    } else {
        sprintf(Glo.xmitbuff, "Timer1 : Period : %d", Glo.Timer1Period);
        UARTWrite(Glo.xmitbuff);
        sprintf(Glo.xmitbuff, "Timer2 : Period : %d", Glo.Timer2Period);
        UARTWrite(Glo.xmitbuff);
    }
    //UARTWrite("");
    return;
}

void MPAbout(){
    char ABOUTMSG[] =
            "\nBenjamin Williams\r\n"
            "Assignment 11\r\n"
            "Version 11.999\r\n"
            __DATE__"\r\n"__TIME__
            "";

    sprintf(Glo.xmitbuff, "%s\r\nIP: %d.%d.%d.%d:%d", ABOUTMSG,
            (uint8_t)(Glo.Net.HostAddr>>24)&0xFF, (uint8_t)(Glo.Net.HostAddr>>16)&0xFF,
            (uint8_t)(Glo.Net.HostAddr>>8)&0xFF, (uint8_t)Glo.Net.HostAddr&0xFF, Glo.Net.UDPPort);
    UARTWrite(Glo.xmitbuff);
    clearBuffer(Glo.xmitbuff);
    //UARTWrite("");
    return;
}

void MPHelp(char *str){

    char HELPMSG1[] =
            "\r\nCommands:\r\n"
            "1.  -about\r\n"
            "2.  -help \t[commandName]\r\n"
            "3.  -print \t[message]\r\n"
            "4.  -memr  \t[address]\r\n"
            "5.  -gpio  \t[0-7]\t[r|w|t]\r\n"
            "6.  -error \t[1-2]\r\n"
            "7.  -timer \t[timerNum]\t[start/stop]\r\n";
    char HELPMSG2[] =
            "8.  -callback \t[index]\t[count]\t[payload]\r\n"
            "9.  -ticker \t[index]\t[delay]\t[count]\t[payload]\r\n"
            "10. -reg \t[dest]\t[operation]\t[SA]\t[SB]\r\n"
            "11. -script \t[operation]\t[line#]\t[command]\r\n"
            "12. -if\t\tSA\tCOND\tSB\t? -FTN-T : FTN-F\r\n"
            "13. -uart\tFTN\tabc"
            "\r\n";
    char HELPMSG3[] =
            "14. -sine\tfreq\r\n"
            "15. -netudp\tIP:PORT\tpayload";

    char HELP_HELP[] = "-Shows list of available commands.";
    char HELP_ABOUT[] = "-Shows Author Name, Build details, and Time of build.";
    char HELP_PRINT[] = "-print [message] -Prints characters entered to the Terminal.";
    char HELP_MEMR[] = "-memr [address] -Read data in entered memory location.";
    char HELP_GPIO[] = "-gpio [pin] [r,w,t] [0,1] -read, write, or toggle a selected gpio pin.";
    char HELP_ERROR[] = "-error [errorNum] -shows list of errors and count.";
    char HELP_TIMER[] = "-timer  | [start/stop/period]\r\n"
            "\t| start : Enables Timer 1\r\n"
            "\t| stop  : Disables Timer 1\r\n"
            "\t| period[integer] : sets the period of Timer 1";
    char HELP_CALLBACK[] = "-callback| [index] [count] [command]\r\n"
            "\t | Index : which callback to use\r\n"
            "\t | Count : Number of calls [-1 forever]\r\n"
            "\t | Payload : command to parse.";
    char HELP_TICKER[] = "-ticker | [index] [delay] [period] [count] [payload]\r\n"
            "\t| index : 0,1,2 index of ticker\r\n"
            "\t| delay : initial delay before payload is parsed\r\n"
            "\t| period : concurrent delay\r\n"
            "\t| count : number of times to parse payload\r\n"
            "\t| payload : command to parse";
    char HELP_REG[] = "-reg\t| [dest] [operator] [src]\r\n"
            "\t| dest : index of reg to store result of operation\r\n"
            "\t| operator : +, -, ++, --, ?, =, /, *, !, ~, &, ^, |, %, >, <, x\r\n"
            "\t| SA : Number or Reg index to use for 1st value\r\n"
            "\t| SB : Number or Reg index to use for 2st value\r\n"
            "\t| (#SA or #SB to use immediate value)";
    char HELP_SCRIPTS[] = "-script | [line#] [command]\r\n"
            "\t| line#: which scripts to use [0-31]\r\n"
            "\t| command: command to parse";
    char HELP_IF[] = "-if\tSA\t\tCOND\tSB\t? -FTN-T\t: FTN-F\r\n"
            "\t| SA and SB are register contents\r\n"
            "\t| COND is [>, =, <]\r\n"
            "\t| If COND is True perform FTN-T. Otherwise, perform FTN-F\r\n"
            "-if [0-31, #] [>, =, <] [0-31, #] ? FTN-T\tFTN-F";
    char HELP_UART[] = "-uart\tFTN\tabc\r\n"
            "\t| FTN command to send through UART7\r\n"
            "\t| abc arguments of FTN";
    char HELP_SINE[] = "-sine\tfreq\r\n"
            "\t| frequency of sine wave to play";
    char HELP_NETUDP[] ="-netudp IP:PORT payload\r\n"
            "\t| IP:   IP Address of destination\r\n"
            "\t| PORT: PORT Number of destination\r\n"
            "\t| payload: data to send.";

    strcpy(Glo.xmitbuff, str);
    getNextStr(Glo.print, Glo.xmitbuff);
    if(strstr(Glo.print,            "print")    != 0)
        UARTWrite(HELP_PRINT);
    else if(strstr(Glo.print,       "help")     != 0)
        UARTWrite(HELP_HELP);
    else if(strstr(Glo.print,       "about")    != 0)
        UARTWrite(HELP_ABOUT);
    else if(strstr(Glo.print,       "memr")     != 0)
        UARTWrite(HELP_MEMR);
    else if(strstr(Glo.print,       "error")    != 0)
        UARTWrite(HELP_ERROR);
    else if(strstr(Glo.print,       "gpio")     != 0)
        UARTWrite(HELP_GPIO);
    else if(strstr(Glo.print,       "timer")    != 0)
        UARTWrite(HELP_TIMER);
    else if(strstr(Glo.print,       "callback") != 0)
        UARTWrite(HELP_CALLBACK);
    else if(strstr(Glo.print,       "ticker")   != 0)
        UARTWrite(HELP_TICKER);
    else if(strstr(Glo.print,       "reg")      != 0){
        UARTWrite(HELP_REG);
    }else if(strstr(Glo.print,      "script")   != 0)
        UARTWrite(HELP_SCRIPTS);
    else if(strstr(Glo.print,       "if")       != 0)
        UARTWrite(HELP_IF);
    else if(strstr(Glo.print,       "uart")     != 0)
        UARTWrite(HELP_UART);
    else if(strstr(Glo.print,       "sine")     != 0)
        UARTWrite(HELP_SINE);
    else if(strstr(Glo.print,       "netudp")   != 0)
        UARTWrite(HELP_NETUDP);
    else{
        UART_write(Glo.uart, &HELPMSG1, strlen(HELPMSG1));
        UART_write(Glo.uart, &HELPMSG2, strlen(HELPMSG2));
        UART_write(Glo.uart, &HELPMSG3, strlen(HELPMSG3));
    }
    clearBuffer(Glo.xmitbuff);
    clearBuffer(Glo.print);
    //UARTWrite("");
}

void MPMemr(char *str){
    uint32_t address;
    int val;
    char *ptr;
    //char RETURN[] = "\r\n";
    //int32_t index = Glo.PayQ.payloadReading;
    strcpy(Glo.MsgBuff, str);
    getNextStr(Glo.xmitbuff, Glo.MsgBuff);
    address = 0xFFFFFFFC & strtol(Glo.xmitbuff, &ptr, 16);

    if(address > 0x000FFFF3 && address < 0x20000000){                           // greater than FLASH less than FRAM
        sprintf(Glo.xmitbuff, "Address out of range. Resetting to 0x0.");
        UARTWrite(Glo.xmitbuff);
        Glo.ErrorCounts[2]++;
        UARTWrite(Glo.ErrorMessages[2]);
        address = 0x0;
    }
    else if(address > 0x2003FFF3 && address < 0x40000000){                       //greater than SRAM less than Peripherals
        sprintf(Glo.xmitbuff, "Address out of range. Resetting to 0x0.");
        UART_write(Glo.uart, Glo.xmitbuff, strlen(Glo.xmitbuff));
        Glo.ErrorCounts[2]++;
        UART_write(Glo.uart, &Glo.ErrorMessages[2], strlen(Glo.ErrorMessages[2]));
        address = 0x0;
    }else if(address > 0x44054FF3){                                              // greater than peripherals
        sprintf(Glo.xmitbuff, "Address out of range. Resetting to 0x0.");
        UART_write(Glo.uart, Glo.xmitbuff, strlen(Glo.xmitbuff));
        Glo.ErrorCounts[2]++;
        UART_write(Glo.uart, &Glo.ErrorMessages[2], strlen(Glo.ErrorMessages[2]));
        address = 0x0;
    }else{
        val = *(int32_t *)(address + 0xC);
        sprintf(Glo.xmitbuff, "%#010x : %#010x ", (address+0xC), val);
        UARTWrite(Glo.xmitbuff);
        val = *(int32_t *)(address + 0x8);
        sprintf(Glo.xmitbuff, "%#010x : %#010x ", (address+0x8), val);
        UARTWrite(Glo.xmitbuff);
        val = *(int32_t *)(address + 0x4);
        sprintf(Glo.xmitbuff, "%#010x : %#010x ", (address+0x4), val);
        UARTWrite(Glo.xmitbuff);
        val = *(int32_t *)(address + 0x0);
        sprintf(Glo.xmitbuff, "%#010x : %#010x ", (address+0x0), val);
        UARTWrite(Glo.xmitbuff);

    }
    memset(Glo.MsgBuff, 0 , 128);
    memset(Glo.xmitbuff, 0, 128);
}

void MPGpio(char *str){
    uint_fast8_t val;
    strcpy(Glo.MsgBuff, str);
    char *loc = MPgetNextStr(Glo.MsgBuff, true);

    if(loc && atoi(loc) >= 0 && atoi(loc) <= 7){
        Glo.gpioSel = atoi(loc);
        loc = MPgetNextStr(loc, true);
        if(loc){
             if(*(loc) == 'r'){
                  val = GPIO_read(Glo.gpioSel);
                  sprintf(Glo.xmitbuff, "GPIO %d : %d", Glo.gpioSel, val);
                  UARTWrite(Glo.xmitbuff);
             } else if(*(loc) == 'w'){
                 loc = MPgetNextStr(loc, true);
                 if(loc){
                     val = atoi(loc);
                     GPIO_write(Glo.gpioSel, val);
                 } else {
                     UART_write(Glo.uart, &Glo.ErrorMessages[0], strlen(Glo.ErrorMessages[0]));
                     Glo.ErrorCounts[0]++;
                 }
             } else if(*(loc) == 't'){
                   GPIO_toggle(Glo.gpioSel);
            }else{
                   Glo.ErrorCounts[0]++;
                   UART_write(Glo.uart, &Glo.ErrorMessages[0], strlen(Glo.ErrorMessages[0]));
            }
        } else {
            val = GPIO_read(Glo.gpioSel);
            sprintf(Glo.xmitbuff, "GPIO %d : %d", Glo.gpioSel, val);
            UARTWrite(Glo.xmitbuff);
            clearBuffer(Glo.xmitbuff);
        }
    } else {
        int i;
        for(i = 0; i <= 7; i++){
            sprintf(Glo.xmitbuff, "\r\nGPIO %d : %d", i, GPIO_read(i));
            UARTWriteProtected(Glo.xmitbuff);
        }
        UARTWriteProtected("\r\n");
        UARTWriteProtected("\r\n");
    }
    return;
}

void MPError(char *str){
    strcpy(Glo.MsgBuff, str);
    char *loc = MPgetNextStr(Glo.MsgBuff, true);
    int i;
    if(loc == NULL){
        for(i = 0; i < ERRORMSGSIZE; i++){
            sprintf(Glo.xmitbuff, "%d : %d : %s", i, Glo.ErrorCounts[i], Glo.ErrorMessages[i]);
            UARTWrite(Glo.xmitbuff);
        }
    } else if(atoi(loc) > 0 && atoi(loc) < ERRORMSGSIZE){
        int val = atoi(loc);
        loc = MPgetNextStr(loc, true);
        if(loc && *(loc) == 'c'){
            Glo.ErrorCounts[0] = 0;
            Glo.ErrorCounts[1] = 0;
            Glo.ErrorCounts[2] = 0;
            Glo.ErrorCounts[3] = 0;
        } else {
            sprintf(Glo.xmitbuff, "%d : %d : %s", val, Glo.ErrorCounts[val], Glo.ErrorMessages[val]);
            UARTWrite(Glo.xmitbuff);
        }

    } else {
        Glo.ErrorCounts[0]++;
        UARTWrite(Glo.ErrorMessages[0]);
    }
    return;
}

void MPPrint(char *str){
    getNextStr(Glo.print, str);
    UARTWrite(Glo.print);
    clearBuffer(Glo.print);
}

void UARTWrite(char *str){
    Semaphore_pend(Glo.Bios.UARTSem, BIOS_WAIT_FOREVER);

    char RETURN[] = "\r\n";
    UART_write(Glo.uart, &RETURN, strlen(RETURN));
    UART_write(Glo.uart, str, strlen(str));
    UART_write(Glo.uart, &RETURN, strlen(RETURN));
    Semaphore_post(Glo.Bios.UARTSem);
}
void UARTWriteProtected(char str[]){
    Semaphore_pend(Glo.Bios.UARTSem, BIOS_WAIT_FOREVER);
    UART_write(Glo.uart, str, strlen(str));
    Semaphore_post(Glo.Bios.UARTSem);
}
void UARTAddByte(char *input){
    Semaphore_pend(Glo.Bios.UARTSem, BIOS_WAIT_FOREVER);
    UART_write(Glo.uart, input, 1);
    Semaphore_post(Glo.Bios.UARTSem);
}

void writeShadow(int32_t index, int32_t value)
{
    if(index > 0 && index < SHADOWCOUNT)
    {
        if(Glo.shadow[index])
            Glo.reg[index] = value;
    }
}

void getNextStr(char print[], char cmd[]){
    clearBuffer(print);
    int i;
    for(i = 0; i < strlen(cmd); i++){
            if(cmd[i] == ' ' && cmd[i+1] != ' '){
                i++;
                break;
            }
            //cmd[i] = 0;
    }
    int j;
    for(j = 0; j < strlen(cmd); j++){
        print[j] = cmd[i + j];

    }
}
char *MPgetNextStr(char buf[], bool flag){
    char *loc;
    if(buf == NULL) return NULL;

    loc = strchr(buf, ' ');

    if(flag)
        while(*loc == ' ')
            loc++;
    else
        if(*loc == ' ')
            loc++;

    if(!*loc)
        return NULL;

    return loc;
}

uint16_t MatchString(char input[], char cmd[]){
    int i;
    for(i = 0; i < strlen(cmd); i++){
        if(input[i] != cmd[i]) return 1;
        else continue;
    }
    return 0;
}

void clearBuffer(char buf[]){
    memset(buf, 0, 128);
}

