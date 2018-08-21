/*
 * =====================================================================================
 *
 *       Filename:  gpioInterrupt.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/06/2017 09:36:27
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Francisco Helder (FHC), helderhdw@gmail.com
 *   Organization:  UFC-Quixadá
 *
 * =====================================================================================
 */

#include "uart_irda_cir.h"
#include "soc_AM335x.h"
#include "interrupt.h"
#include "board.h"
#include "dmtimer.h"
#include "error.h"
#include "beaglebone.h"
#include "gpio_v2.h"
#include "consoleUtils.h"

/******************************************************************************
**                      INTERNAL VARIABLE DEFINITIONS
*******************************************************************************/
static volatile unsigned int flagIsr;
/*****************************************************************************
**                INTERNAL MACRO DEFINITIONS
*****************************************************************************/
#define PIN_HIGH    1
#define PIN_LOW     0
#define BIT_WIDTH 8

#define TIMER_INITIAL_COUNT            (0xFF000000u)
#define TIMER_RLD_COUNT                (0xFFFFFF83u) //(0xFF000000u)

#define T_1MS_COUNT                     (0x5DC0u) 
#define OVERFLOW                        (0xFFFFFFFFu)
#define TIMER_1MS_COUNT                 (0x5DC0u) 

/******************************************************************************
**              FUNCTION DEFINITIONS
******************************************************************************/

/*FUNCTION*-------------------------------------------------------
*
* A function which is used to generate a delay.
*END*-----------------------------------------------------------*/

static void initLed(unsigned int baseAdd, unsigned int module, unsigned int pin){
    
        /* Selecting GPIO pin for use. */
        GPIOPinMuxSetup(module, pin);
    
        /* Setting the GPIO pin as an output pin. */
        GPIODirModeSet(baseAdd, pin, GPIO_DIR_OUTPUT);
}

static void DMTimerSetUp(void){
    DMTimerReset(SOC_DMTIMER_2_REGS);

    /* Load the counter with the initial count value */
    //DMTimerCounterSet(SOC_DMTIMER_2_REGS, TIMER_INITIAL_COUNT);

    /* Load the load register with the reload count value */
    //DMTimerReloadSet(SOC_DMTIMER_2_REGS, TIMER_RLD_COUNT);

    /* Configure the DMTimer for Auto-reload and compare mode */
    DMTimerModeConfigure(SOC_DMTIMER_2_REGS, DMTIMER_AUTORLD_NOCMP_ENABLE);
}

static unsigned  int getAddr(unsigned int module){
    unsigned int addr;

    switch (module) {
        case GPIO0:
            addr = SOC_GPIO_0_REGS; 
            break;
        case GPIO1: 
            addr = SOC_GPIO_1_REGS; 
            break;
        case GPIO2: 
            addr = SOC_GPIO_2_REGS; 
            break;
        case GPIO3: 
            addr = SOC_GPIO_3_REGS; 
            break;
        default:    
            break;
    }/* -----  end switch  ----- */

    return(addr);
}

static void Delay(volatile unsigned int mSec){
   while(mSec != 0){
        DMTimerCounterSet(SOC_DMTIMER_2_REGS, 0);
        DMTimerEnable(SOC_DMTIMER_2_REGS);
        while(DMTimerCounterGet(SOC_DMTIMER_2_REGS) < TIMER_1MS_COUNT);
        DMTimerDisable(SOC_DMTIMER_2_REGS);
        mSec--;
    }
}

void init_zero(){

    /* PINOS DE CONTROLE */

    GPIOPinWrite(getAddr(GPIO1), 16, PIN_LOW); // Define RS em 0
    GPIOPinWrite(getAddr(GPIO2), 4, PIN_LOW); // Define E em 0

    /* PINOS DE DADOS */

    GPIOPinWrite(getAddr(GPIO0), 7, PIN_LOW);  // D0 - PIN42 - P9
    GPIOPinWrite(getAddr(GPIO1), 12, PIN_LOW); // D1 - PIN12 - P8
    GPIOPinWrite(getAddr(GPIO0), 26, PIN_LOW); // D2 - PIN14 - P8
    GPIOPinWrite(getAddr(GPIO1), 14, PIN_LOW); // D3 - PIN16 - P8
    GPIOPinWrite(getAddr(GPIO2), 1, PIN_LOW);  // D4 - PIN18 - P8
    GPIOPinWrite(getAddr(GPIO1), 29, PIN_LOW); // D5 - PIN26 - P8
    GPIOPinWrite(getAddr(GPIO1), 13, PIN_LOW); // D6 - PIN11 - P8 
    GPIOPinWrite(getAddr(GPIO1), 15, PIN_LOW); // D7 - PIN15 - P8
}

