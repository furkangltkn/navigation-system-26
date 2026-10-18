#ifndef INC_ONEWIRE_H_
#define INC_ONEWIRE_H_

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

#define ONEWIRE_GPIO_PORT  GPIOA
#define ONEWIRE_GPIO_PIN   GPIO_PIN_1

void OneWire_Init(void);
uint8_t OneWire_Reset(void);
void OneWire_WriteBit(uint8_t bit);
uint8_t OneWire_ReadBit(void);
void OneWire_WriteByte(uint8_t data);
uint8_t OneWire_ReadByte(void);
uint8_t OneWire_CRC8(const uint8_t *addr, uint8_t len);

// DS18B20 komutları
#define DS18B20_CMD_CONVERTTEMP   0x44
#define DS18B20_CMD_READSCRATCH   0xBE
#define DS18B20_CMD_WRITESCRATCH  0x4E
#define DS18B20_CMD_COPYYSCRATCH  0x48
#define DS18B20_CMD_RECALLEEPROM  0xB8
#define DS18B20_CMD_READPOWER     0xB4
#define DS18B20_CMD_SEARCHROM     0xF0
#define DS18B20_CMD_READROM       0x33
#define DS18B20_CMD_MATCHROM      0x55
#define DS18B20_CMD_SKIPROM       0xCC
#define DS18B20_CMD_ALARMSEARCH   0xEC
// --- ARAMA ALGORİTMASI İÇİN EKLENENLER ---
void OneWire_ResetSearch(void);
uint8_t OneWire_Search(void);
uint8_t OneWire_First(void);
uint8_t OneWire_Next(void);
void OneWire_GetFullROM(uint8_t *rom);
#endif /* INC_ONEWIRE_H_ */
