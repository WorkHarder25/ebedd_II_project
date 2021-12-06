//log.c

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
#include "i2c0.h"
#include "string.h"
#include "adc0.h"
#include "utility.h"
#include <stdlib.h>

// PortB masks for I2C
#define SDA_MASK 8
#define SCL_MASK 4

// Pins
#define I2C0SCL PORTB,2
#define I2C0SDA PORTB,3
#define LEVELSELECT  PORTA,7

#define HB(x) (x >> 8) & 0xFF//defines High Byte for reading/writing to EEPROM
#define LB(x) (x) & 0xFF//defines low byte for reading/writing to EEPROM

#define CURREG 0x0005
#define LOG    0x0006 // Will hold log
#define EEPROM 0xA0

// Defines for the log field
#define MAG         1   // 00000001
#define ACCEL       2   // 00000010
#define GYRO        4   // 00000100
#define TEMP        8   // 00001000
#define TIME        16  // 00010000 ... Will always be on
#define LEVELING    32  // 00100000 ... if on, give random value
#define ENCRYPT     64  // 01000000 ... if on, use encryption
#define SLEEP       128 // 10000000

// HIBDATA offsets
#define HIBLOG      (*((volatile uint32_t *)(0x400FC030 + (0*4))))  // log
#define HIBKEY      (*((volatile uint32_t *)(0x400FC030 + (8*4)))) // encryption key

#define HB(x) (x >> 8) & 0xFF//defines High Byte for reading/writing to EEPROM
#define LB(x) (x) & 0xFF//defines low byte for reading/writing to EEPROM

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

bool storeEEPROMdata(uint16_t add, uint32_t data)
{
    uint32_t temp;
    if(HIBLOG & ENCRYPT) // if encryption is on...
    {
        temp = encrypt(data);
    }
    else
        temp = data;

    uint8_t i2cData[] = {LB(add), temp>>24, temp>>16, temp>>8, temp};//Array for address low byte and data you are storing
    writeI2c0Registers(0xA0 >> 1, HB(add), i2cData, 5);//Writes to address in EEPROM using address high byte, array of address low byte, and 2 for size
    waitMicrosecond(1000000);

    uint32_t readEEprom = readI2c0Register16(0xA0 >> 1, add);
    readEEprom= readEEprom<<8;
    uint8_t dummy= readI2c0Register16(0xA0 >> 1, add+1);
    readEEprom+=dummy;
    readEEprom= readEEprom<<8;
    dummy=readI2c0Register16(0xA0 >> 1, add+2);
    readEEprom+=dummy;
    readEEprom= readEEprom<<8;
    dummy=readI2c0Register16(0xA0 >> 1, add+3);
    readEEprom+=dummy;

    if(HIBLOG & ENCRYPT) // if encryption is on...
    {
        readEEprom = decrypt(readEEprom);
    }

    if(readEEprom!= data) //data was time
        return false;

    add += 4;

    uint8_t data2[] = {LB(CURREG), HB(add), LB(add)};
    writeI2c0Registers(EEPROM >> 1, HB(CURREG), data2, 3);
    waitMicrosecond(50000);

    return true;
}

uint32_t readEEPROM32(uint16_t add)
{
    uint32_t readEEprom = readI2c0Register16(0xA0 >> 1, add);
    readEEprom= readEEprom<<8;
    uint8_t dummy= readI2c0Register16(0xA0 >> 1, add+1);
    readEEprom+=dummy;
    readEEprom= readEEprom<<8;
    dummy=readI2c0Register16(0xA0 >> 1, add+2);
    readEEprom+=dummy;
    readEEprom = readEEprom<<8;
    dummy=readI2c0Register16(0xA0 >> 1, add+3);
    readEEprom+=dummy;

    if(HIBLOG & ENCRYPT) // if encryption is on...
    {
        readEEprom = decrypt(readEEprom);
    }

    return readEEprom;
}

