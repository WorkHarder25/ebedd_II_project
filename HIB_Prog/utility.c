// utility.c
// This files will the Uart utility as a user interface

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
#include "speaker.h"
#include "leveling.h"

// USE TO SET A TIME VALUE FOR PERIOD TIME WHEN TRIGGER MOD IS ORGINALLY SELECTED
#define PRESETTIME  5

// Pins
#define LIGHT        PORTA,4 // 1 when light is on and 0 when light is off
#define LEVELSELECT  PORTA,7

// Defines for the log field
#define MAG         1   // 00000001
#define ACCEL       2   // 00000010
#define GYRO        4   // 00000100
#define TEMP        8   // 00001000
#define TIME        16  // 00010000 ... Will always be on
#define LEVELING    32  // 00100000 ... if on, give random value
#define ENCRYPT     64  // 01000000 ... if on, use encryption
#define SLEEP       128 // 10000000

// EEPROM Addresses
#define CURREG      0x0005 // Will hold value of the last register used
#define LOG         0x0006 // Will hold log
#define MAXSAMP     0x0007 // Will hold max sample amount
#define COUNTSAMP   0x0008 // Will actual sample count
#define EEPROM      0xA0  // Add of EEPROM

// Hibernate values
#define HIBWAKE     0x000153 // 0001 0101 0011 .. NO RTC MATCH
#define HIBTIME     0x00014B // 0001 0100 1011 .. NO WAKE PIN

// HIBDATA offsets
#define HIBLOG      (*((volatile uint32_t *)(0x400FC030 + (0*4))))  // log
#define HIBMSAMP    (*((volatile uint32_t *)(0x400FC030 + (1*4))))  // max samples
#define HIBMODE     (*((volatile uint32_t *)(0x400FC030 + (2*4))))  // 0 trigger - 1 periodic
#define HIBMON      (*((volatile uint32_t *)(0x400FC030 + (3*4))))  // Month
#define HIBDAY      (*((volatile uint32_t *)(0x400FC030 + (4*4))))  // Day
#define HIBHR       (*((volatile uint32_t *)(0x400FC030 + (5*4))))  // Hour
#define HIBMIN      (*((volatile uint32_t *)(0x400FC030 + (6*4))))  // Minute
#define HIBSEC      (*((volatile uint32_t *)(0x400FC030 + (7*4)))) // Second
#define HIBKEY      (*((volatile uint32_t *)(0x400FC030 + (8*4)))) // encryption key
#define HIBSEED     (*((volatile uint32_t *)(0x400FC030 + (9*4)))) // level seed
#define HIBCSAMP    (*((volatile uint32_t *)(0x400FC030 + (10*4)))) // sample count
#define HIBRUN      (*((volatile uint32_t *)(0x400FC030 + (11*4)))) // if run is on (1 if on, 0 if off)
#define HIBPERIOD   (*((volatile uint32_t *)(0x400FC030 + (12*4)))) // store period of time for hibernates in periodic mode
#define HIBOUTPUT   (*((volatile uint32_t *)(0x400FC030 + (13*4)))) // This is used to note if output has just started...used for the leveling program

#define HB(x) (x >> 8) & 0xFF//defines High Byte for reading/writing to EEPROM
#define LB(x) (x) & 0xFF//defines low byte for reading/writing to EEPROM


