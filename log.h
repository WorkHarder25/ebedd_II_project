/*
 * log.h
 *
 *  Created on: Nov 29, 2021
 *      Author: Arcan
 */

#ifndef LOG_H_
#define LOG_H_

#include <stdint.h>
#include <stdbool.h>

void initAdc0Ss3fortemperature();
void initLog();
bool storeEEPROMdata(uint32_t data);
uint8_t readI2c0Register16(uint8_t add, uint16_t reg);
bool logGyro(void);
bool logMag(void);
bool logAcc(void);
bool logTemp(void);
uint16_t getNextAdd();



#endif /* LOG_H_ */
