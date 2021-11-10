// time.h
// This is the header file for time.c which will change seconds over to a Day, Month, H:M:S

#ifndef TIME_H_
#define TIME_H_

#include <stdint.h>



// Getters - TODO will need to connect to/pass in value of where we are reading from
char* getOrigDateTime(); // Get the original date and time as a string

// Setters
// Store user input
uint8_t setUDate(char* input); // Take user input of date and save it
uint8_t setUTime(char* input); // Take user input of time and save it
bool setTimeStamp(); // Stores time stamp when data is taken

// Conversion functions
void secToDateTime(char* month, char* day, uint32_t sec) // Change seconds to Day, Month H:M:S
uint8_t secToMin(uint32_t sec); // Change seconds to minutes
uint8_t secToHr(uint32_t sec); // Change seconds to hours
uint32_t dateToSec(char* date); // Change Day, Month to seconds
uint32_t timeToSec(char* time); // Change H:M:S to seconds

// Table Storage
uint8_t monthDay(uint8_t month); // Get number of days in a given month
uint8_t monthToNum(char* month); // Change month name to number of month

// itoa and atoi functionalities
char* itoa(uint32_t num, char* buffer, uint8_t base);
void reverse(char* str, uint8_t length);
uint32_t atoi(char* str);

// output formatting
void print(uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec); // Will store time (print output for now tho)

#endif /* TIME_H_ */
