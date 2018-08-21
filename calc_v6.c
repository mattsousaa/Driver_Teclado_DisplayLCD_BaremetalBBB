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
#include "lcd.h"

/******************************************************************************
**                      INTERNAL VARIABLE DEFINITIONS
*******************************************************************************/
static volatile unsigned int flagIsr;

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
static void         initButton(unsigned int, unsigned int, unsigned int);
static void         initLed(unsigned int, unsigned int, unsigned int);
static unsigned int getAddr(unsigned int);
static void         gpioIsr(void);
static void         gpioAintcConf(void);
static int          gpioPinIntConf(unsigned int, unsigned int, unsigned int);
static void         gpioPinIntEnable(unsigned int, unsigned int, unsigned int); 
static void         gpioIntTypeSet(unsigned int, unsigned int, unsigned int);
static void         initSerial(void);
int                 asciiToInt (char ascii);
void                intToAscii (int dec, int c_op);
int                 count_op (int op);
/******************************************************************************
**              FUNCTION DEFINITIONS
******************************************************************************/

int main(){

    int pinosLinhas[] = {4,0,1,5};      /* Mapeamento de GPIO's para as linhas do teclado */
    int pinosColunas[] = {2,6,3,7};     /* Mapeamento de GPIO's para as colunas do teclado */

    char teclas[4][4] = {{'1','2','3','A'},     /* Mapeamento de teclas */
                         {'4','5','6','B'},
                         {'7','8','9','C'},
                         {'*','0','#','D'}};

    unsigned int mod2 = 2, but = 6;

    unsigned int modAddr = SOC_GPIO_1_REGS, pin = 17, mod = 1;

    flagIsr = 0;

    /* Reset - HWREG(SOC_PRM_DEVICE_REGS) |= 0x2; */

    /* Enable IRQ in CPSR */
    IntMasterIRQEnable();

    /* Initialize the UART console */
    ConsoleUtilsInit();

    /* This function will enable clocks for the DMTimer2 instance */
    DMTimer2ModuleClkConfig();

    /* Perform the necessary configurations for DMTimer */
    DMTimerSetUp();

    /* Select the console type based on compile time check */
    ConsoleUtilsSetType(CONSOLE_UART);

    initSerial();

    GPIOModuleClkConfig(mod);
    GPIOModuleClkConfig(mod2);

    /* Buzzer - PIN23 - P9 - GPIO1_17 */

    initLed(getAddr(GPIO1), GPIO1, pin);
    initButton(getAddr(mod2), mod2, but);

    //GPIOPinMuxSetup(mod, pin);
    //GPIODirModeSet(modAddr, pin, GPIO_DIR_OUTPUT);

    /* configure gpio interrupt on the INTC */
    gpioAintcConf();

    gpioPinIntConf(SOC_GPIO_2_REGS, GPIO_INTC_LINE_1, but);
    
    /* Enable interrupt generation on detection of a rising or a falling edge.*/
    gpioIntTypeSet(SOC_GPIO_2_REGS, but, GPIO_INTC_TYPE_RISE_EDGE);

    /* Inicia configuração da pinagem do LCD */ 
    init_PIN_LCD();

    /* Inicia o LCD */
    init_LCD();

    /* Deslocamento para a direita no LCD */
    shift_right();

    WelcomeMessage();

while(1){

    char mens1[] = "Reprovou ";
    char mens2[] = "Calculo?";

    int x = 0;          /* Variável auxiliar para contagem */
    int y = 0;          /* Variável auxiliar para contagem */
    int i = 0;          /* Variável auxiliar para contagem */
    int aux = 0;        /* Variável auxiliar para contagem */
    int j = 0;          /* Variável para contagem do tamanho do num1 */
    int z = 0;          /* Variável para contagem do tamanho do num2 */
    int nL = 0;         /* Variável para o número de Linhas do teclado */
    int nC = 0;         /* Variável para o número de Colunas do teclado */
    int num1 = 0;       /* Variável que armazena o 1º número */
    int num2 = 0;       /* Variável que armazena o 2º número */
    int sin_op = 0;     /* Variável que indica o sinal da operação escolhida */
    int flagC = 0;      /* Flag de controle C */
    int flagD = 0;      /* Flag de controle D */
    int flagE = 0;
    int flagF = 0;
    int n_neg = 0;      /* Flag de verificação de número negativo para num1; */
    int n_neg2 = 0;     /* Flag de verificação de número negativo para num2; */
    char vet1[5];       /* Vetor de armazenamento do num1 */
    char vet2[5];       /* Vetor de armazenamento do num2 */
    int soma = 0;       /* Variável que armazena a soma */
    int sub = 0;        /* Variável que armazena a subtração */
    int mult = 0;       /* Variável que armazena a multiplicação */
    int div = 0;        /* Variável que armazena a divisão */
    int b_soma = 0;     /* Variável que recebe a soma */
    int b_sub = 0;      /* Variável que recebe a subtração */
    int b_mult = 0;     /* Variável que recebe a multiplicação */
    int b_div = 0;      /* Variável que recebe a divisão */
    int c_sum = 0;      /* Variável que armazena o tamanho da soma */
    int c_sub = 0;      /* Variável que armazena o tamanho da subtração */
    int c_mult = 0;     /* Variável que armazena o tamanho da multiplicação */
    int c_div = 0;      /* Variável que armazena o tamanho da divisão */
    int sig_sum = 0;    /* Variável que informa se a soma é negativa */
    int sig_sub = 0;    /* Variável que informa se a subtração é negativa */
    int sig_mult = 0;   /* Variável que informa se a multiplicação é negativa */
    int sig_div = 0;    /* Variável que informa se a divisão é negativa */
    int w = 0;          /* Variável auxiliar para preencher o vetor do num1 */
    int k = 0;          /* Variável auxiliar para preencher o vetor do num2 */
    int a = 1;          /* Flag auxiliar para obter o 1º número */
    int b = 0;          /* Flag auxiliar para obter o 2º número */
    int allow = 0;      /* Variável auxiliar para casos de divisão por zero */
    int temp = 0;
    int cont_neg = 0;
    int cont_neg2 = 0;

        /* Inicia configuração da pinagem do LCD */ 
        init_PIN_LCD();

        /* Inicia o LCD */
        init_LCD();

        /* Deslocamento para a direita no LCD */
        shift_right();

        /* Selecting GPIO pin for use. */

        for(i = 0; i <= 3; i++){
            GPIOPinMuxSetup(mod, pinosLinhas[i]);
            GPIOPinMuxSetup(mod, pinosColunas[i]);
        }

        /* Varrendo as linhas */

        for(nL = 0; nL <= 3; nL++){
            /* Setting the GPIO pin as an output pin. */
            GPIODirModeSet(modAddr, pinosLinhas[nL], GPIO_DIR_OUTPUT);
            /* Driving a logic HIGH on the GPIO pin. */
            GPIOPinWrite(modAddr, pinosLinhas[nL], PIN_LOW);
        }

        /* Varrendo as colunas */

        for(nC = 0; nC <= 3; nC++){
            /* Setting the GPIO pin as an input pin. */
            GPIODirModeSet(modAddr, pinosColunas[nC], GPIO_DIR_INPUT);
        } 

    while(1){

        if(flagIsr){

            ConsoleUtilsPrintf("\n\n\r#####  exit system  ##### \n\n\n");

            delete_LCD();

            Farewell_Message();

            LCD_off();
            
            flagIsr = 0;
            Delay(900);

            /* Cold Reset */
            HWREG(SOC_PRM_DEVICE_REGS) |= 0x2;
        }

        
        for(nL = 0; nL <= 3; nL++){

            GPIOPinWrite(modAddr, pinosLinhas[nL], PIN_HIGH);
          
            
            for(nC = 0; nC <= 3; nC++){

                if(GPIOPinRead(SOC_GPIO_1_REGS, pinosColunas[nC])){

                    //ConsoleUtilsPrintf("Linha %d, Coluna %d \n", nL+1, nC+1);
                    //ConsoleUtilsPrintf("Tecla %c pressionada. \n", teclas[nL][nC]);

                    /* Buzzer ativado por 150 ms */
                    GPIOPinWrite(modAddr, pin, PIN_HIGH);
                    Delay(150);
                    GPIOPinWrite(modAddr, pin, PIN_LOW);

                        if(b == 1){
                            //ConsoleUtilsPrintf("Boi Boi\n");
                            if(teclas[nL][nC] != 'A' && teclas[nL][nC] != 'B' && teclas[nL][nC] != 'C'
                            && teclas[nL][nC] != 'D' && teclas[nL][nC] != '*' && teclas[nL][nC] != '#'){
                                if(w <= 3){
                                    set_RS();
                                    send_data(teclas[nL][nC]);
                                    Delay(1);
                                    vet2[w] = teclas[nL][nC];
                                    w++;
                                    flagD++;
                                }else{

                                    delete_LCD();
                                    Warn_Lenght();

                                    x = 0;
                                    y = 0;
                                    
                                        while(vet2[x+1] != '\0'){
                                            vet2[x] = 0;
                                            x++;
                                        }
                                        
                                            while(vet1[y] != '\0'){
                                                vet1[y] = 0;
                                                y++;
                                            }

                                        flagD = 0;
                                        flagF = 1;
                                        w = 0; 
                                        b = 0; 
                                        a = 1; 
                                        k = 0;

                                }
                            }else{
                              
                                if(teclas[nL][nC] == '*'){
                                    if(n_neg2 == 1){
                                        n_neg2 = 0;
                                    }
                                        delete_LCD();
                                        ConsoleUtilsPrintf("Ei boi\n");

                                        
                                        while(vet2[x] != '\0'){
                                            vet2[x] = 0;
                                            x++;
                                        }
                                           
                                            while(vet1[y] != '\0'){
                                                vet1[y] = 0;
                                                y++;
                                            }

                                        x = 0; /* Zera contador x */
                                        y = 0; /* Zera contador y */
                                        w = 0; 
                                        b = 0; 
                                        a = 1; 
                                        flagD = 0; 
                                        cont_neg2 = 0;
                                        temp = 0;
                                    
                                }else{
                                    if(flagD != 0){
                                        if(teclas[nL][nC] == '#'){
                                            set_RS();
                                            send_data('=');
                                            Delay(1);
                                            vet2[w+1] = '\0';
                                            a = 0;
                                            b = 0;
                                        }
                                            if(teclas[nL][nC] == 'A'){
                                                ConsoleUtilsPrintf("A press. Type # or *.\n\n");
                                                a = 0;
                                                b = 1;
                                                temp = 1;
                                            }
                                                if(teclas[nL][nC] == 'B'){
                                                    ConsoleUtilsPrintf("B press. Type # or *.\n\n");
                                                    a = 0;
                                                    b = 1;
                                                    temp = 1;
                                                }
                                                    if(teclas[nL][nC] == 'C'){
                                                        ConsoleUtilsPrintf("C press. Type # or *.\n\n");
                                                        a = 0;
                                                        b = 1;
                                                        temp = 1;
                                                    }
                                                        if(teclas[nL][nC] == 'D'){
                                                            ConsoleUtilsPrintf("D press. Type # or *.\n\n");
                                                            a = 0;
                                                            b = 1;
                                                            temp = 1;
                                                        }
                                        //ConsoleUtilsPrintf("Numero 2: %s\n", vet2);
                                        //ConsoleUtilsPrintf("Boi\n");
                                        //b = 0;
                                    }else{
                                        if(teclas[nL][nC] == 'B'){
                                            if(cont_neg2 != 1){
                                                ConsoleUtilsPrintf("Boi Boi\n");
                                                ConsoleUtilsPrintf("Numero negativo\n");
                                                set_RS();
                                                send_data('-');
                                                Delay(1);
                                                n_neg2 = 1;
                                                cont_neg2 = 1;
                                            }
                                        }

                                        else if(teclas[nL][nC] == 'A'){
                                                ConsoleUtilsPrintf("A press. Type a valid operation! \n\n");
                                        }
                                            else if(teclas[nL][nC] == 'C'){
                                                ConsoleUtilsPrintf("C press. Type a valid operation! \n\n");
                                            }
                                                else if(teclas[nL][nC] == 'D'){
                                                    ConsoleUtilsPrintf("D press. Type a valid operation! \n\n");
                                                }
                                                    else if(teclas[nL][nC] == '#'){
                                                        ConsoleUtilsPrintf("# press. Type a valid operation! \n\n");
                                                    }
                                    }
                                }
                            }
                        }

                        if(a == 1){
                            if((teclas[nL][nC] == 'C' && flagE == 0) || (teclas[nL][nC] == 'D' && flagE == 0)
                            || (teclas[nL][nC] == '#' && flagE == 0) || (teclas[nL][nC] == '*' && flagE == 0)){
                                //delete_LCD();
                                error_Message();
                                ConsoleUtilsPrintf("Ei boi.\n");
                                //flagE = 1;
                            }else{
                                if(teclas[nL][nC] != 'A' && teclas[nL][nC] != 'B' && teclas[nL][nC] != 'C'
                                && teclas[nL][nC] != 'D' && teclas[nL][nC] != '*' && teclas[nL][nC] != '#'){
                                    
                                    if(k <= 3){
                                        if(flagF == 0){
                                            set_RS();
                                            send_data(teclas[nL][nC]);
                                            Delay(1);
                                            vet1[k] = teclas[nL][nC];
                                            k++;
                                            flagC++;
                                            flagE++;
                                        }else{
                                            flagF = 0;
                                        }
                                    }else{

                                        Warn_Lenght();
                                        delete_LCD();
                                        y = 0;

                                        while(vet1[y] != '\0'){
                                            vet1[y] = 0;
                                            y++;
                                        }

                                        k = 0;
                                        flagC = 0;
                                    }
                                }else{
                                    ConsoleUtilsPrintf(" EEEE Boi\n");
                                    if(teclas[nL][nC] == '*'){
                                   
                                        if(n_neg == 1){
                                            n_neg = 0;
                                        }
                                            delete_LCD();
                                            ConsoleUtilsPrintf("Ei boi 2\n");
                                            
                                            while(vet1[y] != '\0'){
                                                vet1[y] = 0;
                                                y++;
                                            }
                                            y = 0; /* Zera contador y */
                                            k = 0; /* Zera contador k para iniciar o novo numero */
                                            b = 0; /* Não permite pegar um 2º número */
                                            a = 1; /* Permite pegar o 1º número novamente */
                                            flagC = 0; /* Zera flag de sinal negativo */
                                            cont_neg = 0;
                                            flagE++;
                                    }else{
                                        if(flagC != 0){
                                            vet1[k+1] = '\0';
                                            //ConsoleUtilsPrintf("Numero 1: %s\n", vet1);
                                            //ConsoleUtilsPrintf("Boi\n");
                                            if(teclas[nL][nC] == 'A' && temp == 0){
                                                sin_op = 1;
                                                flagE++;
                                                set_RS();
                                                send_data('+');
                                                Delay(1);
                                                a = 0;
                                                b = 1;
                                            }
                                                else if(teclas[nL][nC] == 'B' && temp == 0){
                                                    sin_op = 2;
                                                    flagE++;
                                                    set_RS();
                                                    send_data('-');
                                                    Delay(1);
                                                    a = 0;
                                                    b = 1;
                                                }
                                                    else if(teclas[nL][nC] == 'C' && temp == 0){
                                                        sin_op = 3;
                                                        flagE++;
                                                        set_RS();
                                                        send_data('*');
                                                        Delay(1);
                                                        a = 0;
                                                        b = 1;
                                                    }
                                                        else if(teclas[nL][nC] == 'D' && temp == 0){
                                                            sin_op = 4;
                                                            flagE++;
                                                            set_RS();
                                                            send_data('/');
                                                            Delay(1);
                                                            a = 0;
                                                            b = 1;
                                                        }
                                                            else if(teclas[nL][nC] == '#' && temp == 0){
                                                                ConsoleUtilsPrintf("# press. Type a valid operation! \n\n");
                                                                a = 1;
                                                                b = 0;
                                                            }
                                        }else{
                                            if(cont_neg != 1){
                                                n_neg = 1;
                                                flagE++;
                                                ConsoleUtilsPrintf("Boi Boi Boi Boi\n");
                                                ConsoleUtilsPrintf("Numero negativo\n");
                                                set_RS();
                                                send_data('-');
                                                Delay(1);
                                                cont_neg = 1;
                                            }
                                        }
                                    }
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

    soma = num1 + num2;
    sub = num1 - num2;
    mult = num1 * num2;
    div = num1 / num2;
    
    b_soma = soma;
    b_sub = sub;
    b_mult = mult;
    b_div = div;

    if(soma < 0){
        sig_sum = 1;
        b_soma *= (-1);
    }

    if(sub < 0){
        sig_sub = 1;
        b_sub *= (-1);
    }

    if(mult < 0){
        sig_mult = 1;
        b_mult *= (-1);
    }

    if(div < 0){
        sig_div = 1;
        b_div *= (-1);
    }


    if(sin_op == 1){
        if(sig_sum == 1){
            if(b_soma == 0){
                set_RS();
                send_data(0x30);
                Delay(1);
            }else{
                set_RS();
                send_data('-');
                Delay(1);
                ConsoleUtilsPrintf("Soma: %d\n", num1 + num2);
                c_sum = count_op(b_soma);
                intToAscii(b_soma, c_sum);
            }
        }else{
            if(b_soma == 0){
                set_RS();
                send_data(0x30);
                Delay(1);
            }else{
                ConsoleUtilsPrintf("Soma: %d\n", num1 + num2);
                c_sum = count_op(b_soma);
                intToAscii(b_soma, c_sum);
            }
        }
    }

    if(sin_op == 2){
        if(sig_sub == 1){
            if(b_sub == 0){
                set_RS();
                send_data(0x30);
                Delay(1);
            }else{
                set_RS();
                send_data('-');
                Delay(1);
                ConsoleUtilsPrintf("Subtracao: %d\n", num1 - num2);
                c_sub = count_op(b_sub);
                intToAscii(b_sub, c_sub);
            }
        }else{
            if(b_sub == 0){
                set_RS();
                send_data(0x30);
                Delay(1);
            }else{
                ConsoleUtilsPrintf("Subtracao: %d\n", num1 - num2);
                c_sub = count_op(b_sub);
                intToAscii(b_sub, c_sub);
            }
        }
    }   

    if(sin_op == 3){
        if(sig_mult == 1){
            if(b_mult == 0){
                set_RS();
                send_data(0x30);
                Delay(1);
            }else{
                set_RS();
                send_data('-');
                Delay(1);
                ConsoleUtilsPrintf("Multiplicacao: %d\n", num1 * num2);
                c_mult = count_op(b_mult);
                intToAscii(b_mult, c_mult);
            }
        }else{
            if(b_mult == 0){
                set_RS();
                send_data(0x30);
                Delay(1);
            }else{
                ConsoleUtilsPrintf("Multiplicacao: %d\n", num1 * num2);
                c_mult = count_op(b_mult);
                intToAscii(b_mult, c_mult);
            }
        } 
    }

    if(sin_op == 4){
        if(num2 == 0){

            delete_LCD();

            shift_right();
            shift_right();
            shift_right();

            setLine(0,1);
            
            while(mens1[aux] != '\0'){
                set_RS();
                send_data(mens1[aux]);
                Delay(1);
                aux++;
            }

            aux = 0;

            setLine(0,2);

            while(mens2[aux] != '\0'){
                set_RS();
                send_data(mens2[aux]);
                Delay(1);
                aux++;
            }

            allow = 1;
        }
            if(allow == 0){
                if(sig_div == 1){
                    if(b_div == 0){
                        set_RS();
                        send_data(0x30);
                        Delay(1);
                    }else{
                        set_RS();
                        send_data('-');
                        Delay(1);
                        ConsoleUtilsPrintf("Divisao: %d\n", num1 / num2);
                        c_div = count_op(b_div);
                        intToAscii(b_div, c_div);
                    }
                }else{
                    if(b_div == 0){
                        set_RS();
                        send_data(0x30);
                        Delay(1);
                    }else{
                        ConsoleUtilsPrintf("Divisao: %d\n", num1 / num2);
                        c_div = count_op(b_div);
                        intToAscii(b_div, c_div);
                    }
                }     
            }
    }


    y = 0;

    while(vet1[y] != '\0'){
        vet1[y] = 0;
        y++;
    }

    x = 0;

    while(vet2[x] != '\0'){
        vet2[x] = 0;
        x++;
    }

    Delay(3000);
    delete_LCD();

}    

    ConsoleUtilsPrintf("#####  exit system  #####\n");

    return(0);
}

/*FUNCTION*-------------------------------------------------------
*  Enabling functional clocks for GPIO instance. 
*END*-----------------------------------------------------------*/
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
* A function which is used to initialize GPIO like LED.
*END*-----------------------------------------------------------*/
static void initLed(unsigned int baseAdd, unsigned int module, unsigned int pin){
    
        /* Selecting GPIO pin for use. */
        GPIOPinMuxSetup(module, pin);
    
        /* Setting the GPIO pin as an output pin. */
        GPIODirModeSet(baseAdd, pin, GPIO_DIR_OUTPUT);
}

/*FUNCTION*-------------------------------------------------------
*
* A function which is used to initialize GPIO like BUTTON.
*END*-----------------------------------------------------------*/
static void initButton(unsigned int baseAdd, unsigned int module, unsigned int pin){
    
        /* Selecting GPIO pin for use. */
        GPIOPinMuxSetup(module, pin);
    
        /* Setting the GPIO pin as an output pin. */
        GPIODirModeSet(baseAdd, pin, GPIO_DIR_INPUT);
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

/*FUNCTION*-------------------------------------------------------
*
* Function Name : gpioAintcconfigure
* Comments      : Do the necessary gpio configurations on to AINTC.
*END*-----------------------------------------------------------*/
static void gpioAintcConf(void){

    /* Initialize the ARM interrupt control */
    IntAINTCInit();
 
    /* Registering gpioIsr */
    IntRegister(SYS_INT_GPIOINT2A, gpioIsr);
    
    /* Set the priority */
    IntPrioritySet(SYS_INT_GPIOINT2A, 0, AINTC_HOSTINT_ROUTE_IRQ);
    
    /* Enable the system interrupt */
    IntSystemEnable(SYS_INT_GPIOINT2A);
   
}

/*FUNCTION*-------------------------------------------------------
*
* Function Name : gpioIsr
* Comments      : DMTimer interrupt service routine. This will 
* send a character to serial.
*END*-----------------------------------------------------------*/   
static void gpioIsr(void) {
    flagIsr = 1;    
        /*  Clear wake interrupt    */
    //HWREG(SOC_GPIO_1_REGS + 0x3C) = 0x1000;
    //HWREG(SOC_GPIO_1_REGS + 0x34) = 0x1000;
    //HWREG(SOC_GPIO_1_REGS + 0x2C) = 0x1000;
    HWREG(SOC_GPIO_2_REGS + 0x2C) = 0xFFFFFFFF;
}

/*FUNCTION*-------------------------------------------------------
*
* Function Name : gpioPinIntConfig
* Comments      :
*END*-----------------------------------------------------------*/
static int gpioPinIntConf(unsigned int baseAdd, unsigned int intLine,
                  unsigned int pinNumber){

        /* Setting interrupt GPIO pin. */
        gpioPinIntEnable(baseAdd,
               intLine,
               pinNumber);
    
        return(0);
}

/*FUNCTION*-------------------------------------------------------
*
* Function Name : GPIOPinIntEnable
* Comments      : This API enables the configured interrupt event on a specified input
* GPIO pin to trigger an interrupt request.
*
* \param  baseAdd     The memory address of the GPIO instance being used
* \param  intLine     This specifies the interrupt request line on which the
*                     interrupt request due to the transitions on a specified
*                     pin be propagated
* \param  pinNumber   The number of the pin in the GPIO instance
*
* 'intLine' can take one of the following two values:
* - GPIO_INT_LINE_1 - interrupt request be propagated over interrupt line 1\n
* - GPIO_INT_LINE_2 - interrupt request be propagated over interrupt line 2\n
* 
* 'pinNumber' can take one of the following values:
* (0 <= pinNumber <= 31)\n
*
* \return None
*
*END*-----------------------------------------------------------*/
static void gpioPinIntEnable(unsigned int baseAdd,
                      unsigned int intLine,
                      unsigned int pinNumber){
    if(GPIO_INTC_LINE_1 == intLine){
        HWREG(baseAdd + GPIO_IRQSTATUS_SET(0)) = (1 << pinNumber);
    }else{
        HWREG(baseAdd + GPIO_IRQSTATUS_SET(1)) = (1 << pinNumber);
    }
}

/*FUNCTION*-------------------------------------------------------
*
* Function Name : gpioAintcconfigure
* Comments      : This API configures the event type for a specified 
* input GPIO pin. Whenever the selected event occurs on that GPIO pin 
* and if interrupt generation is enabled for that pin, the GPIO module 
* will send an interrupt to CPU.
*
* \param  baseAdd    The memory address of the GPIO instance being used
* \param  pinNumber  The number of the pin in the GPIO instance
* \param  eventType  This specifies the event type on whose detection,
*                    the GPIO module will send an interrupt to CPU,
*                    provided interrupt generation for that pin is enabled.
*
* 'pinNumber' can take one of the following values:
* (0 <= pinNumber <= 31)\n
*
* 'eventType' can take one of the following values:
* - GPIO_INT_TYPE_NO_LEVEL - no interrupt request on occurence of either a
*   logic LOW or a logic HIGH on the input GPIO pin\n
* - GPIO_INT_TYPE_LEVEL_LOW - interrupt request on occurence of a LOW level
*   (logic 0) on the input GPIO pin\n
* - GPIO_INT_TYPE_LEVEL_HIGH - interrupt request on occurence of a HIGH level
*   (logic 1) on the input GPIO pin\n
* - GPIO_INT_TYPE_BOTH_LEVEL - interrupt request on the occurence of both the
*   LOW level and HIGH level on the input GPIO pin\n
* - GPIO_INT_TYPE_NO_EDGE -  no interrupt request on either rising or
*   falling edges on the pin\n
* - GPIO_INT_TYPE_RISE_EDGE - interrupt request on occurence of a rising edge
*   on the input GPIO pin\n
* - GPIO_INT_TYPE_FALL_EDGE - interrupt request on occurence of a falling edge
*   on the input GPIO pin\n
* - GPIO_INT_TYPE_BOTH_EDGE - interrupt request on occurence of both a rising
*   and a falling edge on the pin\n
*
* \return  None
*
* \note  A typical use case of this API is explained below:
* 
*        If it is initially required that interrupt should be generated on a
*        LOW level only, then this API can be called with
*        'GPIO_INT_TYPE_LEVEL_LOW' as the parameter.
*        At a later point of time, if logic HIGH level only should be made as
*        the interrupt generating event, then this API has to be first called
*        with 'GPIO_INT_TYPE_NO_LEVEL' as the parameter and later with
*        'GPIO_INT_TYPE_LEVEL_HIGH' as the parameter. Doing this ensures that
*        logic LOW level trigger for interrupts is disabled.
*END*-----------------------------------------------------------*/
static void gpioIntTypeSet(unsigned int baseAdd,
                    unsigned int pinNumber,
                    unsigned int eventType){
    eventType &= 0xFF;

    switch(eventType)
    {

        case GPIO_INT_TYPE_NO_LEVEL:

            /* Disabling logic LOW level detect interrupt generation. */
            HWREG(baseAdd + GPIO_LEVELDETECT(0)) &= ~(1 << pinNumber);

            /* Disabling logic HIGH level detect interrupt generation. */
            HWREG(baseAdd + GPIO_LEVELDETECT(1)) &= ~(1 << pinNumber);

        break;

        case GPIO_INT_TYPE_LEVEL_LOW:

            /* Enabling logic LOW level detect interrupt geenration. */
            HWREG(baseAdd + GPIO_LEVELDETECT(0)) |= (1 << pinNumber);

            /* Disabling logic HIGH level detect interrupt generation. */
            HWREG(baseAdd + GPIO_LEVELDETECT(1)) &= ~(1 << pinNumber);

            /* Disabling rising edge detect interrupt generation. */
            HWREG(baseAdd + GPIO_RISINGDETECT) &= ~(1 << pinNumber);

            /* Disabling falling edge detect interrupt generation. */
            HWREG(baseAdd + GPIO_FALLINGDETECT) &= ~(1 << pinNumber);

        break;

        case GPIO_INT_TYPE_LEVEL_HIGH:

            /* Disabling logic LOW level detect interrupt generation. */
            HWREG(baseAdd + GPIO_LEVELDETECT(0)) &= ~(1 << pinNumber);

            /* Enabling logic HIGH level detect interrupt generation. */
            HWREG(baseAdd + GPIO_LEVELDETECT(1)) |= (1 << pinNumber);

            /* Disabling rising edge detect interrupt generation. */
            HWREG(baseAdd + GPIO_RISINGDETECT) &= ~(1 << pinNumber);

            /* Disabling falling edge detect interrupt generation. */
            HWREG(baseAdd + GPIO_FALLINGDETECT) &= ~(1 << pinNumber);
        
        break;

        case GPIO_INT_TYPE_BOTH_LEVEL:
            
            /* Enabling logic LOW level detect interrupt geenration. */
            HWREG(baseAdd + GPIO_LEVELDETECT(0)) |= (1 << pinNumber);

            /* Enabling logic HIGH level detect interrupt generation. */
            HWREG(baseAdd + GPIO_LEVELDETECT(1)) |= (1 << pinNumber);

            /* Disabling rising edge detect interrupt generation. */
            HWREG(baseAdd + GPIO_RISINGDETECT) &= ~(1 << pinNumber);

            /* Disabling falling edge detect interrupt generation. */
            HWREG(baseAdd + GPIO_FALLINGDETECT) &= ~(1 << pinNumber);
            
        break;

        case GPIO_INT_TYPE_NO_EDGE:
            
            /* Disabling rising edge detect interrupt generation. */
            HWREG(baseAdd + GPIO_RISINGDETECT) &= ~(1 << pinNumber);

            /* Disabling falling edge detect interrupt generation. */
            HWREG(baseAdd + GPIO_FALLINGDETECT) &= ~(1 << pinNumber);

        break;

        case GPIO_INT_TYPE_RISE_EDGE:

            /* Enabling rising edge detect interrupt generation. */
            HWREG(baseAdd + GPIO_RISINGDETECT) |= (1 << pinNumber);

            /* Disabling falling edge detect interrupt generation. */
            HWREG(baseAdd + GPIO_FALLINGDETECT) &= ~(1 << pinNumber);

            /* Disabling logic LOW level detect interrupt generation. */
            HWREG(baseAdd + GPIO_LEVELDETECT(0)) &= ~(1 << pinNumber);

            /* Disabling logic HIGH level detect interrupt generation. */
            HWREG(baseAdd + GPIO_LEVELDETECT(1)) &= ~(1 << pinNumber);

        break;

        case GPIO_INT_TYPE_FALL_EDGE:

            /* Disabling rising edge detect interrupt generation. */
            HWREG(baseAdd + GPIO_RISINGDETECT) &= ~(1 << pinNumber);

            /* Enabling falling edge detect interrupt generation. */
            HWREG(baseAdd + GPIO_FALLINGDETECT) |= (1 << pinNumber);

            /* Disabling logic LOW level detect interrupt generation. */
            HWREG(baseAdd + GPIO_LEVELDETECT(0)) &= ~(1 << pinNumber);

            /* Disabling logic HIGH level detect interrupt generation. */
            HWREG(baseAdd + GPIO_LEVELDETECT(1)) &= ~(1 << pinNumber);

        break;

        case GPIO_INT_TYPE_BOTH_EDGE:

            /* Enabling rising edge detect interrupt generation. */
            HWREG(baseAdd + GPIO_RISINGDETECT) |= (1 << pinNumber);

            /* Enabling falling edge detect interrupt generation. */
            HWREG(baseAdd + GPIO_FALLINGDETECT) |= (1 << pinNumber);

            /* Disabling logic LOW level detect interrupt generation. */
            HWREG(baseAdd + GPIO_LEVELDETECT(0)) &= ~(1 << pinNumber);

            /* Disabling logic HIGH level detect interrupt generation. */
            HWREG(baseAdd + GPIO_LEVELDETECT(1)) &= ~(1 << pinNumber);

        break;

        default:
        break;
    }
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
//printf("%s\n", aux);

i = 0;

while(aux[i] != '\0'){
    set_RS();
    send_data(aux[i]);
    Delay(1);
    i++;
}

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
