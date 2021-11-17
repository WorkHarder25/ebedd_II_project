// utility.c
// This files will the Uart utility as a user interface

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "tm4c123gh6pm.h"
#include "utility.h"

// Use user input to determine what command to do and call functions
void commands()
{
    char input[128];

    getInput(input);


}

// Get user input
void getInput(char* buffer)
{

}

// Parse user input
void parseCommand(char* input)
{

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

