/*
 *  ======== main_tirtos.c ========
 */

/* RTOS header files */

#include <ti/drivers/Board.h>

#include "p100.h"


extern void ti_ndk_config_Global_startupFxn();
extern void mainThread(void *arg0);


/*
 *  ======== main ========
 */
int main(void)
{
    InitializeDrivers();
    Initialize();

    ti_ndk_config_Global_startupFxn();

    BIOS_start();

    return (0);
}


/*
 * 4 Nov 2022
 * for -sine 125 that is 8000Hz sampling
 * then -sine 3999 is the max
 *
 * gateSWI_Handle
 * these are useful because you can get into trouble if you pend a semaphore in an HWI
 * gates can be used to turn off inturrupts while a certain snippit of code runs.
 * this can delay timers and such unless the code has low run-time.
 * gates prevent two things from running the same code at the same time
 *
 *
 *
 * 19 Oct 2022
 * Sine wave notes
 * Domain is number of samples / wave
 * sine ranges from 0 to 8142
 * if we sample at 8000 [samples/sec] / 256 [samples/wave] = 31.25 Hz
 *                             ''     / 257                = 31.12 Hz
 *              -we can only sample at most a 4000 Hz wave (Nyquist)
 *              -Dr Nutter uses 257 [samples / wave]
 * DTMF (Dual tone multi frequency) Chart -Not the best solution, lacks flexibility but saves memory
 *                                        -but we also dont like Taylor Series Expansion because need around 11 terms just for a 16 bit wave and takes alot of processing
 * [122] 89
 * [123] 62     if we want 124.5, we take the average (39 + 22) / 2 = 31
 * [124] 39
 * [125] 22
 * [126] 10
 *                                        -
 * 21 October 2022
 * (ROV) RTOS Object view allows you to view what is going on with Tasks, Semaphores, SWIs etc.
 * DONT PRINT FROM HWI (AddPayload instead)
 *
 * 24 October 2022z\
 * for UART7 need to set param: uartParams.readEcho = UART_ECHO_OFF
 * uartParams.writeDataMode = UART_DATA_BINARY;
 * uartParams.readDataMode = UART_DATA_TEXT;
 * uartParams. readReturnMode = UART_RETURN_NEWLINE;
 * uartParams.baudrate = 115200;
 * uartParams.readEcho = UART_ECHO_OFF;
 *
 *  SINE lookupTable has 256 entries
 *  To get lookup table values use excel t = 0 to 256, then sine(t(0 to 2pi))
 *
 * SCRIPT 32 -timer 125 (8000  HZ)
 * SCRIPT 33 -gpio 6 w 0
 * SCRIPT 34 -sine 200
 * SCRIPT 35 -callback 0 -1 -sine
 *
 *
 */