bool logMag(uint16_t add)
{
    writeI2c0Register(0x68, 0x1A, 0x0);//configure register 26 DLPF 0x7, 0x17
    //writeI2c0Register(0x68, 0x23, 0xFF);//register 35 FIFO enable
    writeI2c0Register(0x68, 0x38, 0x01);// interrupt register
    writeI2c0Register(0x68, 0x37, 0x02);
    writeI2c0Register(0x0C, 0x0A, 0x01);
    while(!(readI2c0Register(0x0C, 0x02) & 1));
    uint32_t x = readI2c0Register(0x0C, 0x04);
    x = (x << 8) | readI2c0Register(0x0C, 0x03);
    uint32_t y = readI2c0Register(0x0C, 0x06);
    y = (y << 8) | readI2c0Register(0x0C, 0x05);
    uint32_t z = readI2c0Register(0x0C, 0x08);
    z = (z << 8) | readI2c0Register(0x0C, 0x07);

    // TODO REMOVE TEST OUTPUT
    char buffer[20];
    putsUart0("Mag X - ");
    itoa(x, buffer, 10);
    putsUart0(buffer);
    putsUart0(" at add ");
    itoa(add, buffer, 10);
    putsUart0(buffer);
    putsUart0(" Mag Y - ");
    itoa(y, buffer, 10);
    putsUart0(buffer);
    putsUart0(" at add ");
    itoa(add+4, buffer, 10);
    putsUart0(buffer);
    putsUart0(" Mag Z - ");
    itoa(z, buffer, 10);
    putsUart0(buffer);
    putsUart0(" at add ");
    itoa(add+8, buffer, 10);
    putsUart0(buffer);

    if (!storeEEPROMdata(add, x))
           return false;
    if (!storeEEPROMdata(add+4, y))
           return false;
    if (!storeEEPROMdata(add+8, z))
           return false;

       return true;
}

bool logAcc(uint16_t add)
{
    writeI2c0Register(0x68, 0x1A, 0x0);//configure register 26 DLPF 0x7, 0x17
    //writeI2c0Register(0x68, 0x23, 0xFF);//register 35 FIFO enable
    writeI2c0Register(0x68, 0x38, 0x01);// interrupt register
    writeI2c0Register(0x68, 0x1C, 0x18);//configure register 28 page 14  accelerometer

    uint16_t accregx = readI2c0Register(0x68, 0x3B);//register 68 page 31  ACC_XOUT_H 59
    uint8_t accregx2 = readI2c0Register(0x68, 0x3C);//register 68 page 31  ACC_XOUT_L 60
    uint32_t accX = (accregx<<8) | accregx2;//combined high byte and low byte
    uint16_t accregy = readI2c0Register(0x68, 0x3D);//register 68 page 31  ACC_YOUT_H 61
    uint8_t accregy2 = readI2c0Register(0x68, 0x3E);//register 68 page 31  ACC_YOUT_L 62
    uint32_t accY = (accregy<<8) | accregy2;//combined high byte and low byte
    uint16_t accregz = readI2c0Register(0x68, 0x3F);//register 68 page 31  ACC_ZOUT_H 63
    uint8_t accregz2 = readI2c0Register(0x68, 0x40);//register 68 page 31  ACC_ZOUT_L 64
    uint32_t accZ = (accregz<<8) | accregz2;//combined high byte and low byte

    if (!storeEEPROMdata(add, accX))
           return false;
    if (!storeEEPROMdata(add+4, accY))
           return false;
    if (!storeEEPROMdata(add+8, accZ))
           return false;

    // TODO REMOVE TEST OUTPUT
    char buffer[20];
    putsUart0("Accel X - ");
    itoa(accX, buffer, 10);
    putsUart0(buffer);
    putsUart0(" at add ");
    itoa(add, buffer, 10);
    putsUart0(buffer);
    putsUart0(" Accel Y - ");
    itoa(accY, buffer, 10);
    putsUart0(buffer);
    putsUart0(" at add ");
    itoa(add+4, buffer, 10);
    putsUart0(buffer);
    putsUart0(" Accel Z - ");
    itoa(accZ, buffer, 10);
    putsUart0(buffer);
    putsUart0(" at add ");
    itoa(add+8, buffer, 10);
    putsUart0(buffer);

       return true;
}

