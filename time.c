// time.c
// This file holds the commands for time change from seconds to Day, Month Year H:M:S

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "tm4c123gh6pm.h"
#include "time.h"
#include "string.h"

/*
 * Legend for reading months as numbers:
 * 1   -   Jan
 * 2   -   Feb
 * 3   -   Mar
 * 4   -   Apr
 * 5   -   May
 * 6   -   Jun
 * 7   -   Jul
 * 8   -   Aug
 * 9   -   Sept
 * 10  -   Oct
 * 11  -   Nov
 * 12  -   Dec
 */

#define DAYSEC 86400
#define ADDMON 0x0000
#define ADDDAY 0x0002
#define ADDHR  0x0004
#define ADDMIN 0x0006
#define ADDSEC 0x0008
#define EEPROM 0xA0

#define HB(x) (x >> 8) & 0xFF//defines High Byte for reading/writing to EEPROM
#define LB(x) (x) & 0xFF//defines low byte for reading/writing to EEPROM

// Getters
// Get original date and time given by user
char* getOrigDateTime()
{
    // Get user set date
    // Get user set date

    // return a string in the form: Day, Month H:M:S
}

// Setters
// Take user input of date and save it...return 0 is successful, 1 if invalid input, and 2 if error storing in EEPROM
uint8_t setUDate(char input[])
{
    // Save month as a number from the legend above
    // Change input to sec
    uint32_t i = 0;
    uint8_t numM, numD;
    uint8_t count = 0;
    uint8_t j = 0;
    char month[128], day[128];

    // Parse input
    while(input[i] != '\0') // While the current character of the input does not equal a
    {                                       // terminating character
        if(input[i] >= 32) // check is input is a printable value
        {
            if(input[i] == 32 && count == 0) // month store in complete, move to day
            {
                month[j] = '\0'; // Put null term on month
                j = 0; // reset j for day
                count++; // increase count to show day is complete
                i++; // move i to next spot in input
            }
            else if(count == 0) // Store values in month
            {
                month[j] = input[i];
                j++; // move j to next spot in month
                i++; // move i to next spot in input
            }
            else if(count == 1) // Store values in day
            {
                day[j] = input[i];
                j++; // move j to next spot in day
                i++; // move i to next spot in input
            }
        }
    }

    day[j] = '\0'; // Put null term on day

    numM = monthToNum(month);
    if(numM == 0) // if the above function returned a 0, then the month was invalid
        return 1;

    numD = atoi(day);
    if(numD > monthDay(numM)) // if the day input is not a valid day for the month given, return error msg 1
        return 1;

    uint8_t data1[] = {LB(ADDMON), numM};
    writeI2c0Registers(EEPROM >> 1, HB(ADDMON), data1, 2);
    if(readI2c0Register16(EEPROM >>1, ADDMON) != numM) // If data was not stored correctly, return error msg 2
        return 2;

    uint8_t data2[] = {LB(ADDDAY), numD};
    writeI2c0Registers(EEPROM >> 1, HB(ADDDAY), data2, 2);
    if(readI2c0Register16(EEPROM >>1, ADDDAY) != numD) // If data was not stored correctly, return error msg 2
        return 2;

    return 0; // If not other errors were hit, then the write worked
}

// Take user input of time and save it...return 0 is successful, 1 if invalid input, and 2 if error storing in EEPROM
uint8_t setUTime(char input[])
{
    // Save month as a number from the legend above
    // Change input to sec
    uint32_t i = 0;
    uint8_t numH, numM, numS;
    uint8_t count = 0;
    uint8_t j = 0;
    char hr[128], min[128], sec[128];

    // Parse input
    while(input[i] != '\0') // While the current character of the input does not equal a
    {                                       // terminating character
        if(input[i] >= 32) // check is input is a printable value
        {
            if(input[i] == 32) // space means move to the next field
            {
                if(count == 0) // if hr is complete
                {
                    hr[j] = '\0'; // Put null term on hr
                    j = 0; // reset j for min
                    count++; // increase count to show hr is complete
                    i++; // move i to next spot in input
                }
                else if(count == 1) // if min is complete
                {
                    min[j] = '\0'; // Put null term on min
                    j = 0; // reset j for sec
                    count++; // increase count to show min is complete
                    i++; // move i to next spot in input
                }

            }
            else if(count == 0) // Store values in hr
            {
                hr[j] = input[i];
                j++; // move j to next spot in month
                i++; // move i to next spot in input
            }
            else if(count == 1) // Store values in min
            {
                min[j] = input[i];
                j++; // move j to next spot in day
                i++; // move i to next spot in input
            }
            else if(count == 2)
            {
                sec[j] = input[i];
                j++; // move j to next spot in day
                i++; // move i to next spot in input
            }
        }
    }

    sec[j] = '\0'; // Put null term on day

    numH = atoi(hr);
    if(numH < 0 || numH > 24) // if the above function returned a number below 0 or above 24, then the hr was invalid
        return 1;

    numM = atoi(min);
    if(numH < 0 || numH > 59) // if the above function returned a number below 0 or above 59, then the min was invalid
        return 1;

    numS = atoi(sec);
    if(numH < 0 || numH > 59) // if the above function returned a number below 0 or above 59, then the sec was invalid
        return 1;

    uint8_t data1[] = {LB(ADDHR), numM};
    writeI2c0Registers(EEPROM >> 1, HB(ADDHR), data1, 2);
    if(readI2c0Register16(EEPROM >>1, ADDHR) != numH) // If data was not stored correctly, return error msg 2
        return 2;

    uint8_t data2[] = {LB(ADDMIN), numD};
    writeI2c0Registers(EEPROM >> 1, HB(ADDMIN), data2, 2);
    if(readI2c0Register16(EEPROM >>1, ADDMIN) != numM) // If data was not stored correctly, return error msg 2
        return 2;

    uint8_t data2[] = {LB(ADDSEC), numD};
    writeI2c0Registers(EEPROM >> 1, HB(ADDSEC), data2, 2);
    if(readI2c0Register16(EEPROM >>1, ADDSEC) != numS) // If data was not stored correctly, return error msg 2
        return 2;

    return 0; // If not other errors were hit, then the write worked
}

