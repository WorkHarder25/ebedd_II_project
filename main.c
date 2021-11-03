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


//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------


// Initialize Hardware
void initHw()
{
     //Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();
}

void getsUart0(char str[], uint8_t size)
{
    uint8_t count = 0;
    bool end = false;
    char c;
    while(!end)
    {
        c = getcUart0();
        end = (c == 13) || (count == size);
        if (!end)
        {
            if ((c == 8 || c == 127) && count > 0)
                count--;
            if (c >= ' ' && c < 127)
                str[count++] = c;
        }
    }
    str[count] = '\0';
}

uint8_t asciiToUint8(const char str[])
{
    uint8_t data;
    if (str[0] == '0' && tolower(str[1]) == 'x')
        sscanf(str, "%hhx", &data);
    else
        sscanf(str, "%hhu", &data);
    return data;
}

//I2C
void initI2c0(void)
{
    // Enable clocks
    SYSCTL_RCGCI2C_R |= SYSCTL_RCGCI2C_R0;
    _delay_cycles(3);
    enablePort(PORTB);

    // Configure I2C
    selectPinPushPullOutput(I2C0SCL);
    setPinAuxFunction(I2C0SCL, GPIO_PCTL_PB2_I2C0SCL);
    selectPinOpenDrainOutput(I2C0SDA);
    setPinAuxFunction(I2C0SDA, GPIO_PCTL_PB3_I2C0SDA);

    // Configure I2C0 peripheral
    I2C0_MCR_R = 0;                                     // disable to program
    I2C0_MTPR_R = 199;                                  // (40MHz/2) / (199+1) = 100kbps
    I2C0_MCR_R = I2C_MCR_MFE;                           // master
    I2C0_MCS_R = I2C_MCS_STOP;
}

// For simple devices with a single internal register
void writeI2c0Data(uint8_t add, uint8_t data)
{
    I2C0_MSA_R = add << 1; // add:r/~w=0
    I2C0_MDR_R = data;
    I2C0_MICR_R = I2C_MICR_IC;
    I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP;
    while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
}

uint8_t readI2c0Data(uint8_t add)
{
    I2C0_MSA_R = (add << 1) | 1; // add:r/~w=1
    I2C0_MICR_R = I2C_MICR_IC;
    I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP;
    while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
    return I2C0_MDR_R;
}

// For devices with multiple registers
void writeI2c0Register(uint8_t add, uint8_t reg, uint8_t data)
{
    I2C0_MSA_R = add << 1; // add:r/~w=0
    I2C0_MDR_R = reg;
    I2C0_MICR_R = I2C_MICR_IC;
    I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN;
    while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
    I2C0_MDR_R = data;
    I2C0_MICR_R = I2C_MICR_IC;
    I2C0_MCS_R = I2C_MCS_RUN | I2C_MCS_STOP;
    while (!(I2C0_MRIS_R & I2C_MRIS_RIS));
}

void writeI2c0Registers(uint8_t add, uint8_t reg, uint8_t data[], uint8_t size)
{
    uint8_t i;
    I2C0_MSA_R = add << 1; // add:r/~w=0
    I2C0_MDR_R = reg;
    if (size == 0)
    {
        I2C0_MICR_R = I2C_MICR_IC;
        I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP;
        while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
    }
    else
    {
        I2C0_MICR_R = I2C_MICR_IC;
        I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN;
        while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
        for (i = 0; i < size-1; i++)
        {
            I2C0_MDR_R = data[i];
            I2C0_MICR_R = I2C_MICR_IC;
            I2C0_MCS_R = I2C_MCS_RUN;
            while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
        }
        I2C0_MDR_R = data[size-1];
        I2C0_MICR_R = I2C_MICR_IC;
        I2C0_MCS_R = I2C_MCS_RUN | I2C_MCS_STOP;
        while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
    }
}

uint8_t readI2c0Register(uint8_t add, uint8_t reg)
{
    I2C0_MSA_R = add << 1; // add:r/~w=0
    I2C0_MDR_R = reg;
    I2C0_MICR_R = I2C_MICR_IC;
    I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN;
    while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
    I2C0_MSA_R = (add << 1) | 1; // add:r/~w=1
    I2C0_MICR_R = I2C_MICR_IC;
    I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP;
    while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
    return I2C0_MDR_R;
}

bool pollI2c0Address(uint8_t add)
{
    I2C0_MSA_R = (add << 1) | 1; // add:r/~w=1
    I2C0_MICR_R = I2C_MICR_IC;
    I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP;
    while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
    return !(I2C0_MCS_R & I2C_MCS_ERROR);
}

bool isI2c0Error(void)
{
    return (I2C0_MCS_R & I2C_MCS_ERROR);
}


//EEPROM
void initEeprom(void)
{
    SYSCTL_RCGCEEPROM_R = 1;
    _delay_cycles(3);
    while (EEPROM_EEDONE_R & EEPROM_EEDONE_WORKING);
}

void writeEeprom(uint16_t add, uint32_t data)
{
    EEPROM_EEBLOCK_R = add >> 4;
    EEPROM_EEOFFSET_R = add & 0xF;
    EEPROM_EERDWR_R = data;
    while (EEPROM_EEDONE_R & EEPROM_EEDONE_WORKING);
}

uint32_t readEeprom(uint16_t add)
{
    EEPROM_EEBLOCK_R = add >> 4;
    EEPROM_EEOFFSET_R = add & 0xF;
    return EEPROM_EERDWR_R;
}


int main(void)
{
	return 0;
}
