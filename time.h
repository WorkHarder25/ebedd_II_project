// time.h
// This is the header file for time.c which will change seconds over to a Day, Month, H:M:S

#ifndef TIME_H_
#define TIME_H_

#include <stdint.h>

// Getters
void getOrigDateTime(uint8_t* origDate); // Returns as {Mon, Day, Hr, Min, Sec}

// Setters
// Store user input
uint8_t setUDate(char* input); // Take user input of date and save it
uint8_t setUTime(char* input); // Take user input of time and save it
bool setTimeStamp(uint16_t add); // Stores time stamp when data is taken

// Conversion functions
void secToDateTime(uint32_t sec); // Change seconds to Day, Month H:M:S

// Table Storage
uint8_t monthDay(uint8_t month); // Get number of days in a given month
uint8_t monthToNum(char* month); // Change month name to number of month
void numToMonth(uint8_t month, char* monthS); // Change month number to month name

#endif /* TIME_H_ */