// Use user input to determine what command to do and call functions. Returns false if run is activated
void commands()
{
    char input[128];
    USER_INPUT uIn;

    uint8_t temp; // dummy variable used as needed
    uint8_t log = TIME; // Set the log vars to log time

    while(HIBCSAMP < HIBMSAMP-1) // Make sure we do not exceed max sample
    {
        if(HIBRUN == 0) // if not running
        {
            // clear input and uIn for next use

            strcpy(input, "\0");
            strcpy(uIn.command, "\0");
            strcpy(uIn.fields, "\0");

            putsUart0("Enter command: ");

            getsUart0(input);
            putcUart0('\n');
            parseCommand(input, &uIn);

            // Check what command was called

            if(!strcmp(uIn.command, "time"))
            {
                // Must check if user wants to store values or if they are requesting the time
                if(uIn.fieldCount == 0) // display time
                {
                    outputTime();
                }
                else // store time
                {
                    temp = setUTime(uIn.fields); // returns 0 if successful, 1 if input was invalid, and 2 if there was an EEPROM error

                    switch(temp)
                    {
                    case 0:
                        putsUart0("Time set successfully.\n\n");
                        break;
                    case 1:
                        putsUart0("Invalid input. Please try again.\n\n");
                        break;
                    case 2:
                        putsUart0("ERROR IN EEPROM.\n\n");
                    }
                }
            }
            else if(!strcmp(uIn.command, "date"))
            {
                // Must check if user want to store values or if they are requesting the date
                if(uIn.fieldCount == 0) // display date
                {
                    outputDate();
                }
                else // store time
                {
                    temp = setUDate(uIn.fields); // returns 0 if successful, 1 if input was invalid, and 2 if there was an EEPROM error

                    switch(temp)
                    {
                    case 0:
                        putsUart0("Date set successfully.\n\n");
                        break;
                    case 1:
                        putsUart0("Invalid input. Please try again.\n\n");
                        break;
                    case 2:
                        putsUart0("ERROR IN EEPROM.\n\n");
                    }
                }
            }
            else if(!strcmp(uIn.command, "temp"))
            {
                printTemp();
            }
            else if(!strcmp(uIn.command, "reset"))
            {
                NVIC_APINT_R |= NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
            }
            else if(!strcmp(uIn.command, "log"))
            {
                if(uIn.fieldCount == 0)
                    logFields(log);
                else
                {
                if(!strcmp(uIn.fields, "compass"))
                    log |= MAG;
                else if(!strcmp(uIn.fields, "accel"))
                    log |= ACCEL;
                else if(!strcmp(uIn.fields, "gyro"))
                    log |= GYRO;
                else if(!strcmp(uIn.fields, "temp"))
                    log |= TEMP;
                else
                    putsUart0("Not a valid log command. Please try again.\n\n");
                }
            }
            else if(!strcmp(uIn.command, "samples"))
            {
                if(uIn.fieldCount == 1)
                {
                    if(uIn.fields[0] > 47 && uIn.fields[0] < 58)
                    {
                    uint16_t maxSample = myatoi(uIn.fields);
                    uint8_t data[] = {LB(MAXSAMP), maxSample};
                    writeI2c0Registers(EEPROM >> 1, HB(MAXSAMP), data, 2);
                    waitMicrosecond(10000);
                    if(readI2c0Register16(EEPROM >>1, MAXSAMP) != maxSample) // If data was not stored correctly, return error msg 2
                        putsUart0("ERROR STORING MAX SAMPLE COUNT IN EEPROM\n\n");
                    HIBMSAMP = maxSample; // Store max number of samples in HIBDATA
                    }
                    else
                        putsUart0("Only numbers can be stored for 'samples.' Please try again.\n\n");
                }
                else
                    putsUart0("Not a valid input for 'samples.' Please try again.\n\n");
            }
            else if(!strcmp(uIn.command, "sleep"))
            {
                if(!strcmp(uIn.fields, "on"))
                    log |= SLEEP;
                else if(!strcmp(uIn.fields, "off"))
                    log = log & ~SLEEP;
                else
                    putsUart0("Not a valid input for sleep. Please try again.\n\n");

            }
            else if(!strcmp(uIn.command, "leveling"))
            {
                if(!strcmp(uIn.fields, "on"))
                    log |= LEVELING;
                else if(!strcmp(uIn.fields, "off"))
                    log = log & ~LEVELING;
                else
                {
                    putsUart0("Not a valid input for leveling. Please try again.\n\n");
                }

            }
            else if(!strcmp(uIn.command, "encrypt"))
            {
                uint32_t key;

                if(!strcmp(uIn.fields, "off"))
                {
                    log = log & ~ENCRYPT;
                    key = 0;
                }
                else
                {
                    log |= ENCRYPT;
                    key = myatoi(uIn.fields);
                }

                HIBKEY = key; // Store the encryption key in HIBDATA
                while(!(HIB_CTL_R & HIB_CTL_WRC));

            }
            else if(!strcmp(uIn.command, "periodic"))
            {
                if(uIn.fieldCount == 1)
                {
                    uint8_t i;
                    bool valid = true;
                    // store user time for period time
                    for(i = 0; i < strlen(uIn.fields); i++)
                    {
                        if(uIn.fields[i] < 48 || uIn.fields[i] > 57)
                            valid = false;
                    }
                    if(valid)
                    {

                        // Store log in EEPROM
                        uint8_t data[] = {LB(LOG), log};
                        writeI2c0Registers(EEPROM >> 1, HB(LOG), data, 2); // Store log in EEPROM header
                        waitMicrosecond(10000); // Give time for write to take place
                        if(readI2c0Register16(EEPROM >>1, LOG) != log)
                            putsUart0("ERROR STORING LOG IN EERPOM!\n\n");
                        HIBLOG = log; // store log in HIBDATA
                        while(!(HIB_CTL_R & HIB_CTL_WRC));

                        // Turn run on
                        HIBRUN = 1;

                        HIBPERIOD = myatoi(uIn.fields);

                        uint32_t time[2];
                        getSec(HIBPERIOD, time); // devide up time entered into seconds and subseconds

                        // Set register to know where to begin writing when the board wakes up
                        uint16_t startAdd = 0x000A;
                        uint8_t data1[] = {LB(CURREG), startAdd>>8, startAdd};
                        writeI2c0Registers(EEPROM >> 1, HB(CURREG), data1, 3);
                        waitMicrosecond(30000);

                        // Check it was stored correctly
                        uint16_t add = readI2c0Register16(EEPROM >> 1, CURREG); // get upper bits
                        add = add<<8;
                        uint8_t y = readI2c0Register16(EEPROM >> 1, CURREG+1); // get lower bit
                        add += y;

                        if(add != startAdd)
                            putsUart0("ERROR STORING START ADD IN EEPROM\n\n");

                        // hibernate req
                        /*waitMicrosecond(1000000);
                        hibernate(HIBTIME, time);*/
                    }
                    else
                        putsUart0("Only integers can be used for 'periodic.' Please try again.\n\n");
                }
            else
                putsUart0("Not a valid input for 'periodic.' Please try again.\n\n");
            }
            else if(!strcmp(uIn.command, "trigger"))
            {
                if(uIn.fieldCount == 0)
                {
                    // Store log in EEPROM
                    uint8_t data[] = {LB(LOG), log};
                    writeI2c0Registers(EEPROM >> 1, HB(LOG), data, 2); // Store log in EEPROM header
                    waitMicrosecond(10000); // Give time for write to take place
                    if(readI2c0Register16(EEPROM >>1, LOG) != log)
                        putsUart0("ERROR STORING LOG IN EERPOM!\n\n");
                    HIBLOG = log; // store log in HIBDATA
                    while(!(HIB_CTL_R & HIB_CTL_WRC));

                    // Turn run on
                    HIBRUN = 1;

                    // store 0 for period time
                    HIBPERIOD = PRESETTIME;

                    uint32_t time[] = {};

                    // Set register to know where to begin writing when the board wakes up
                    uint16_t startAdd = 0x000A;
                    uint8_t data2[] = {LB(CURREG), startAdd>>8, startAdd};
                    writeI2c0Registers(EEPROM >> 1, HB(CURREG), data2, 3);
                    waitMicrosecond(30000);

                    // Check it was stored correctly
                    uint16_t add = readI2c0Register16(EEPROM >> 1, CURREG); // get upper bits
                    add = add<<8;
                    uint8_t y = readI2c0Register16(EEPROM >> 1, CURREG+1); // get lower bit
                    add += y;

                    if(add != startAdd)
                        putsUart0("ERROR STORING START ADD IN EEPROM\n\n");

                    // hibernation req
                    /*waitMicrosecond(1000000);
                    hibernate(HIBWAKE, time);*/
                }
                else
                    putsUart0("Not a valid input for 'trigger' Please try again.\n\n");
            }
            else if(!strcmp(uIn.command, "help"))
            {
                // TODO output list of commands
            }
            else if(!strcmp(uIn.command, "data"))
            {
                dataOutput();
            }
            else
            {
                putsUart0("Invalid command, please try again\n\n");
            }
        }
        else // is running
        {
            if(kbhitUart0())
            {
                strcpy(input, "\0");
                getsUart0(input);
                if(!strcmp(input, "stop"))
                {
                    stopHibernation();

                    // Store actual sample count into the EERPOm
                    uint8_t data[] = {LB(COUNTSAMP), HIBCSAMP};
                    writeI2c0Registers(EEPROM >> 1, HB(COUNTSAMP), data, 2); // Store log in EEPROM header
                    waitMicrosecond(100000); // Give time for write to take place
                    if(readI2c0Register16(EEPROM >>1, COUNTSAMP) != log)
                        putsUart0("ERROR STORING SAMPLES IN EERPOM!\n\n");

                    // Turn run off
                    HIBRUN = 0;

                    putsUart0("Log has been manually stopped. Enter 'data' to view logs\n\n");
                }
                else if(!strcmp(input, "data"))
                {
                    dataOutput();
                }
                else
                {
                    putsUart0("Board is currently taking samples. Only valid commands are 'stop' and 'data'. Please try again\n\n");
                }
            }

            if(!getPinValue(LIGHT)) // while light is on (outputs a 1)
            {
                HIBMODE = 1; // Change mode to periodic
                while(!(HIB_CTL_R & HIB_CTL_WRC));

                //if(rtcCausedWakeUp()) // If woken up by the RTC match value
                //{
                    // get next add
                    uint16_t add = getNextAdd();

                    // get original time stamp
                    uint32_t time1[2];
                    getTimeStamp(time1);

                    // play speaker (will play until complete or until jostled)
                    playAlert();
                    waitMicrosecond(1000);

                    // get new time stamp
                    uint32_t time2[2];
                    getTimeStamp(time2);
                    // subtract time stamp 2 form time stamp 1 and hold it to pass into the record function
                    uint32_t time3[] = {time2[0] - time1[0], time2[1] - time1[1]};

                    // store original time stamp
                    storeEEPROMdata(add, time1[0]);

                    char buffer[128];

                    // TODO REMOVE TEST OUTPUT
                    putsUart0("Sample ");
                    itoa(HIBCSAMP, buffer, 10);
                    putsUart0(buffer);
                    putsUart0(": Time Stamp - ");
                    itoa(time1[0], buffer, 10);
                    putsUart0(buffer);
                    putsUart0("\n");

                    if(readEEPROM32(add) != time1[0])
                    {
                        putsUart0("ERROR STORING TIMESTAMP FOR SAMPLE ");
                        char buffer[50];
                        itoa(HIBCSAMP, buffer, 10);
                        putsUart0(buffer);
                        putsUart0(" IN EEPROM\n\n");
                    }

                    if(!record(time3, add+4))
                    {
                        putsUart0("ERROR STORING SAMPLE ");
                        char buffer[30];
                        itoa(HIBCSAMP, buffer, 10);
                        putsUart0(buffer);
                        putsUart0("\n\n");

                    }

                    putsUart0("Recording sample: ");
                    itoa(HIBCSAMP+1, buffer, 10);
                    putsUart0(buffer);
                    putsUart0(" of ");
                    itoa(HIBMSAMP, buffer, 10);
                    putsUart0(buffer);
                    putsUart0("\n");

                    HIBCSAMP++;
                //}
            }
            else // light is off. No records are taken while lights are off
            {
                HIBMODE = 0; // Change mode to periodic
            }

           // turn off leveling circuit before hibernation (gets turned back on in initHw()
            setPinValue(LEVELSELECT, 0);
            waitMicrosecond(10); // give the leveling circuit a chance to turn off to avlid any issues*/

            // Activate hibernation request
            /*if(HIBMODE == 1) // periodic mode
            {
                waitMicrosecond(1000000);
                uint32_t time[] = {HIBPERIOD, 0};
                hibernate(HIBTIME, time);
            }

            else // trigger mode
            {
                waitMicrosecond(1000000);
                uint32_t time[] = {HIBPERIOD, 0};
                hibernate(HIBWAKE, time);
            }*/
        }
    }

    putsUart0("Log complete: Max samples reached\nEnter 'data' to view log information\n\n");
    getsUart0(input);
    putcUart0('\n');
    parseCommand(input, &uIn);

    if(!strcmp(uIn.command, "data"))
    {
        dataOutput();
    }
    else
    {
        putsUart0("Invalid command, please try again\n\n");
    }


}

