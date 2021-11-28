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
#include "string.h"
#include "adc0.h"
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

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------


void initAdc0Ss3fortemperature()
{
    // Enable clocks
    SYSCTL_RCGCADC_R |= SYSCTL_RCGCADC_R0;
    _delay_cycles(16);

    // Configure ADC
    ADC0_ACTSS_R &= ~ADC_ACTSS_ASEN3;                // disable sample sequencer 3 (SS3) for programming
    ADC0_CC_R = ADC_CC_CS_SYSPLL;                    // select PLL as the time base (not needed, since default value)
    ADC0_PC_R = ADC_PC_SR_1M;                        // select 1Msps rate
    ADC0_EMUX_R = ADC_EMUX_EM3_PROCESSOR;            // select SS3 bit in ADCPSSI as trigger
    ADC0_SSCTL3_R = 0x0E;                 // mark first sample as the end
    ADC0_ACTSS_R |= ADC_ACTSS_ASEN3;                 // enable SS3 for operation
}

// Initialize Hardware
void initHw()
{

     //Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();//change to 400 kHz
    initUart0();
    //initAdc0Ss3();//for temperature
    initAdc0Ss3fortemperature();
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


uint8_t readI2c0Register16(uint8_t add, uint16_t reg)
{
    I2C0_MSA_R = add << 1; // Write address
    I2C0_MDR_R = (reg >> 8) & 0xFF; // Get HB of add
    I2C0_MICR_R = I2C_MICR_IC;
    I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN; // Transmit HB of add
    while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
    I2C0_MDR_R = reg & 0xFF; // Get LB of add
    I2C0_MICR_R = I2C_MICR_IC;
    I2C0_MCS_R = I2C_MCS_RUN; // Transmit LB
    while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
    I2C0_MSA_R = (add << 1) | 1; // BEGIN READ
    I2C0_MICR_R = I2C_MICR_IC;
    I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP; // Complete read and stop
    while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
    return I2C0_MDR_R;
}

// reverse the output string to put in the correct order
void reverse(char* str, uint8_t length)
{
    uint8_t start = 0;
    uint8_t end = length - 1;
    char temp;

    while (start < end)
    {
        temp = *(str + start); // pointer math -- equ to str[start]
        *(str + start) = *(str + end); // equ to str[start] = str[end] -- swap letters
        *(str + end) = temp; //  put letter that was at str[start] at str[end]

        start++;
        end--;
    }

}

void itoa(int32_t num, char* buffer, uint8_t base)
{
uint8_t i = 0;
uint32_t temp;
bool neg = false;



// Handle 0 explicitly, otherwise empty string is printed for num = 0
if(num == 0)
{
buffer[i++] = '0';
buffer[i] = '\0';
return;
}



if(num < 0)
{
neg = true;
num = abs(num);
}



while(num != 0)
{
temp = num % base; // Gets last digit of number with base 10 (will always be base 10)
buffer[i] = temp + '0'; // ASCII value of 0 is 48 with other digits following sequentially after
// i.e. '1' is ASCII 49. '0' + 1 == 48 + 1 = 49 == '1'
i++;
num = num/base; // Integer division allows for dividing by the base to remove the number just processed
}



if(neg == true)
{
buffer[i] = '-';
i++;
}



buffer[i] = '\0'; // append null terminator to end str



// The number was processed from LSB to MSB, meaning the LSB is currently at the front of buffer...
// Therefore, we need to reverse buffer to get the correct number



uint8_t length = strlen(buffer);
reverse(buffer, length);
}







int main(void)
{
    initHw();//initialize hardware
    initI2c0();//initialize I2C
    char IMUGYROX[120];
    char IMUGYROY[120];
    char IMUGYROZ[120];
    char IMUACCX[120];
    char IMUACCY[120];
    char IMUACCZ[120];
    char IMUTEMP[120];
    char IMUMAGX[120];
    char IMUMAGY[120];
    char IMUMAGZ[120];

    enablePort(PORTA);// Enables port A sets LevelSelect as an output
    selectPinPushPullOutput(LEVELSELECT);
    setPinValue(LEVELSELECT, 1);

    uint8_t checkpoll;//variable and function for checking polling to see if the EEPROM is connected

    if (pollI2c0Address(0xA0 >> 1) == true){
        checkpoll=1;
    }

    uint16_t address = 0xBE;
    uint8_t i2cData[] = { LB(address), 0xA };//Array for address low byte and data you are storing
    writeI2c0Registers(0xA0 >> 1, HB(address), i2cData, 2);//Writes to address in EEPROM using address high byte, array of address low byte, and 2 for size
    uint16_t data = readI2c0Register16(0xA0 >> 1, address);//Reads value stored in address of EEPROM


while(true){

    volatile long ADC_Output=0;
    volatile long temp_c=0;
    setAdc0Ss3Mux(1);
    ADC_Output= (readAdc0Ss3() & 0xFFF);
    temp_c= 147.5-((247.5*ADC_Output)/4096);//equation for converting temp to celsius

    uint8_t i2cDataIMU= readI2c0Data(0x68);
    writeI2c0Register(0x68, 0x1A, 0x0);//configure register 26 DLPF 0x7, 0x17
    //writeI2c0Register(0x68, 0x23, 0xFF);//register 35 FIFO enable
    writeI2c0Register(0x68, 0x38, 0x01);// interrupt register

    writeI2c0Register(0x68, 0x1B, 0x18);//register 27 gyro write a value of 00011011(0x1B) or 0x18*change these lines if needed for accelerometer*

    writeI2c0Register(0x68, 0x1C, 0x18);//configure register 28 page 14 was writeI2c0Register(0x68, 0x1C, 0x18); accelerometer
    //writeI2c0Register(0x68, 0x1D, 0x05);

    writeI2c0Register(0x68, 0x37, 0x02);// turn on the bypass multiplexer.
    writeI2c0Register(0x0C, 0x0A, 0x00);//configure magnetometer pages 24 and 48 of both data sheets 0x00, 01, or 11
    waitMicrosecond(100);
    writeI2c0Register(0x0C, 0x0A, 0x16); //set the Magnetometer to continuous mode 2(100Hz) and 16-bit output
    waitMicrosecond(100);



    int8_t gyroreg = readI2c0Register(0x68, 0x1B);//*change these lines if needed for accelerometer* this is just for checking that you wrote to register correctly
    int16_t gyroreg2 = readI2c0Register(0x68, 0x43);//register 68 page 33  GYRO_XOUT_H
    int8_t gyroreg3 = readI2c0Register(0x68, 0x44);//register 68 page 33  GYRO_XOUT_L
    int16_t gyroX = (gyroreg2<<8) | gyroreg3;//combined high byte and low byte
    int16_t gyroregy = readI2c0Register(0x68, 0x45);//register 68 page 33  GYRO_YOUT_H
    int8_t gyroregy2 = readI2c0Register(0x68, 0x46);//register 68 page 33  GYRO_YOUT_L
    int16_t gyroY = (gyroregy<<8) | gyroregy2;//combined high byte and low byte
    int16_t gyroregz = readI2c0Register(0x68, 0x47);//register 68 page 33  GYRO_ZOUT_H
    int8_t gyroregz2 = readI2c0Register(0x68, 0x48);//register 68 page 33  GYRO_ZOUT_L
    int16_t gyroZ = (gyroregz<<8) | gyroregz2;//combined high byte and low byte
    uint32_t IMUpoll= pollI2c0Address(0x68);//polling to make sure we can read from device *0x68
    uint16_t whoami=readI2c0Register(0x68, 0x75);

    int16_t accregx = readI2c0Register(0x68, 0x3B);//register 68 page 31  ACC_XOUT_H 59
    int8_t accregx2 = readI2c0Register(0x68, 0x3C);//register 68 page 31  ACC_XOUT_L 60
    int16_t accX = (accregx<<8) | accregx2;//combined high byte and low byte
    int16_t accregy = readI2c0Register(0x68, 0x3D);//register 68 page 31  ACC_YOUT_H 61
    int8_t accregy2 = readI2c0Register(0x68, 0x3E);//register 68 page 31  ACC_YOUT_L 62
    int16_t accY = (accregy<<8) | accregy2;//combined high byte and low byte
    int16_t accregz = readI2c0Register(0x68, 0x3F);//register 68 page 31  ACC_ZOUT_H 63
    int8_t accregz2 = readI2c0Register(0x68, 0x40);//register 68 page 31  ACC_ZOUT_L 64
    int16_t accZ = (accregz<<8) | accregz2;//combined high byte and low byte

    int16_t magregx = readI2c0Register(0x0C, 0x04);//address 12 pages 24 and 48 of both data sheets MAG_XOUT_H
    int8_t magregx2 = readI2c0Register(0x0C, 0x03);//address 12 pages 24 and 48 of both data sheets MAG_XOUT_L
    int16_t magX = (magregx<<8) | magregx2;//combined high byte and low byte
    int16_t magregy = readI2c0Register(0x0C, 0x06);//address 12 pages 24 and 48 of both data sheets MAG_YOUT_H
    int8_t magregy2 = readI2c0Register(0x0C, 0x05);//address 12 pages 24 and 48 of both data sheets MAG_YOUT_H
    int16_t magY = (magregy<<8) | magregy2;//combined high byte and low byte
    int16_t magregz = readI2c0Register(0x0C, 0x08);//address 12 pages 24 and 48 of both data sheets MAG_ZOUT_H
    int8_t magreg2 = readI2c0Register(0x0C, 0x07);//address 12 pages 24 and 48 of both data sheets MAG_ZOUT_H
    int16_t magZ = (magregz<<8) | magreg2;//combined high byte and low byte
    //pow(2,15)=32768
    int16_t magXadjusted= ((magX/32768)*4900);//4900 is magnetometer sensitivity: 4800 uT
    int16_t magYadjusted= ((magY/32768)*4900);
    int16_t magZadjusted= ((magZ/32768)*4900);
    uint16_t whoami2=readI2c0Register(0x0C, 0x75);

    itoa(gyroX, IMUGYROX, 10);//was i2cDataIMU
    itoa(gyroY, IMUGYROY, 10);
    itoa(gyroZ, IMUGYROZ, 10);

    itoa(accX, IMUACCX, 10);
    itoa(accY, IMUACCY, 10);
    itoa(accZ, IMUACCZ, 10);

    itoa(magXadjusted, IMUMAGX, 10);
    itoa(magYadjusted, IMUMAGY, 10);
    itoa(magZadjusted, IMUMAGZ, 10);

    itoa(temp_c, IMUTEMP, 10);


    putsUart0(IMUGYROX);
    putcUart0('\n');
    putsUart0(IMUGYROY);
    putcUart0('\n');
    putsUart0(IMUGYROZ);
    putcUart0('\n');
    putcUart0('\n');
    putsUart0(IMUTEMP);
    putcUart0('\n');
    putcUart0('\n');
    putsUart0(IMUACCX);
    putcUart0('\n');
    putsUart0(IMUACCY);
    putcUart0('\n');
    putsUart0(IMUACCZ);
    putcUart0('\n');
    putcUart0('\n');
    putsUart0(IMUMAGX);
    putcUart0('\n');
    putsUart0(IMUMAGY);
    putcUart0('\n');
    putsUart0(IMUMAGZ);
    putcUart0('\n');
    putcUart0('\n');
    waitMicrosecond(500000);//2000000
}

	return 0;
}
