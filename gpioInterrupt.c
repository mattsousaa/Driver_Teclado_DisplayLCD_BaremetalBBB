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
 *         Author:  Mateus Sousa, mateuseng_ec@alu.ufc.br
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
static volatile unsigned int flagIsr = 0;
int seq = 0;
int mSec=200;
unsigned int mod = 1;
unsigned int led = 16; // PIN15 - P9
unsigned int but = 12; // PIN12 - P8
unsigned int but2 = 2; // PIN5 - P8
//unsigned int but = 28;
unsigned int led2 = 17; // PIN23 - P9
unsigned int led3 = 14; // PIN16 - P8

/*****************************************************************************
**                INTERNAL MACRO DEFINITIONS
*****************************************************************************/
#define PIN_HIGH    1
#define PIN_LOW     0

#define TIMER_INITIAL_COUNT            (0xFF000000u)
#define TIMER_RLD_COUNT                (0xFFFFFF83u) //(0xFF000000u) 

/* Values denoting the Interrupt Line number to be used. */
#define GPIO_INTC_LINE_1                  (0x0)
#define GPIO_INTC_LINE_2                  (0x1)

/*
** Values used to enable/disable interrupt generation due to level
** detection on an input GPIO pin.
*/
#define GPIO_INTC_TYPE_NO_LEVEL           (0x01)
#define GPIO_INTC_TYPE_LEVEL_LOW          (0x04)
#define GPIO_INTC_TYPE_LEVEL_HIGH         (0x08)
#define GPIO_INTC_TYPE_BOTH_LEVEL         (0x0C)

/*
** Values used to enable/disable interrupt generation due to edge
** detection on an input GPIO pin.
*/
#define GPIO_INTC_TYPE_NO_EDGE            (0x80)
#define GPIO_INTC_TYPE_RISE_EDGE          (0x10)
#define GPIO_INTC_TYPE_FALL_EDGE          (0x20)
#define GPIO_INTC_TYPE_BOTH_EDGE          (0x30)

#define T_1MS_COUNT                     (0x5DC0u) 
#define OVERFLOW                        (0xFFFFFFFFu)
#define TIMER_1MS_COUNT                 (0x5DC0u)

/*****************************************************************************
**                INTERNAL FUNCTION PROTOTYPES
*****************************************************************************/
static void         DMTimerSetUp(void);
static void         Delay(volatile unsigned int minSec);
static void         initSerial(void);
int                 asciiToInt (char ascii);
void                intToAscii (int dec, int c_op);
int                 count_op (int op);
/******************************************************************************
**              FUNCTION DEFINITIONS
******************************************************************************/