void set_RS(){ // função que coloca o RS em 1 para enviar caracteres

    /* PINOS DE CONTROLE */

    GPIOPinWrite(getAddr(GPIO1), 16, PIN_HIGH); // Define RS em 1
    GPIOPinWrite(getAddr(GPIO2), 4, PIN_LOW); // Define E em 0

    /* PINOS DE DADOS */

    GPIOPinWrite(getAddr(GPIO0), 7, PIN_LOW);  // D0 - PIN42 - P9
    GPIOPinWrite(getAddr(GPIO1), 12, PIN_LOW); // D1 - PIN12 - P8
    GPIOPinWrite(getAddr(GPIO0), 26, PIN_LOW); // D2 - PIN14 - P8
    GPIOPinWrite(getAddr(GPIO1), 14, PIN_LOW); // D3 - PIN16 - P8
    GPIOPinWrite(getAddr(GPIO2), 1, PIN_LOW);  // D4 - PIN18 - P8
    GPIOPinWrite(getAddr(GPIO1), 29, PIN_LOW); // D5 - PIN26 - P8
    GPIOPinWrite(getAddr(GPIO1), 13, PIN_LOW); // D6 - PIN11 - P8 
    GPIOPinWrite(getAddr(GPIO1), 15, PIN_LOW); // D7 - PIN15 - P8
}

void send_data(int number){

    int i = 0;

    // MAPEAMENTO DE PINOS DE DADOS EM UM VETOR DE INTEIRO
    int pin_value[] = {7, 12, 26, 14, 1, 29, 13, 15, 4, 16};

    // MAPEAMENTO DOS MÓDULOS EM UM VETOR DE INTEIROS
    int mod_number[] = {GPIO0, GPIO1, GPIO0, GPIO1, GPIO2, GPIO1, GPIO1, GPIO1, GPIO2, GPIO1};

    /* 
    pin_value[8] = 4;  // Posição do vetor referente ao pino do E
    pin_value[9] = 16; // Posição do vetor referente ao pino do RS

    mod_number[8] = GPIO2; // Posição do vetor referente ao Módulo do E
    mod_number[9] = GPIO1; // Posição do vetor referente ao Módulo do RS
    */

int cont = 8;

while(cont > 0){


    for(i = 0; i < BIT_WIDTH; ++i){
        if(number & (1 << i)){
            GPIOPinWrite(getAddr(mod_number[i]), pin_value[i], PIN_HIGH);
            //delay();
        }
    }
    Delay(1);
    cont--;
}

    GPIOPinWrite(getAddr(mod_number[8]), pin_value[8], PIN_HIGH);
    Delay(1);
    GPIOPinWrite(getAddr(mod_number[8]), pin_value[8], PIN_LOW);
    Delay(1);
}

void init_LCD(){

    init_zero();

    send_data(0x80); 

    Delay(1);
    init_zero();

    send_data(0x30);
    Delay(1);
    init_zero();

    send_data(0x38);
    Delay(1);
    init_zero();

    send_data(0xF);
    Delay(1);
    init_zero();

    send_data(0x80);
    Delay(1);
}

void shift_right(){

    init_zero();

    send_data(0x1C); 

    Delay(1);
}

void shift_left(){

    init_zero();

    send_data(0x18); 

    Delay(1);
}

void delete_LCD(){

    init_zero();

    send_data(0x01);

    Delay(1);
}

int main(){

    //unsigned int modAddr=SOC_GPIO_1_REGS, mod=GPIO1, pin=13, dir=GPIO_DIR_OUTPUT ;

    /* This function will enable clocks for the DMTimer2 instance */
    DMTimer2ModuleClkConfig();

     /* Perform the necessary configurations for DMTimer */
    DMTimerSetUp();

    char letter[] = "#FicaHelder";
    int i = 0;

    GPIOModuleClkConfig(GPIO0);
    GPIOModuleClkConfig(GPIO1);
    GPIOModuleClkConfig(GPIO2);

    /* PINOS DE CONTROLE */

    initLed(getAddr(GPIO1), GPIO1, 16); // RS - PIN15 - P9
    initLed(getAddr(GPIO2), GPIO2, 4); // E  - PIN10 - P8

    /* PINOS DE DADOS */

    initLed(getAddr(GPIO0), GPIO0, 7);  // D0 - PIN42 - P9
    initLed(getAddr(GPIO1), GPIO1, 12); // D1 - PIN12 - P8
    initLed(getAddr(GPIO0), GPIO0, 26); // D2 - PIN14 - P8
    initLed(getAddr(GPIO1), GPIO1, 14); // D3 - PIN16 - P8
    initLed(getAddr(GPIO2), GPIO2, 1);  // D4 - PIN18 - P8
    initLed(getAddr(GPIO1), GPIO1, 29); // D5 - PIN26 - P8
    initLed(getAddr(GPIO1), GPIO1, 13); // D6 - PIN11 - P8
    initLed(getAddr(GPIO1), GPIO1, 15); // D7 - PIN15 - P8 

    init_LCD();

    shift_right();

    while(letter[i] != '\0'){
        set_RS();
        send_data(letter[i]);
        Delay(1);
        i++;
    }

    //ConsoleUtilsPrintf("#####  exit system  #####\n");
    
    return(0);
}




/******************************* End of file *********************************/