bool logGyro(uint16_t add)
{
    writeI2c0Register(0x68, 0x1A, 0x0);//configure register 26 DLPF 0x7, 0x17
    //writeI2c0Register(0x68, 0x23, 0xFF);//register 35 FIFO enable
    writeI2c0Register(0x68, 0x38, 0x01);// interrupt register
    writeI2c0Register(0x68, 0x1B, 0x18);//register 27 gyro write a value of 00011011(0x1B) or 0x18

    //int8_t gyroreg = readI2c0Register(0x68, 0x1B);//*change these lines if needed for accelerometer* this is just for checking that you wrote to register correctly
    uint16_t gyroreg2 = readI2c0Register(0x68, 0x43);//register 68 page 33  GYRO_XOUT_H
    uint8_t gyroreg3 = readI2c0Register(0x68, 0x44);//register 68 page 33  GYRO_XOUT_L
    uint32_t gyroX = (gyroreg2<<8) | gyroreg3;//combined high byte and low byte
    uint16_t gyroregy = readI2c0Register(0x68, 0x45);//register 68 page 33  GYRO_YOUT_H
    uint8_t gyroregy2 = readI2c0Register(0x68, 0x46);//register 68 page 33  GYRO_YOUT_L
    uint32_t gyroY = (gyroregy<<8) | gyroregy2;//combined high byte and low byte
    uint16_t gyroregz = readI2c0Register(0x68, 0x47);//register 68 page 33  GYRO_ZOUT_H
    uint8_t gyroregz2 = readI2c0Register(0x68, 0x48);//register 68 page 33  GYRO_ZOUT_L
    uint32_t gyroZ = (gyroregz<<8) | gyroregz2;//combined high byte and low byte

    if (!storeEEPROMdata(add, gyroX))
        return false;
    if (!storeEEPROMdata(add+4, gyroY))
        return false;
    if (!storeEEPROMdata(add+8, gyroZ))
        return false;

    // TODO REMOVE TEST OUTPUT
    char buffer[20];
    putsUart0("Gyro X - ");
    itoa(gyroX, buffer, 10);
    putsUart0(buffer);
    putsUart0(" at add ");
    itoa(add, buffer, 10);
    putsUart0(buffer);
    putsUart0(" Gyro Y - ");
    itoa(gyroY, buffer, 10);
    putsUart0(buffer);
    putsUart0(" at add ");
    itoa(add+4, buffer, 10);
    putsUart0(buffer);
    putsUart0(" Gyro Z - ");
    itoa(gyroZ, buffer, 10);
    putsUart0(buffer);
    putsUart0(" at add ");
    itoa(add+8, buffer, 10);
    putsUart0(buffer);

    return true;
}

bool logTemp(uint16_t add)
{
    volatile long ADC_Output=0;
    volatile long temp_c=0;
    setAdc0Ss3Mux(1);
    ADC_Output= (readAdc0Ss3() & 0xFFF);
    temp_c= 147.5-((247.5*ADC_Output)/4096);//equation for converting temp to celsius

    if (!storeEEPROMdata(add, temp_c))
           return false;

    // TODO remove test output
    char buffer[20];
    putsUart0("Temp - ");
    itoa(temp_c, buffer, 10);
    putsUart0(buffer);
    putsUart0(" at add ");
    itoa(add, buffer, 10);
    putsUart0(buffer);

       return true;
}

// Store time the alarm was allowed to go off
bool logTime(uint16_t add, uint32_t* time)
{
    double subseconds = time[1]/32768;
    double seconds = time[0] + subseconds;

    if(!storeEEPROMdata(add, seconds))
        return false;

    //TODO remove test output
    char buffer[20];
    putsUart0("Time - ");
    itoa(seconds, buffer, 10);
    putsUart0(buffer);
    putsUart0(" at add ");
    itoa(add, buffer, 10);
    putsUart0(buffer);

    return true;
}

uint32_t encrypt(uint32_t value)
{
    uint32_t eValue = value + HIBKEY;
    return eValue;
}

uint32_t decrypt(uint32_t value)
{
    uint32_t dValue = value - HIBKEY;
    return dValue;
}

bool record(uint32_t* time, uint16_t add)
{
    // Note: Time stamp is already stored at this point

    uint8_t i = 0;

    if(HIBLOG & MAG)
    {
        if(!logMag(add+i))
            return false;
        i += 12;
        putsUart0("\n");
    }
    if(HIBLOG & ACCEL)
    {
        if(!logAcc(add+i))
            return false;
        i += 12;
        putsUart0("\n");
    }
    if(HIBLOG & GYRO)
    {
        if(!logGyro(add+i))
            return false;
        i += 12;
        putsUart0("\n");
    }
    if(HIBLOG & TEMP)
    {
        if(!logTemp(add+i))
            return false;
        i += 4;
        putsUart0("\n");
    }
    if(HIBLOG & TIME)
    {
        if(!logTime(add+i, time))
            return false;
        putsUart0("\n");
    }

    return true;
}