int main(){

int i;
int j = 0;
int z = 0;
int nL;
int nC;
int num1;
int num2;
int sin_op;
int flagC = 0;
int flagD = 0;
int n_neg = 0;
int n_neg2 = 0;
char vet1[5];
char vet2[5];
int w = 0;
int k = 0;
int a = 1;
int b = 0;
int pinosLinhas[] = {4,0,1,5};
int pinosColunas[] = {2,6,3,7};
char teclas[4][4] = {{'1','2','3','A'},
                     {'4','5','6','B'},
                     {'7','8','9','C'},
                     {'*','0','#','D'}};

    /* Initialize the UART console */
    ConsoleUtilsInit();

     /* This function will enable clocks for the DMTimer2 instance */
    DMTimer2ModuleClkConfig();

     /* Perform the necessary configurations for DMTimer */
    DMTimerSetUp();

    /* Select the console type based on compile time check */
    ConsoleUtilsSetType(CONSOLE_UART);

    unsigned int modAddr=SOC_GPIO_1_REGS, mod=GPIO1, pin=13, dir=GPIO_DIR_OUTPUT ;

    // PIN11 - P8 - GPIO1_13

    initSerial();

    GPIOModuleClkConfig(mod);

    /* Enabling functional clocks for GPIO instance. */
    
    switch(mod) {
        case GPIO0:
            modAddr = SOC_GPIO_0_REGS;  
            break;
        case GPIO1: 
            modAddr = SOC_GPIO_1_REGS;  
            break;
        case GPIO2: 
            modAddr = SOC_GPIO_2_REGS;  
            break;
        case GPIO3: 
            modAddr = SOC_GPIO_3_REGS;  
            break;
        default:    
            break;
    }/* -----  end switch  ----- */

    // Buzzer

    GPIOPinMuxSetup(mod, pin);
    GPIODirModeSet(modAddr, pin, GPIO_DIR_OUTPUT);

    /* Selecting GPIO pin for use. */

    for(i = 0; i <= 3; i++){
        GPIOPinMuxSetup(mod, pinosLinhas[i]);
        GPIOPinMuxSetup(mod, pinosColunas[i]);
    }

    /* Varrendo as linhas deixando como OUTPUT e nível BAIXO */

    for(nL = 0; nL <= 3; nL++){
        /* Setting the GPIO pin as an output pin. */
        GPIODirModeSet(modAddr, pinosLinhas[nL], GPIO_DIR_OUTPUT);
        /* Driving a logic HIGH on the GPIO pin. */
        GPIOPinWrite(modAddr, pinosLinhas[nL], PIN_LOW);
    }

    /* Varrendo as colunas deixando como INPUT */

    for(nC = 0; nC <= 3; nC++){
        /* Setting the GPIO pin as an input pin. */
        GPIODirModeSet(modAddr, pinosColunas[nC], GPIO_DIR_INPUT);
    } 

while(1){

    //faz varredura em todas as linhas, desligando uma de cada vez
    for(nL = 0; nL <= 3; nL++){

        GPIOPinWrite(modAddr, pinosLinhas[nL], PIN_HIGH);
      
        //faz varredura em todas as colunas verificando se tem algum botao apertado
        for(nC = 0; nC <= 3; nC++){

            if(GPIOPinRead(SOC_GPIO_1_REGS, pinosColunas[nC])){

                //ConsoleUtilsPrintf("Linha %d, Coluna %d \n", nL+1, nC+1);
                //ConsoleUtilsPrintf("Tecla %c pressionada. \n", teclas[nL][nC]);
                GPIOPinWrite(modAddr, pin, PIN_HIGH);
                Delay(150);
                GPIOPinWrite(modAddr, pin, PIN_LOW);

                    if(b == 1){
                        //ConsoleUtilsPrintf("Boi Boi\n");
                        if(teclas[nL][nC] != 'A' && teclas[nL][nC] != 'B' && teclas[nL][nC] != 'C'
                        && teclas[nL][nC] != 'D' && teclas[nL][nC] != '*' && teclas[nL][nC] != '#'){
                            vet2[w] = teclas[nL][nC];
                            w++;
                            flagD++;
                        }else{
                            if(flagD != 0){
                                vet2[w+1] = '\0';
                                //ConsoleUtilsPrintf("Numero 2: %s\n", vet2);
                                //ConsoleUtilsPrintf("Boi\n");
                                b = 0;
                            }else{
                                ConsoleUtilsPrintf("Boi Boi\n");
                                ConsoleUtilsPrintf("Numero negativo\n");
                                n_neg2 = 1;
                            }
                        }
                    }

                    if(a == 1){
                        if(teclas[nL][nC] != 'A' && teclas[nL][nC] != 'B' && teclas[nL][nC] != 'C'
                        && teclas[nL][nC] != 'D' && teclas[nL][nC] != '*' && teclas[nL][nC] != '#'){
                            vet1[k] = teclas[nL][nC];
                            k++;
                            flagC++;
                        }else{
                            if(flagC != 0){
                                vet1[k+1] = '\0';
                                //ConsoleUtilsPrintf("Numero 1: %s\n", vet1);
                                //ConsoleUtilsPrintf("Boi\n");
                                if(teclas[nL][nC] == 'A'){
                                    sin_op = 1;
                                }
                                    if(teclas[nL][nC] == 'B'){
                                        sin_op = 2;
                                    }
                                        if(teclas[nL][nC] == 'C'){
                                        sin_op = 3;
                                    }
                                            if(teclas[nL][nC] == 'D'){
                                                sin_op = 4;
                                            }
                                a = 0;
                                b = 1;
                            }else{
                                n_neg = 1;
                                ConsoleUtilsPrintf("Boi Boi\n");
                                ConsoleUtilsPrintf("Numero negativo\n");
                            }
                        }
                    }

            }
        }

        GPIOPinWrite(modAddr, pinosLinhas[nL], PIN_LOW);
    }

    Delay(150);

    if(a == 0 && b == 0){
        ConsoleUtilsPrintf("Boi Boi\n");
        break;
    }
}

while(vet1[j] != '\0'){
    j++;
}

while(vet2[z] != '\0'){
    z++;
}

    ConsoleUtilsPrintf("Numero 1: %s\n", vet1);
    ConsoleUtilsPrintf("Numero 1: %s\n", vet2);
    ConsoleUtilsPrintf("Quantidade de numeros em num1: %d\n", j);
    ConsoleUtilsPrintf("Quantidade de numeros em num2: %d\n", z);

if(j == 1){
    num1 = asciiToInt(vet1[0]);
    //ConsoleUtilsPrintf("1 numero: %d\n", num1);
}
if(j == 2){
    num1 = asciiToInt(vet1[1]);
    num1 += asciiToInt(vet1[0])*10;
    //ConsoleUtilsPrintf("1 numero: %d\n", num1);
}
if(j == 3){
    num1 = asciiToInt(vet1[2]);
    num1 += asciiToInt(vet1[1])*10;
    num1 += asciiToInt(vet1[0])*100;
    //ConsoleUtilsPrintf("1 numero: %d\n", num1);
}
if(j == 4){
    num1 = asciiToInt(vet1[3]);
    num1 += asciiToInt(vet1[2])*10;
    num1 += asciiToInt(vet1[1])*100;
    num1 += asciiToInt(vet1[0])*1000;
    //ConsoleUtilsPrintf("1 numero: %d\n", num1);
}
if(j == 5){
    num1 = asciiToInt(vet1[4]);
    num1 += asciiToInt(vet1[3])*10;
    num1 += asciiToInt(vet1[2])*100;
    num1 += asciiToInt(vet1[1])*1000;
    num1 += asciiToInt(vet1[0])*10000;
    //ConsoleUtilsPrintf("1 numero: %d\n", num1);
}
if(z == 1){
    num2 = asciiToInt(vet2[0]);
    //ConsoleUtilsPrintf("2 numero: %d\n", num2);
}
if(z == 2){
    num2 = asciiToInt(vet2[1]);
    num2 += asciiToInt(vet2[0])*10;
    //ConsoleUtilsPrintf("2 numero: %d\n", num2);
}
if(z == 3){
    num2 = asciiToInt(vet2[2]);
    num2 += asciiToInt(vet2[1])*10;
    num2 += asciiToInt(vet2[0])*100;
    //ConsoleUtilsPrintf("2 numero: %d\n", num2);
}
if(z == 4){
    num2 = asciiToInt(vet2[3]);
    num2 += asciiToInt(vet2[2])*10;
    num2 += asciiToInt(vet2[1])*100;
    num2 += asciiToInt(vet2[0])*1000;
    //ConsoleUtilsPrintf("2 numero: %d\n", num2);
}
if(z == 5){
    num2 = asciiToInt(vet2[4]);
    num2 += asciiToInt(vet2[3])*10;
    num2 += asciiToInt(vet2[2])*100;
    num2 += asciiToInt(vet2[1])*1000;
    num2 += asciiToInt(vet2[0])*10000;
    //ConsoleUtilsPrintf("2 numero: %d\n", num2);
}

if(n_neg == 1){
    num1 *= (-1);
}

if(n_neg2 == 1){
    num2 *= (-1);
}

ConsoleUtilsPrintf("1 numero: %d\n", num1);

ConsoleUtilsPrintf("2 numero: %d\n", num2);

if(sin_op == 1){
    ConsoleUtilsPrintf("Soma: %d\n", num1 + num2);
}

if(sin_op == 2){
    ConsoleUtilsPrintf("Subtracao: %d\n", num1 - num2);
}

if(sin_op == 3){
    ConsoleUtilsPrintf("Multiplicacao: %d\n", num1 * num2); 
}

if(sin_op == 4){
    ConsoleUtilsPrintf("Divisao: %d\n", num1 / num2);    
}

    //ConsoleUtilsPrintf("Soma: %d\n", num1 + num2); 
    //ConsoleUtilsPrintf("Subtracao: %d\n", num1 - num2); 
    //ConsoleUtilsPrintf("Multiplicacao: %d\n", num1 * num2); 
    //ConsoleUtilsPrintf("Divisao: %d\n", num1 / num2);    

    //ConsoleUtilsPrintf("#####  exit system  #####\n");
    
    return(0);
}

