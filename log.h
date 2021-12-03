// log.h

#ifndef LOG_H_
#define LOG_H_

// EEPROM storage
bool storeEEPROMdata(uint16_t add, uint32_t data);
uint32_t readEEPROM32(uint16_t add);

// log functions
bool logMag(uint16_t add);
bool logAcc(uint16_t add);
bool logGyro(uint16_t add);
bool logTemp(uint16_t add);
bool logTime(uint16_t add, uint32_t* time);

// encryption
uint32_t encrypt(uint32_t value);
uint32_t decrypt(uint32_t value);

// take measurments
bool record(uint32_t* time, uint16_t add);

#endif /* LOG_H_ */