void dataOutput()
{
    uint32_t temp;
    uint32_t readBack;
    uint32_t i, j;

    HIBOUTPUT = 1;

    // get first address
    uint16_t add = 0x000A;
    if(HIBLOG & LEVELING) // leveling on
    {
        temp = leveling();
        add += temp;
    }

    char buffer[128] = {};

    // header for output
    strcpy(buffer, "Sample,");
    strcat(buffer, "Time Stamp,");

    if(HIBLOG & MAG)
        strcat(buffer, "Magnetometer X,Magnetometer Y,Magnetometer Z,");
    if(HIBLOG & ACCEL)
        strcat(buffer, "Accelerometer X,Accelerometer Y,Accelerometer Z,");
    if(HIBLOG & GYRO)
        strcat(buffer, "Gyroscope X,Gyroscope Y,Gyroscope Z,");
    if(HIBLOG & TEMP)
        strcat(buffer, "Temperature,");
    if(HIBLOG & TIME)
        strcat(buffer, "Time Stretched For\n");

    putsUart0(buffer); // output the header

    for(i = 0; i < HIBCSAMP; i++) // while not read through all samples...
    {
        itoa(i+1, buffer, 10);
        putsUart0(buffer);
        putsUart0(",");

        // Start reading EEPROM
        // get time stamp
        readBack = readEEPROM32(add);
        add += 4;

        // convert time stamp
        secToDateTime(buffer, 0, readBack); // 0 is for the type to get the full time stamp
        putsUart0(buffer);
        putsUart0(",");

        // get log output
        if(HIBLOG & MAG)
        {
            for(j = 0; j < 3; j++)
            {
                temp = readEEPROM32(add);
                add += 4;
                itoa(temp, buffer, 10);
                putsUart0(buffer);
                putsUart0(",");
            }
        }
        if(HIBLOG & ACCEL)
        {
            for(j = 0; j < 3; j++)
            {
                temp = readEEPROM32(add);
                add += 4;
                itoa(temp, buffer, 10);
                putsUart0(buffer);
                putsUart0(",");
            }
        }
        if(HIBLOG & GYRO)
        {
            for(j = 0; j < 3; j++)
            {
                temp = readEEPROM32(add);
                add += 4;
                itoa(temp, buffer, 10);
                putsUart0(buffer);
                putsUart0(",");
            }
        }
        if(HIBLOG & TEMP)
        {
            temp = readEEPROM32(add);
            add += 4;
            itoa(temp, buffer, 10);
            putsUart0(buffer);
            putsUart0(",");
        }
        if(HIBLOG & TIME)
        {
            temp = readEEPROM32(add);
            add += 4;
            itoa(temp, buffer, 10);
            putsUart0(buffer);
        }

        putsUart0("\n");

        // get next address
        if(HIBLOG & LEVELING) // leveling in on
        {
            uint8_t temp = leveling();
            add += temp;
        }

        if(HIBOUTPUT == 1)
            HIBOUTPUT = 0;
    }
}