/*FUNCTION*-------------------------------------------------------
*
* A function which is used to generate a delay.
*END*-----------------------------------------------------------*/

static void Delay(volatile unsigned int minSec){
   while(minSec != 0){
        DMTimerCounterSet(SOC_DMTIMER_2_REGS, 0);
        DMTimerEnable(SOC_DMTIMER_2_REGS);
        while(DMTimerCounterGet(SOC_DMTIMER_2_REGS) < TIMER_1MS_COUNT);
        DMTimerDisable(SOC_DMTIMER_2_REGS);
        minSec--;
    }
}

/*FUNCTION*-------------------------------------------------------
*
* A function which is used to initialize UART.
*END*-----------------------------------------------------------*/
static void initSerial(){
    /* Initialize console for communication with the Host Machine */
        ConsoleUtilsInit();

        /* Select the console type based on compile time check */
        ConsoleUtilsSetType(CONSOLE_UART);
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

int asciiToInt (char ascii){

        if (ascii < '0' || ascii > '9') {
            return -1; //error
        }
            else{
                return (int)(ascii - '0'); // This works because '0' has the int value 48 in Ascii and '1' 49 and so on.
            }
}

void intToAscii (int dec, int c_op){

register int i;
int j = 0;

char str[c_op];
char aux[c_op];

for (i = 0; i < c_op; i++){
    if(dec == 0) 
        break;
    str[i] = (dec % 10) + 48;
    dec /= 10;
}

str[i] = 0x0;

for(i = 0; i < c_op; i++){
    aux[c_op-i-1] = str[i];
}

aux[c_op] = '\0';
//printf("%s\n", str);
printf("%s\n", aux);

//return str;

}

int count_op(int op){

int count = 0;

    while(op > 0){
        op = op / 10;
        count = count + 1;  
    }

    return count;
}



/******************************* End of file *********************************/
