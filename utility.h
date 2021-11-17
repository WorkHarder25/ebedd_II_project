// utility.h
// This files will the Uart utility as a user interface

#ifndef UTILITY_H_
#define UTILITY_H_

void commands(); // Use user input to determine what command to do and call functions
void getInput(char* buffer); // Get user input
void parseCommand(char* input); // Parse user input

// itoa and atoi functionalities
void itoa(int32_t num, char* buffer, uint8_t base);
void reverse(char* str, uint8_t length);
uint32_t myatoi(char* str);

#endif /* UTILITY_H_ */
