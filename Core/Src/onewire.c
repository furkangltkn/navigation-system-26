#include "onewire.h"
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

// extern uint32_t SystemCoreClock; // Eğer SystemCoreClock tanımlı değil hatası alırsanız bu satırı ekleyin.

// extern uint32_t SystemCoreClock; // Eğer derleyici SystemCoreClock'u bulamazsa bu yorum satırını açın.

static void OneWire_Delay_us(uint32_t us) {
    uint32_t startTick = DWT->CYCCNT;

    // İşlemci hızını dinamik olarak alıp MHz'e çeviriyoruz. (Örn: 168 MHz ise 168)
    uint32_t freq = HAL_RCC_GetHCLKFreq() / 1000000;
    if(freq == 0) freq = 168; // Güvenlik önlemi (0'a bölme/çarpma hatasını engelle)

    uint32_t delayTicks = us * freq;

    while ((DWT->CYCCNT - startTick) < delayTicks) {
        // İstenilen mikrosaniye süresi dolana kadar bekle
    }
}
void OneWire_Init(void) {
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    gpio.Pin = ONEWIRE_GPIO_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_OD; // Open-Drain önemli
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(ONEWIRE_GPIO_PORT, &gpio);
    HAL_GPIO_WritePin(ONEWIRE_GPIO_PORT, ONEWIRE_GPIO_PIN, GPIO_PIN_SET);

    // DWT Unlock (Kilit Açma) ve Başlatma İşlemleri
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    *(volatile uint32_t *)0xE0001FB0 = 0xC5ACCE55; // DWT Lock Access Register kilidini açar
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

uint8_t OneWire_Reset(void) {
    uint8_t presence = 0;

    taskENTER_CRITICAL();   // FreeRTOS güvenli kesinti kapatma

    HAL_GPIO_WritePin(ONEWIRE_GPIO_PORT, ONEWIRE_GPIO_PIN, GPIO_PIN_RESET);
    OneWire_Delay_us(480);
    HAL_GPIO_WritePin(ONEWIRE_GPIO_PORT, ONEWIRE_GPIO_PIN, GPIO_PIN_SET);
    OneWire_Delay_us(70);
    presence = HAL_GPIO_ReadPin(ONEWIRE_GPIO_PORT, ONEWIRE_GPIO_PIN);
    OneWire_Delay_us(410);

    taskEXIT_CRITICAL();    // FreeRTOS güvenli kesinti açma

    return presence;
}
void OneWire_WriteBit(uint8_t bit) {
    taskENTER_CRITICAL();
    if (bit) {
        HAL_GPIO_WritePin(ONEWIRE_GPIO_PORT, ONEWIRE_GPIO_PIN, GPIO_PIN_RESET);
        OneWire_Delay_us(2);
        HAL_GPIO_WritePin(ONEWIRE_GPIO_PORT, ONEWIRE_GPIO_PIN, GPIO_PIN_SET);
        OneWire_Delay_us(60);
    } else {
        HAL_GPIO_WritePin(ONEWIRE_GPIO_PORT, ONEWIRE_GPIO_PIN, GPIO_PIN_RESET);
        OneWire_Delay_us(60);
        HAL_GPIO_WritePin(ONEWIRE_GPIO_PORT, ONEWIRE_GPIO_PIN, GPIO_PIN_SET);
        OneWire_Delay_us(2);
    }
    taskEXIT_CRITICAL();
}
uint8_t OneWire_ReadBit(void) {
    uint8_t bit = 0;
    taskENTER_CRITICAL();
    HAL_GPIO_WritePin(ONEWIRE_GPIO_PORT, ONEWIRE_GPIO_PIN, GPIO_PIN_RESET);
    OneWire_Delay_us(2);
    HAL_GPIO_WritePin(ONEWIRE_GPIO_PORT, ONEWIRE_GPIO_PIN, GPIO_PIN_SET);
    OneWire_Delay_us(10);
    bit = HAL_GPIO_ReadPin(ONEWIRE_GPIO_PORT, ONEWIRE_GPIO_PIN);
    OneWire_Delay_us(50);
    taskEXIT_CRITICAL();
    return bit;
}

void OneWire_WriteByte(uint8_t data) {
    for (uint8_t i = 0; i < 8; i++) {
        OneWire_WriteBit(data & 0x01);
        data >>= 1;
    }
}

uint8_t OneWire_ReadByte(void) {
    uint8_t data = 0;
    for (uint8_t i = 0; i < 8; i++) {
        data >>= 1;
        if (OneWire_ReadBit()) {
            data |= 0x80;
        }
    }
    return data;
}

uint8_t OneWire_CRC8(const uint8_t *addr, uint8_t len) {
    uint8_t crc = 0;
    while (len--) {
        uint8_t inbyte = *addr++;
        for (uint8_t i = 8; i; i--) {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) crc ^= 0x8C;
            inbyte >>= 1;
        }
    }
    return crc;
}
// =========================================================================
//                   1-WIRE SEARCH ROM ALGORİTMASI EKLENTİSİ
// =========================================================================

