//Abiria Placide
//axp1223 lab7 and lab9 project

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "tm4c123gh6pm.h"
#include "lab5_AbiriaPlacide.h"
#include "lab6_AbiriaPlacide.h"
#include "eeprom.h"
#include "uart0.h"
#include "wait.h"


//ALL HASH defines

// PortB masks
#define RED_BL_LED_MASK 32

// PortE masks
#define BLUE_BL_LED_MASK 16
#define GREEN_BL_LED_MASK 32

#define PB4_MASK (1 << 6)
// PortE masks

#define BLUE_BL_LED_MASK 16
#define GREEN_BL_LED_MASK 32

//LEDS DEBUGGING
#define DEBUG

#define GREEN_LED_MASK 8
#define BLUE_LED_MASK  4
#define RED_LED_MASK  2

#define RED_LED      (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 +  1*4)))
#define BLUE_LED      (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 2*4)))
#define GREEN_LED     (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))

#define LOGICAL_BASE_PULSE  22500  //at 40 Mhz where time  = 526.5 us
#define LOGICAL_1_PULSE_OFF 67500 // at 40 Mhz where time  = 1.6875ms
#define LOGICAL_1_PULSE_ON  LOGICAL_BASE_PULSE
#define LOGICAL_0_PULSE_ON  LOGICAL_BASE_PULSE
#define LOGICAL_0_PULSE_OFF LOGICAL_BASE_PULSE
#define END_TRANSMISSION    LOGICAL_BASE_PULSE
#define NINE_millisec 360000
#define FOUR_FIVE_millisec 180000//4.5 ms

//prototypes
void configure_timer2(uint32_t freq, bool period);
void IRQ_Timer2_ISR();
void load_byte(uint8_t byte);

//global flag variables and commands

uint8_t Commands[] = {162,98,226,34,2,194,224,168,144,104,152,176,48,24,122,16,56,90,66,74,82}; //Button Data from IR Remote controller
bool pulse_status  = 0; //use to check if pwm on PB 5 is driving IR LED (IE, ON or OFF)
bool nine_ms_ready = 0;
bool four_ms_ready = 0;
volatile bool BOOL_INTERRUPT_CLEAR = 0;
uint8_t numberOfCommands = 0; //keeps track of the number of commands
uint8_t newCommandPosition = 1; //keeps track of next address store new command


//data_bits_copy is an array shared between lab7 and lab6. copy happens between in lab6
uint8_t binaryToDecimal(uint8_t databitsCopy[32], uint8_t range0, uint8_t range1) // range0-range1 has to be lsb-msb eg. 16-23
{

    uint8_t sum = 0; // final value will be binarytoDecimal conversion
    uint8_t exponential = 0;
    uint8_t i;

    for (i=range1; i >=range0; i--)
    {
        //convert from binary array to decimal
        if(databitsCopy[i] == 1)
        {
            sum += (1 << exponential);
        }
        exponential+=1;
    }
    return sum;
}

void printByte(uint32_t eeread)
{
    char ptr = eeread;
    int8_t i = 7;
    for(i=7; i >= 0; i--)
     {
         if( ptr & (1 << i) )
         {
             putcUart0('1');
         }
         else
         {
             putcUart0('0');
         }
     }
}


