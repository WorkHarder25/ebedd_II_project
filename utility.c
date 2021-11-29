// utility.c
// This files will the Uart utility as a user interface

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "tm4c123gh6pm.h"
#include "utility.h"
#include "uart0.h"
#include "string.h"
#include "time.h"
#include "i2c0.h"

// Defines for the log field
#define MAG         1  // 00000001
#define ACCEL       2  // 00000010
#define GYRO        4  // 00000100
#define TEMP        8  // 00001000
#define TIME        16 // 00010000 ... Will always be on
#define LEVELING    20 // 00100000 ... if on, give random value
#define ENCRYPT     24 // 01000000 ... if on, use encryption

#define CURREG      0x0005 // Will hold value of the last register used
#define EEPROM 0xA0  // Add of EEPROM

#define HB(x) (x >> 8) & 0xFF//defines High Byte for reading/writing to EEPROM
#define LB(x) (x) & 0xFF//defines low byte for reading/writing to EEPROM


// Use user input to determine what command to do and call functions. Returns false if run is activated
bool commands()
{
    char input[128];
    USER_INPUT uIn;

    uint8_t temp; // dummy variable used as needed
    uint8_t log = TIME; // Set the log vars to log time

    while(true) // only way to get out are exit or run
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
        else if(!strcmp(uIn.command, "temp"))
        {
            // TODO connect to function that reads and outputs temp
        }
        else if(!strcmp(uIn.command, "reset"))
        {
            // TODO connect to function that resets hardware (Also figure out what that means...)
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
            // TODO connect to a function that will set the number of samples
        }
        else if(!strcmp(uIn.command, "gating"))
        {
            // TODO connect to a function that offers the gating option (not sure this is someting we will use??)
        }
        else if(!strcmp(uIn.command, "hysteresis"))
        {
            // TODO connect to a function that will set the threshold hysteresis for the parameter. Value of 0 turns hysteresis off
        }
        else if(!strcmp(uIn.command, "sleep"))
        {
            // May change this to an int later that goes into a function
            if(!strcmp(uIn.fields, "on"))
            {
                // TODO turn on sleep
            }
            else if(!strcmp(uIn.fields, "off"))
            {
                // TODO turn off sleep
            }
            else
            {
                putsUart0("Not a valid input for sleep. Please try again.\n\n");
            }

        }
        else if(!strcmp(uIn.command, "leveling"))
        {
            // May change this to an int later that goes into a function
            if(!strcmp(uIn.fields, "on"))
            {
                // TODO turn on leveling
            }
            else if(!strcmp(uIn.fields, "off"))
            {
                // TODO turn off leveling
            }
            else
            {
                putsUart0("Not a valid input for sleep. Please try again.\n\n");
            }

        }
        else if(!strcmp(uIn.command, "encrypt"))
        {
            uint32_t key;

            if(!strcmp(uIn.fields, "off"))
            {
                log =
                key = 0;
            }
            else
                key = myatoi(uIn.fields);

            //TODO Connect to a function that will create this encryption (0 means encryption off)
        }
        else if(!strcmp(uIn.command, "periodic"))
        {
            //TODO Connect to a function that will configure periodic time with delay of T
        }
        else if(!strcmp(uIn.command, "trigger"))
        {
            //TODO Connect to a function that will configure trigger mode
        }
        else if(!strcmp(uIn.command, "stop"))
        {
            //TODO Connect to a function that will stop sampling
        }
        else if(!strcmp(uIn.command, "run"))
        {
            // TODO
            // check if time and date were set. If not, notify of default values
            // check if any logs were set. If not, ask user if okay
            // save log vars in EEPROM header and RTCC RAM
            return false; // signify run
        }
        else if(!strcmp(uIn.command, "help"))
        {
            // output list of commands
        }
        else if(!strcmp(uIn.command, "quit"))
        {
            return true; // signify exit...exits program and saves no changes
        }
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
    /*if(log & LEVELING) // leveling in on
        //Write code to get leveling value of next add

    else
    {*/
    uint16_t add = readI2c0Register16(EEPROM >> 1, CURREG); // get upper bits
    add = add<<8;
    uint8_t y = readI2c0Register16(EEPROM >> 1, CURREG+1); // get lower bit
    add += y;

        return add;
    //}
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
