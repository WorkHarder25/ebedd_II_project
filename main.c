//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC: TM4C123GH6PM
// System Clock: 40 MHz

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "clock.h"
#include "uart0.h"
#include "gpio.h"
#include "hibernation.h"
#include "wait.h"
#include "eeprom.h"
#include "i2c0.h"

// PortB masks for I2C
#define SDA_MASK 8
#define SCL_MASK 4

// Pins
#define I2C0SCL PORTB,2
#define I2C0SDA PORTB,3
#define LEVELSELECT  PORTA,7

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

    enablePort(PORTA);// Enables port A sets LevelSelect as an output
    selectPinPushPullOutput(LEVELSELECT);
    setPinValue(LEVELSELECT, 1);
}

int main(void)
{
    initHw();//initialize hardware

    uint8_t checkpoll;//variable and function for checking polling to see if the EEPROM is connected

    if (pollI2c0Address(0xA0 >> 1) == true){
        checkpoll=1;
    }

    uint16_t address = 0xBE;
    uint8_t i2cData[] = { LB(address), 0xA };//Array for address low byte and data you are storing
    writeI2c0Registers(0xA0 >> 1, HB(address), i2cData, 2);//Writes to address in EEPROM using address high byte, array of address low byte, and 2 for size
    uint16_t data = readI2c0Register16(0xA0 >> 1, address);//Reads value stored in address of EEPROM

    return 0;
}