// Outputs what log fields are set to take measurements
void logFields(uint8_t log)
{
    putsUart0("Current sensors set to log:\n");

    if(log & MAG)
        putsUart0("- Compass\n");
    if(log & ACCEL)
        putsUart0("- Accelerometer\n");
    if(log & GYRO)
        putsUart0("- Gyroscope\n");
    if(log & TEMP)
        putsUart0("- Temperature\n");
    if(log & TIME)
        putsUart0("- Time of stretch\n");

    putcUart0('\n');
}

// Gets a string of user input
void getsUart0(char* input)
{
    uint8_t count = 0;
    uint8_t end = 0;

    do
    {
        // Get a letter from the input
        char c = getcUart0();

        // Check for backspace
        if(c == 127)
        {
            //Check if there were any letters input to 'backspace'
            if(count != 0)
                count--;
        }
        else
        {
            // Check for a carrage return or line feed
            if(c == 13 || c == 10)
            {
                // End case
                input[count] = '\0';
                end = 1;
            }
            else
            {
                // Check if input is a printable value
                if(c >= 32)
                {
                    // Store letter in array
                    input[count] = c;
                    count++;

                    // Check for max characters
                    if(count == 128)
                    {
                        // End case
                        input[count] = '\0';
                        end = 1;
                    }
                }
            }
        }
    }while(end != 1);
}