// Stores time stamp when data is taken
bool setTimeStamp()
{
    return true;
}

// Conversion functions
// Change seconds into Day, Month H:M:S
void secToDateTime(uint32_t sec)
{
    uint32_t daysLeft, secLeft, d, s;
    char* currMonth, currDay, origTime;
    char buffer[20];

    origTime = getOrigDateTime(); // Original time given by the user will be returned in a string in the form: Day, Month H:M:S
    uint8_t month, day;

    daysleft = monthDay(month) - day; // Check the amount of day remaining in the month before the time
                                                  // rolls into the next month
    secLeft = DAYSEC * daysleft; // Check the amt of time left in the month

    // While month subtraction does not cause seconds to go below 0...
    while(sec > 0)
    {
        if(sec > secLeft) // If more time has elapsed than what is left in the month....
        {
            sec -= secLeft; // Remove days in original month elapsed from seconds elapsed

            currMonth = nextMonth(month); // Move to next month since all time in previous month has elapsed
            secLeft = monthSec(month); // Get the amount of seconds in the new month

        }
        else // else, just add days elapsed
        {
            d = sec / DAYSEC; // Get number of days left in the seconds elapsed
            s = sec - (d*DAYSEC); // seconds elapsed - (seconds in the days) = seconds remaining
            sec = 0; // To exit while loop since time is computed
        }
    }

    //TODO write output using itoa
    currDay = itoa(d, buffer, 10);

}

// Change seconds to hours
uint8_t secToHr(uint32_t sec)
{

}

// Change H:M:S to seconds
uint32_t timeToSec(char* time)
{

}

// Change Day, Month to seconds
uint32_t dateToSec(char* date)
{

}

// Change seconds to minutes
uint8_t secToMin(uint32_t sec)
{

}

// Table Storage
// Get number of seconds in a given month
uint8_t monthDay(uint8_t month)
{
    uint8_t day;

    switch(month){
        case 1:
            day = 31;
            break;
        case 2:
            day = 28;
            break;
        case 3:
            day = 31;
            break;
        case 4:
            day = 30;
            break;
        case 5:
            day = 31;
            break;
        case 6:
            day = 30;
            break;
        case 7:
            day = 31;
            break;
        case 8:
            day = 31;
            break;
        case 9:
            day = 30;
            break;
        case 10:
            day = 31;
            break;
        case 11:
            day = 30;
            break;
        case 12:
            day = 31;
            break;
    }

    return day;
}

// Change month name to number of month
uint8_t monthToNum(char* month)
{
    if(!strcmp(month, "January"))
        return 1;
    if(!strcmp(month, "February"))
        return 2;
    if(!strcmp(month, "March"))
        return 3;
    if(!strcmp(month, "April"))
        return 4;
    if(!strcmp(month, "May"))
        return 5;
    if(!strcmp(month, "June"))
        return 6;
    if(!strcmp(month, "July"))
        return 7;
    if(!strcmp(month, "August"))
        return 8;
    if(!strcmp(month, "September"))
        return 9;
    if(!strcmp(month, "October"))
        return 10;
    if(!strcmp(month, "November"))
        return 11;
    if(!strcmp(month, "December"))
        return 12;
    else
        return 0;
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

// output formatting
// Will output date and time for user output
void printDandT(uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec)
{

}
