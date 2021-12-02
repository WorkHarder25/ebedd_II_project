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
#include "utility.h"
#include "command.h"
#include "log.h"

// PortB masks for I2C
#define SDA_MASK 8
#define SCL_MASK 4

// EEPROM addresses
#define LOG 0x0006 // This is where log will be stored in the EEPROM
#define EEPROM 0xA0 // EEPROM address

// Pins
#define LIGHT        PORTA,4 // 1 when light is on and 0 when light is off
#define LEVELSELECT  PORTA,7
#define I2C0SCL      PORTB,2
#define I2C0SDA      PORTB,3
#define RED_LED      PORTF,1
#define BLUE_LED     PORTF,2
#define GREEN_LED    PORTF,3

#define DAYSEC 86400

// log
#define SLEEP       28 // 10000000

// EERPOM ADDRESSES
#define HIBWAKE     0x000153 // 0001 0101 0011 .. NO RTC MATCH
#define HIBTIME     0x00014B // 0001 0100 1011 .. NO WAKE PIN
#define HIBWAKENOS  0x000053 // 0000 0101 0011 .. NO RTC MATCH
#define HIBTIMENOS  0x00004B // 0000 0100 1011 .. NO WAKE PIN

// HIBDATA offsets
#define HIBLOG  (*((volatile uint32_t *)(0x400FC030 + (0*4)))) // log
#define HIBSAMP (*((volatile uint32_t *)(0x400FC030 + (1*4)))) // N samples
#define HIBGATE (*((volatile uint32_t *)(0x400FC030 + (2*4)))) // gating param
#define HIBLG   (*((volatile uint32_t *)(0x400FC030 + (3*4)))) // gating less than (0) or grater than (1)
#define HIBHYST (*((volatile uint32_t *)(0x400FC030 + (4*4)))) // hyst value
#define HIBMODE (*((volatile uint32_t *)(0x400FC030 + (5*4)))) // 0 trigger - 1 periodic
#define HIBMON  (*((volatile uint32_t *)(0x400FC030 + (6*4)))) // Month
#define HIBDAY  (*((volatile uint32_t *)(0x400FC030 + (7*4)))) // Day
#define HIBHR   (*((volatile uint32_t *)(0x400FC030 + (5*4)))) // Hour
#define HIBMIN  (*((volatile uint32_t *)(0x400FC030 + (5*4)))) // Minute
#define HIBSEC  (*((volatile uint32_t *)(0x400FC030 + (5*4)))) // Second

#define HB(x) (x >> 8) & 0xFF//defines High Byte for reading/writing to EEPROM
#define LB(x) (x) & 0xFF//defines low byte for reading/writing to EEPROM

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

    enablePort(PORTA);// Enables port A sets LevelSelect as an output
    selectPinPushPullOutput(LEVELSELECT);
    setPinValue(LEVELSELECT, 1);
    selectPinDigitalInput(LIGHT);
    //enablePinPullup(LIGHT);

    // Set up on board LEDs
    enablePort(PORTF);
    selectPinDigitalInput(PORTF, 4);
    enablePinPullup(PORTF, 4);
    selectPinPushPullOutput(PORTF, 1);
    selectPinPushPullOutput(PORTF, 2);
    selectPinPushPullOutput(PORTF, 3);

    waitMicrosecond(5); // Let the leveling circuit completely turn on
}

int main(void)
{
    initHw();//initialize hardware

    setPinValue(RED_LED, 1);
    waitMicrosecond(1000000);
    setPinValue(RED_LED, 0);

    if(!checkIfConfigured()) // Only need to run the commands the first time through this code
        initHibernationModule();

    uint8_t log = commands();

    uint8_t data[] = {LB(LOG), log};
    writeI2c0Registers(EEPROM >> 1, HB(LOG), data, 2); // Store log in EEPROM header
    waitMicrosecond(10000); // Give time for write to take place
    if(readI2c0Register16(EEPROM >>1, LOG) != log)
        putsUart0("ERROR STORING LOG IN EERPOM!\n\n");

    setPinValue(BLUE_LED, 0);
    setPinValue(GREEN_LED, 0);

    if(!getPinValue(LIGHT)) // while light is on (outputs a 1)
    {
        HIBMODE = 1; // Change mode to periodic
        while(!(HIB_CTL_R & HIB_CTL_WRC));

        if(rtcCausedWakeUp()) // If woken up by the RTC match value
        {
            // take time stamp
            // play speaker
            // wait until jostled
            // stop speaker
            // get new time stamp
            // subtract time stamp 2 form time stamp 1 and hold it to pass into the record function
        }

        //record(log, timePassed); // TODO write function in log.c
    }
    else
    {
        HIBMODE = 0; // Change mode to periodic
    }

    if(HIBMODE == 1 && (log && SLEEP)) // periodic mode with sleep on
    {
        hibernate(HIBTIME); // Will hibernate for 1 hr
    }
    else if(HIBMODE == 1) // periodic mode with sleep off
    {
        //Complete hibernation request for when light is off
        hibernate(HIBTIMENOS);
    }
    else if(HIBMODE == 0 && (log && SLEEP)) // trigger mode with sleep on
    {
        hibernate(HIBWAKE); // Will hibernate for 1 hr
    }
    else // trigger mode with sleep off
    {
        //Complete hibernation request for when light is off
        hibernate(HIBWAKENOS);
    }

    while(true);

}