// Parse user input
void parseCommand(char* input, USER_INPUT *uIn)
{
    // Will keep track of index in input
    uint8_t count = 0;

    // Will be used to index the command and fields count
    uint8_t i = 0;

    // will keep track of how many items have been processed
    uint8_t items = 0;

    // This variable will keep track of how many items the user entered
    uIn->fieldCount = 0;

    while(input[count] != '\0')
    {
        if(input[count] >= 65 && input[count] <= 90) // Checking for capital letters
        {
            if(items == 0) // Must be command
            {
                for(i = 0; input[count] != 32 && input[count] != '\0'; i++) // fill in full command
                {
                    uIn->command[i] = input[count]; // Put first word into command
                    count++; // Inc count for input
                }

                uIn->command[i] = '\0'; // Put null term on end of command

                items++; // Inc items because the command item has been processed
                if(input[count] != '\0')
                    count++; // Inc to get past the space after the command to begin fields
            }
            else // Must be field
            {
                for(i = 0; input[count] != '\0'; i++) // fill in rest of string in fields
                {
                    uIn->fields[i] = input[count];

                    if(input[count] == 32) // Check for spaces to inc field count for validation later
                        uIn->fieldCount++;

                    count++;
                }
                uIn->fields[i] = '\0';
                uIn->fieldCount++; // Must inc once more for final field
            }
        }
        else if(input[count] >= 97 && input[count] <= 122) // Checks for lower case letters
        {
            if(items == 0) // Must be command
            {
                for(i = 0; input[count] != 32 && input[count] != '\0'; i++) // fill in full command
                {
                    uIn->command[i] = input[count]; // Put first word into command
                    count++; // Inc count for input
                }

                uIn->command[i] = '\0'; // Put null term on end of command

                items++; // Inc items because the command item has been processed
                if(input[count] != '\0')
                    count++; // Inc to get past the space after the command to begin fields
            }
            else // Must be field
            {
                for(i = 0; input[count] != '\0'; i++) // fill in rest of string in fields
                {
                    uIn->fields[i] = input[count];

                    if(input[count] == 32) // Check for spaces to inc field count for validation later
                        uIn->fieldCount++;

                    count++;
                }
                uIn->fields[i] = '\0';
                uIn->fieldCount++; // Must inc once more for final field
            }
        }
        else if(input[count] >= 48 && input[count] <= 57) // Checks for numbers
        {
            if(items == 0) // Must be command
            {
                for(i = 0; input[count] != 32 && input[count] != '\0'; i++) // fill in full command
                {
                    uIn->command[i] = input[count]; // Put first word into command
                    count++; // Inc count for input
                }

                uIn->command[i] = '\0'; // Put null term on end of command

                items++; // Inc items because the command item has been processed
                if(input[count] != '\0')
                    count++; // Inc to get past the space after the command to begin fields
            }
            else // Must be field
            {
                for(i = 0; input[count] != '\0'; i++) // fill in rest of string in fields
                {
                    uIn->fields[i] = input[count];

                    if(input[count] == 32) // Check for spaces to inc field count for validation later
                        uIn->fieldCount++;

                    count++;
                }
                uIn->fields[i] = '\0';
                uIn->fieldCount++; // Must inc once more for final field
            }
        }

    }

}

// itoa functionalities
// Convert numbers to strings for output
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
        i++;
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

// reverse the output string to put in the correct order
void reverse(char str[], uint8_t length)
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

// EEPROM address
// get the next EEPROM address to write to
uint16_t getNextAdd()
{
    uint16_t add = readI2c0Register16(EEPROM >> 1, CURREG); // get upper bits
    add = add<<8;
    uint8_t y = readI2c0Register16(EEPROM >> 1, CURREG+1); // get lower bit
    add += y;

    if(HIBLOG & LEVELING) // leveling in on
    {
        uint8_t temp = leveling();
        add += temp;
    }

    return add;
}

// convert strings to numbers
uint32_t myatoi(char* str)
{
    uint32_t num = 0;
    uint8_t i, temp;

    for(i = 0; str[i] != '\0'; i++)
    {
        temp = *(str + i) - '0'; // equ to str[i] - '0'. This works because '0' is ASCII value 48 and all other
                                 // numbers follow sequentially after it. i.e. '1' - '0' == 49 - 48 = 1 which is what we want
        num = num * 10 + temp; // Move the value already in num up by a tens place and then add on the next number
    }

    return num;
}