// Initialize PWM0 gen 1 on PB5
void initPWM()
{
    // Configure HW to work with 16 MHz XTAL, PLL enabled, system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);
    // Set GPIO ports to use APB (not needed since default configuration -- for clarity)
    SYSCTL_GPIOHBCTL_R = 0;
    // Enable clocks
    SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R0;
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1 | SYSCTL_RCGCGPIO_R4 |SYSCTL_RCGCGPIO_R5; //enable clock for PORT B & E & F
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R2; //enable clock for timer2
    _delay_cycles(3);

    // Configure three backlight LEDs
    GPIO_PORTB_DIR_R   |= RED_BL_LED_MASK;                      // make bit5 of PORTB a PWM output
    GPIO_PORTB_DR2R_R  |= RED_BL_LED_MASK;                      // set drive strength to 2mA
    GPIO_PORTB_DEN_R   |= RED_BL_LED_MASK;                       // enable digital
    GPIO_PORTB_AFSEL_R |= RED_BL_LED_MASK;                     // select auxilary function
    GPIO_PORTB_PCTL_R  &= GPIO_PCTL_PB5_M;                      // = 0x00F00000 enable PWM
    GPIO_PORTB_PCTL_R  |= GPIO_PCTL_PB5_M0PWM3; //0x00400000

    GPIO_PORTE_DIR_R |= GREEN_BL_LED_MASK | BLUE_BL_LED_MASK;  // make bits 4 and 5 outputs
    GPIO_PORTE_DR2R_R |= GREEN_BL_LED_MASK | BLUE_BL_LED_MASK; // set drive strength to 2mA
    GPIO_PORTE_DEN_R |= GREEN_BL_LED_MASK | BLUE_BL_LED_MASK;  // enable digital
    GPIO_PORTE_AFSEL_R |= GREEN_BL_LED_MASK | BLUE_BL_LED_MASK;// select auxilary function
    GPIO_PORTE_PCTL_R &= GPIO_PCTL_PE4_M | GPIO_PCTL_PE5_M;    // enable PWM
    GPIO_PORTE_PCTL_R |= GPIO_PCTL_PE4_M0PWM4 | GPIO_PCTL_PE5_M0PWM5;

#ifdef DEBUG
    // Configure LED and pushbutton pins
    GPIO_PORTF_DIR_R |= GREEN_LED_MASK | BLUE_LED_MASK | RED_LED_MASK;
    GPIO_PORTF_DEN_R |= GREEN_LED_MASK | BLUE_LED_MASK | RED_LED_MASK;

#endif

    // RED   on M0PWM3 (PB5), M0PWM1b

    SYSCTL_SRPWM_R = SYSCTL_SRPWM_R0;                // reset PWM0 module
    SYSCTL_SRPWM_R = 0;                              // leave reset state

    PWM0_1_CTL_R = 0;                                // turn-off PWM0 generator 1
    PWM0_2_CTL_R = 0;                                // turn-off PWM0 generator 2


    PWM0_1_GENB_R = PWM_0_GENB_ACTCMPBD_ZERO | PWM_0_GENB_ACTLOAD_ONE; //PB-5


    PWM0_2_GENA_R = PWM_0_GENA_ACTCMPAD_ZERO | PWM_0_GENA_ACTLOAD_ONE; // output 4 on PWM0, gen 2a, cmpa

    PWM0_2_GENB_R = PWM_0_GENB_ACTCMPBD_ZERO | PWM_0_GENB_ACTLOAD_ONE; // output 5 on PWM0, gen 2b, cmpb

    PWM0_1_LOAD_R = 1047;//526  PB5                          // set period to 40 MHz sys clock / 2 / 1024 = 19.53125 kHz

    PWM0_2_LOAD_R = 6000;//526  PE4,E5

    PWM0_INVERT_R = PWM_INVERT_PWM3INV | PWM_INVERT_PWM4INV | PWM_INVERT_PWM5INV;
                                                     // invert outputs so duty cycle increases with increasing compare values

    PWM0_1_CMPB_R = 524; //pb 5                             // red off (0=always low, 1023=always high)

    //good values for speaker: 3000, 100
    PWM0_2_CMPA_R = 3000;   //pe 4                            // blue off
    PWM0_2_CMPB_R = 100;    //pe 5                           // green off


    PWM0_1_CTL_R = PWM_0_CTL_ENABLE;                 // turn-on PWM0 generator 1
    PWM0_2_CTL_R = PWM_0_CTL_ENABLE;                 // turn-on PWM0 generator 1


    PWM0_ENABLE_R = PWM_ENABLE_PWM3EN | PWM_ENABLE_PWM4EN | PWM_ENABLE_PWM5EN;
                                                     // enable outputs

    //initially pw0 output should be zero to allow for first 9ms burst when called from a different function
    GPIO_PORTB_DEN_R   &= ~(RED_BL_LED_MASK);
    GPIO_PORTE_DEN_R   &= ~(GREEN_BL_LED_MASK); //used for alert on | off
    GPIO_PORTE_DEN_R   &= ~(BLUE_BL_LED_MASK); //used for alert on | off

}

