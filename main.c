//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC: TM4C123GH6PM
// System Clock: 40 MHz

// Address 0x0000 - 0x0004 are reserved for the user set time and date

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "tm4c123gh6pm.h"
#include "clock.h"
#include "uart0.h"
#include "gpio.h"
#include "hibernation.h"
#include "wait.h"
#include "i2c0.h"
#include "string.h"
#include "time.h"
#include "adc0.h"
#include "utility.h"
#include "command.h"
#include "log.h"
#include "speaker.h"

// PortB masks for I2C
#define SDA_MASK 8
#define SCL_MASK 4

// Hibernate values
#define HIBWAKE     0x000153 // 0001 0101 0011 .. NO RTC MATCH
#define HIBTIME     0x00014B // 0001 0100 1011 .. NO WAKE PIN

// Pins
#define LIGHT        PORTA,4 // 1 when light is on and 0 when light is off
#define LEVELSELECT  PORTA,7
#define I2C0SCL      PORTB,2
#define I2C0SDA      PORTB,3
#define RED_LED      PORTF,1
#define PB1         PORTF,4

// HIB_DATA_R
#define HIBMSAMP    (*((volatile uint32_t *)(0x400FC030 + (1*4))))  // max samples
#define HIBCSAMP    (*((volatile uint32_t *)(0x400FC030 + (10*4)))) // sample count
#define HIBRUN      (*((volatile uint32_t *)(0x400FC030 + (11*4)))) // if run is on (1 if on, 0 if off)
//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------


// Initialize Hardware
void initHw()
{
     //Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();//change to 400 kHz

    initI2c0();//initialize I2C
    initUart0();
    initAdc0Ss3();//for temperature
    initspeakerHw();

    enablePort(PORTA);// Enables port A sets LevelSelect as an output
    selectPinPushPullOutput(LEVELSELECT);
    setPinValue(LEVELSELECT, 1);
    selectPinDigitalInput(LIGHT);

    // Set up on board LEDs
    enablePort(PORTF);
    selectPinDigitalInput(PORTF, 4);
    enablePinPullup(PORTF, 4);
    selectPinPushPullOutput(RED_LED);

    waitMicrosecond(5); // Let the leveling circuit completely turn on
}

int main(void)
{
    initHw();//initialize hardware

    setPinValue(RED_LED, 1);
    waitMicrosecond(1000000);
    setPinValue(RED_LED, 0);

    if(!checkIfConfigured()) // Only need to run the commands the first time through this code
    {
        initHibernationModule();

        // Initalize values to make commands() run properly
        HIBMSAMP = 10;
        while(!(HIB_CTL_R & HIB_CTL_WRC));
        HIBCSAMP = 0;
        while(!(HIB_CTL_R & HIB_CTL_WRC));
        HIBRUN = 0;
        while(!(HIB_CTL_R & HIB_CTL_WRC));
    }

    startTime();
    commands();
    stopHibernation();

    while(true);

}

// Example of Git hub changing stuff
