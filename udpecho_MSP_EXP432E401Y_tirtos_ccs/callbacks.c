/*
 * Callbacks.c
 * Contains callbacks for timer, buttons, and ticker functions.
 */
#include "p100.h"

#define CALLBACKCOUNT       3


int32_t AddPayload(char *payload){
    int32_t index;

    if(!payload || payload[0] == 0)
        return -1;

    index = Glo.PayQ.payloadWriting;
    Semaphore_pend(Glo.Bios.PayloadLowP1WriteSem, BIOS_WAIT_FOREVER);
    strcpy(Glo.PayQ.payloads[index].payload, payload);
    Glo.PayQ.payloads[index].payloadCount = strlen(payload);
    Glo.PayQ.payloadWriting++;
    if(Glo.PayQ.payloadWriting >= PAYLOADCOUNT)
        Glo.PayQ.payloadWriting = 0;
    Semaphore_post(Glo.Bios.PayloadLowP1WriteSem);
    Semaphore_post(Glo.Bios.PayloadLowP1Sem);

    return index;
}

int32_t AddTicker(int32_t index, int32_t delay, int32_t period, int32_t count, char *payload){
    if(index < 0 || index >= TICKERCOUNT)
        return index;
    Semaphore_pend(Glo.Bios.TickerSem, BIOS_WAIT_FOREVER);
    Glo.tickers[index].delay = delay;
    Glo.tickers[index].count = count;
    Glo.tickers[index].period = period;
    strcpy(Glo.tickers[index].payload, payload);
    Semaphore_post(Glo.Bios.TickerSem);
    return index;
}

int32_t AddCallback(int32_t index, int32_t count, char *payload){  //used for -callback add count payload
    if(index < 0 || index >= CALLBACKCOUNT)
        return index;
    Semaphore_pend(Glo.Bios.CallbackSem, BIOS_WAIT_FOREVER);
    Glo.callbacks[index].count = count;
    strcpy(Glo.callbacks[index].payload, payload);
    Semaphore_post(Glo.Bios.CallbackSem);

    return index;
}

void timer0Callback(Timer_Handle timer, int_fast16_t status){
    Swi_post(Glo.Bios.Timer0SWI);
}

void SWITimer0(UArg arg0, UArg arg1){
    int32_t i = 0;
    //uint32_t gateKey;

    //gateKey = GateSwi_enter(Glo.Bios.CallbackGate);
    Semaphore_pend(Glo.Bios.CallbackSem, BIOS_WAIT_FOREVER);
    if(Glo.callbacks[i].count != 0)
    {
        if(Glo.callbacks[i].count > 0)
            Glo.callbacks[i].count--;
        if(MatchString("-sine", Glo.callbacks[i].payload) == 0)
            MPSine(Glo.callbacks[i].payload);
        else
            AddPayload(Glo.callbacks[i].payload);
    }
    //GateSwi_leave(Glo.Bios.CallbackGate, gateKey);
    Semaphore_post(Glo.Bios.CallbackSem);
}

void timer1Callback(Timer_Handle timer, int_fast16_t status){
    //Swi_post(Glo.Bios.Timer1SWI);
    Semaphore_pend(Glo.Bios.TickerSem,  BIOS_WAIT_FOREVER);
    int32_t i;
    for(i = 0; i < TICKERCOUNT; i++)
    {
        if(Glo.tickers[i].delay > 0)
        {
            Glo.tickers[i].delay--;
            if(Glo.tickers[i].delay <= 0)
            {
                if(Glo.tickers[i].count != 0)
                {
                    Glo.tickers[i].delay = Glo.tickers[i].period;
                    if(Glo.tickers[i].count > 0)
                        Glo.tickers[i].count--;
                }
                AddPayload(Glo.tickers[i].payload);
            } //delay
        }//delay
    }//tickercount
    Semaphore_post(Glo.Bios.TickerSem);
}

void SWITimer1(UArg arg0, UArg arg1){
    /*int32_t i;

    Semaphore_pend(Glo.Bios.TickerSem,  BIOS_WAIT_FOREVER);
    for(i = 0; i < TICKERCOUNT; i++)
    {
        if(Glo.tickers[i].delay > 0)
        {
            Glo.tickers[i].delay--;
            if(Glo.tickers[i].delay <= 0)
            {
                if(Glo.tickers[i].count != 0)
                {
                    Glo.tickers[i].delay = Glo.tickers[i].period;
                    if(Glo.tickers[i].count > 0)
                        Glo.tickers[i].count--;
                }
                AddPayload(Glo.tickers[i].payload);
            } //delay
        }//delay
    }//tickercount
    Semaphore_post(Glo.Bios.TickerSem);*/
}

void gpioButtonSW1Callback(uint_least8_t index){
    int32_t i = 1;

    Semaphore_pend(Glo.Bios.CallbackSem, BIOS_WAIT_FOREVER);
    if(Glo.callbacks[i].count != 0){
        if(Glo.callbacks[i].count > 0)
            Glo.callbacks[i].count--;
        AddPayload(Glo.callbacks[i].payload);
    }
    Semaphore_post(Glo.Bios.CallbackSem);
}

void gpioButtonSW2Callback(uint_least8_t index){
    int32_t i = 2;

    Semaphore_pend(Glo.Bios.CallbackSem, BIOS_WAIT_FOREVER);
    if(Glo.callbacks[i].count != 0){
        if(Glo.callbacks[i].count > 0)
            Glo.callbacks[i].count--;
        AddPayload(Glo.callbacks[i].payload);
    }
    Semaphore_post(Glo.Bios.CallbackSem);
}