void configure_timer2(uint32_t freq, bool period)
{
    TIMER2_CTL_R &= ~TIMER_CTL_TAEN;                 // turn-off timer before reconfiguring
    TIMER2_CFG_R = TIMER_CFG_32_BIT_TIMER;           // configure as 32-bit timer (A+B)
    if (period == true)
    {
        TIMER2_TAMR_R = TIMER_TAMR_TAMR_PERIOD;          // configure for periodic mode (count down)
    }
    else
    {
        TIMER2_TAMR_R = TIMER_TAMR_TAMR_1_SHOT;          // configure for 1-shot mode(count down)
    }

    TIMER2_TAILR_R = freq;                           // set load value to 40e6 for 1 Hz interrupt rate
    TIMER2_IMR_R = TIMER_IMR_TATOIM;                 // turn-on interrupts
    NVIC_EN0_R |= 1 << (INT_TIMER2A-16);             // turn-on interrupt 39 (TIMER2A)
    TIMER2_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer
}

void IRQ_Timer2_ISR()
{
    static uint8_t byte_index = 0;
    static uint8_t bit_shift = 0;
    static bool bit_load = 0;

    GPIO_PORTB_DEN_R   &= ~RED_BL_LED_MASK; //turn off pwm0
    BOOL_INTERRUPT_CLEAR  = 1;
    TIMER2_ICR_R = TIMER_ICR_TATOCINT;               // clear timer 2 interrupt flag for next signal

}

void load_byte(uint8_t byte)
{
    uint8_t i;
    for(i=8 ;i>0;i--)
    {
        //turn on pwm0 then wait for LOGICAL_BASE_PULSE time = 562.5 us
        GPIO_PORTB_DEN_R   |= RED_BL_LED_MASK;
        _delay_cycles(22500); //562.5 us
        GPIO_PORTB_DEN_R   &= ~RED_BL_LED_MASK;  //turn off pwm0

        //while off, either add 562.5us or 1.6875ms
        if(byte & 0x80) //if logical 1, add an additional 1.6875 ms
        {
            //delay for 1.6875ms space = 67500
            _delay_cycles(67500);
        }

        else //if logical 0, add an extra 562,5 us
        {
            //turn off for 562.5 us space
            _delay_cycles(22500);
        }

        byte = byte << 1;
    }
}

void help()
{
    putsUart0("\r\n###Commands###\r\n");
    putsUart0("\r\ndecode - turns on IR receiver\r");
    putsUart0("\r\nlist   - list all commands in eeprom\r");
    putsUart0("\r\nlearn [name] - learn button pressed by remote. only allows 4 chars.\r");
    putsUart0("\r\ninfo  [name] - info on [name] in eeprom. only allows 4 chars.\r");
    putsUart0("\r\nerase [name] - erase [name] from eeprom. only allows 4 chars\r");
    putsUart0("\r\nplay  [name] - transmit [name] to IR receiver. only allows 4 chars\r");
    putsUart0("\r\nalert [good|bad] [on|off] - turns on|off speaker on good|bad command received\r");

}

void playCommand(uint8_t signal_address, uint8_t signal_data)
{
    //transmit signal is on PB5.
    //turn on pwm 9 ms
    GPIO_PORTB_DEN_R |= RED_BL_LED_MASK;
    _delay_cycles(360000);
    GPIO_PORTB_DEN_R &= ~RED_BL_LED_MASK;

    //4.5ms off
    _delay_cycles(180000);

    //send data
    load_byte(signal_address);
    load_byte(~signal_address);
    load_byte(signal_data);
    load_byte(~signal_data);

    //last pulse for end of message
    GPIO_PORTB_DEN_R   |= RED_BL_LED_MASK;
    _delay_cycles(END_TRANSMISSION); //562.5 us
    GPIO_PORTB_DEN_R   &= ~RED_BL_LED_MASK;
    _delay_cycles(1600); //40 us
}