static uint8_t ROM_NO[8];
static uint8_t LastDiscrepancy;
static uint8_t LastFamilyDiscrepancy;
static uint8_t LastDeviceFlag;

void OneWire_ResetSearch(void) {
    LastDiscrepancy = 0;
    LastDeviceFlag = false;
    LastFamilyDiscrepancy = 0;
    for(int i = 0; i < 8; i++) ROM_NO[i] = 0;
}

uint8_t OneWire_Search(void) {
    uint8_t id_bit_number = 1;
    uint8_t last_zero = 0;
    uint8_t rom_byte_number = 0;
    uint8_t search_result = 0;
    uint8_t id_bit, cmp_id_bit, search_direction;

    if (!LastDeviceFlag) {
        if (OneWire_Reset() != 0) {
            OneWire_ResetSearch();
            return 0; // Cihaz bulunamadı
        }

        OneWire_WriteByte(DS18B20_CMD_SEARCHROM); // 0xF0 Komutu

        do {
            id_bit = OneWire_ReadBit();
            cmp_id_bit = OneWire_ReadBit();

            if ((id_bit == 1) && (cmp_id_bit == 1)) {
                break; // Hata: Cihaz yok
            } else {
                if (id_bit != cmp_id_bit) {
                    search_direction = id_bit;
                } else {
                    if (id_bit_number < LastDiscrepancy) {
                        search_direction = ((ROM_NO[rom_byte_number] & (1 << ((id_bit_number - 1) % 8))) > 0);
                    } else {
                        search_direction = (id_bit_number == LastDiscrepancy);
                    }
                    if (search_direction == 0) {
                        last_zero = id_bit_number;
                        if (last_zero < 9) LastFamilyDiscrepancy = last_zero;
                    }
                }

                if (search_direction == 1)
                    ROM_NO[rom_byte_number] |= (1 << ((id_bit_number - 1) % 8));
                else
                    ROM_NO[rom_byte_number] &= ~(1 << ((id_bit_number - 1) % 8));

                OneWire_WriteBit(search_direction);
                id_bit_number++;
                rom_byte_number = (id_bit_number - 1) / 8;
            }
        } while (rom_byte_number < 8);

        if (!(id_bit_number < 65)) {
            LastDiscrepancy = last_zero;
            if (LastDiscrepancy == 0) LastDeviceFlag = true;
            search_result = true;
        }
    }

    if (!search_result || !ROM_NO[0]) {
        OneWire_ResetSearch();
        search_result = false;
    }
    return search_result;
}

uint8_t OneWire_First(void) {
    OneWire_ResetSearch();
    return OneWire_Search();
}

uint8_t OneWire_Next(void) {
    return OneWire_Search();
}

void OneWire_GetFullROM(uint8_t *rom) {
    for(int i = 0; i < 8; i++) rom[i] = ROM_NO[i];
}