int main()
{
    // Setup UART0 & baud_rate
    initUart0();
    setUart0BaudRate(115200, 40e6);

    //setup M0_PWM0 on PORTB-5 and EEPROM
    initPWM();
    initEeprom();
    help(); //print commands
    putsUart0("\n\nReady...\r\n");
    putcUart0('#');

    USER_DATA data;
    while(true)
    {
         getsUart0(&data); //get data from  tty interface
         putsUart0("\r\n");
         putcUart0('>');
         putsUart0(data.buffer); //output input data

         // Parse fields
         parseFields(&data);
         //copy command

         if(isCommand(&data, "decode", 0))
         {
             //inialize receiver
             initLab6();
         }

         else if (isCommand(&data, "learn", 1))
         {
             putsUart0("\r\n learn command initiated...\r\n");

             //variables to store in eeprom
             uint8_t sum_addr = 0;
             uint8_t sum_data = 0;
             uint8_t valid_bit = 1;

             //get user input command
             char *command = getFieldString(&data, 1);
             char *nullterm = '\0';

             //run lab6 to receive command to store.
             initLab6();

             while(data_ready);//data_ready will change to zero if IR signal is recognized found. so wait...

             //store name, address, and data of IR code in EEPROM. this assumes that each address points to 1 byte of data and does not actually point to 4 bytes as discussed in class.

             int16_t i;
             for(i = 0; command[i] != '\0'; i++)              //store name
             {
                 writeEeprom(newCommandPosition+i, *((uint32_t *)&command[i]));
             }

             writeEeprom(newCommandPosition+i, *((uint32_t *)&nullterm[0])); //add null term

             //get data and addr in integer format.
             sum_data = binaryToDecimal(databitsCopy,16,23);
             sum_addr = binaryToDecimal(databitsCopy,8,15); //for some reason it crashes when its 0-7. so used 8-15 for the address.

             //store address + data + valid_bit
             writeEeprom(newCommandPosition+i+1, *((uint32_t *)&sum_addr)); // write address
             writeEeprom(newCommandPosition+i+2, *((uint32_t *)&sum_data)); //write data
             writeEeprom(newCommandPosition+i+3, *((uint32_t *)&valid_bit)); //write valid bit

             //reset to wait for data at next interval.
             data_ready = 1;//reset to wait until data is ready from lab6 receiver
             writeEeprom(0, *((uint32_t *)&newCommandPosition)); //store number of commands to traverse at addr 0
             numberOfCommands +=1;   //keep track of number of commands
             newCommandPosition +=9; //update where to position new command
         }

         else if(isCommand(&data, "info", 1))
         {
             char * command = getFieldString(&data, 1);
             uint8_t len = strlen(command);

             //search for command then printout address and data
             uint8_t eepromCommandPosition = readEeprom(0); //index of offset of commands
             uint8_t i;
             uint8_t j;


             for(i = 1; i <= eepromCommandPosition; i+=9)
             {
                 uint8_t flag = 0;
                 uint32_t readeeprom = readEeprom(i);
                // char * eepromCommand = (char *)&readeeprom;
                char * eepromCommand = (char *)&readeeprom;


                for(j =0; j < len; j++)
                {
                      char a = command[j];
                      char b = eepromCommand[j];

                      if(a == b)
                      {
                          flag+= 1;
                      }

                      else
                      {
                          flag = 0;
                      }
                }

                if(flag == len)
                {
                    uint8_t valid= readEeprom(i+7);

                    if (valid)
                    {

                    putsUart0("\r\naddr: \r\n");

                    uint8_t addr = readEeprom(i+5); //address
                    printByte(addr);
                    putsUart0("\r\ndata\r\n");
                    uint8_t data = readEeprom(i+6); //data
                    printByte(data);
                    //putsUart0("\r\nvalid:\r\n");
                    //readeeprom = readEeprom(i+7); //valid bit
                    //printByte(readeeprom);
                    putsUart0("\r\n");

                    }

                    else
                    {
                        putsUart0("\r\ninvalid command\r\n");
                    }

                    break;
                }

             }
         }

         else if(isCommand(&data, "list", 0))
         {
             //list all stored commands. it would be great to have a counter keeping track of the number of commands
             putsUart0("\r\n list command initiated...\r\n");

             uint8_t eepromCommandPosition = readEeprom(0);
             uint8_t i;
             for(i = 1; i <= eepromCommandPosition; i+=9)
             {
                 uint8_t valid= readEeprom(i+7);
                 if (valid)
                 {
                     uint32_t readeeprom = readEeprom(i);
                     char * print = (char*)&readeeprom;
                     putsUart0(print);
                     putsUart0("\r\n");
                 }

             }
         }

         else if(isCommand(&data, "erase", 1))
         {
             //copy name from memory until \0. match which input command. if correct, offset = 16. 17bit =0
             putsUart0("\r\n erase command initiated...\r\n");

             char * command = getFieldString(&data, 1);
             uint8_t len = strlen(command);

             //search for command then printout address and data
             uint8_t eepromCommandPosition = readEeprom(0); //index of offset of commands
             uint8_t i;
             uint8_t j;

             for(i = 1; i <= eepromCommandPosition; i+=9)
             {
                 uint8_t flag = 0;
                 uint32_t readeeprom = readEeprom(i);
                // char * eepromCommand = (char *)&readeeprom;
                char * eepromCommand = (char *)&readeeprom;


                for(j =0; j < len; j++)
                {
                      char a = command[j];
                      char b = eepromCommand[j];

                      if(a == b)
                      {
                          flag+= 1;
                      }

                      else
                      {
                          flag = 0;
                      }
                }

                if(flag == len)
                {
                    uint8_t valid_bit = 0;
                    writeEeprom(i+7, *((uint32_t *)&valid_bit)); //write valid bit
                    break;
                }
             }
         }

         else if(isCommand(&data, "play", 1))
         {
             putsUart0("\r\n play command initiated,sending signal\r\n");

             char * command = getFieldString(&data, 1);
             uint8_t len = strlen(command);

             //search for command then printout address and data
             uint8_t eepromCommandPosition = readEeprom(0); //index of offset of commands
             uint8_t i;
             uint8_t j;


             for(i = 1; i <= eepromCommandPosition; i+=9)
             {
                 uint8_t flag = 0;
                 uint32_t readeeprom = readEeprom(i);
                // char * eepromCommand = (char *)&readeeprom;
                char * eepromCommand = (char *)&readeeprom;


                for(j =0; j < len; j++)
                {
                      char a = command[j];
                      char b = eepromCommand[j];

                      if(a == b)
                      {
                          flag+= 1;
                      }

                      else
                      {
                          flag = 0;
                      }
                }

                if(flag == len)
                {
                    uint8_t valid= readEeprom(i+7); //valid offset

                    if (valid)
                    {
                    uint8_t address = readEeprom(i+5); //address offset
                    uint8_t data = readEeprom(i+6); //data offset

                    playCommand(address,data);
                    }

                    else
                    {
                        putsUart0("\r\ninvalid command\r\n");
                    }

                    break;
                }

             }
         }

         else if(isCommand(&data, "alert", 2))
         {
             char * argv1 = getFieldString(&data, 1);
             char * argv2 = getFieldString(&data, 2);

             uint8_t match_good = strcmp(argv1, "good");
             uint8_t match_bad  = strcmp(argv1, "bad");
             if(match_good == 0)
             {
                 match_good = strcmp(argv2, "on");
                 match_bad = strcmp(argv2, "off");
                 if(match_good == 0)
                 {
                     ALERTGOODON = 1;
                 }
                 else if(match_bad == 0)
                 {
                     ALERTGOODON = 0;
                 }

             }

             else if(match_bad == 0)
             {
                 match_good = strcmp(argv2, "on");
                 match_bad = strcmp(argv2, "off");
                 if(match_good == 0)
                 {
                     ALERTBADON = 1;
                 }

                 else if(match_bad == 0)
                 {
                     ALERTBADON = 0;
                 }
             }
         }

         else
         {
             putsUart0("\r\ncommand not found.\r\n");
         }

         putsUart0("#");

    }
}

