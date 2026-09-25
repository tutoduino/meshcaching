/*
 * variant.h - LilyGo T-Echo (nRF52840 + SX1262 + 1.54" e-ink)
 * Taken from the MeshCore firmware (variants/lilygo_techo, MIT license),
 * itself derived from Seeed's template. Trimmed to what this project uses.
 */

#pragma once

#include "WVariant.h"

////////////////////////////////////////////////////////////////////////////////
// Low frequency clock source

#define USE_LFXO    // 32.768 kHz crystal oscillator
#define VARIANT_MCK (64000000ul)

#define WIRE_INTERFACES_COUNT   (1)

////////////////////////////////////////////////////////////////////////////////
// Power

#define PIN_PWR_EN              (12)   // rail of every peripheral (e-ink, LoRa, GPS, sensors)

#define BATTERY_PIN             (4)
#define ADC_MULTIPLIER          (4.90F)

#define ADC_RESOLUTION          (14)
#define BATTERY_SENSE_RES       (12)

#define AREF_VOLTAGE            (3.0)

////////////////////////////////////////////////////////////////////////////////
// Number of pins

#define PINS_COUNT              (48)
#define NUM_DIGITAL_PINS        (48)
#define NUM_ANALOG_INPUTS       (1)
#define NUM_ANALOG_OUTPUTS      (0)

////////////////////////////////////////////////////////////////////////////////
// UART pin definition (GPS)

#define PIN_SERIAL1_RX          PIN_GPS_TX
#define PIN_SERIAL1_TX          PIN_GPS_RX

////////////////////////////////////////////////////////////////////////////////
// I2C pin definition

#define PIN_WIRE_SDA            (26)             // P0.26
#define PIN_WIRE_SCL            (27)             // P0.27

////////////////////////////////////////////////////////////////////////////////
// SPI pin definition - SPI is the radio's bus, SPI1 the e-ink's

#define SPI_INTERFACES_COUNT    (2)

#define PIN_SPI_MISO            (23)
#define PIN_SPI_MOSI            (22)
#define PIN_SPI_SCK             (19)
#define PIN_SPI_NSS             (24)

#define PIN_SPI1_MISO           (38)
#define PIN_SPI1_MOSI           (29)
#define PIN_SPI1_SCK            (31)

// Adafruit-variant convention; GxEPD2 references these globals in a
// panel driver this project does not even use (GxEPD2_1248).
static const uint8_t MISO = PIN_SPI_MISO;
static const uint8_t MOSI = PIN_SPI_MOSI;
static const uint8_t SCK  = PIN_SPI_SCK;

////////////////////////////////////////////////////////////////////////////////
// QSPI FLASH

#define PIN_QSPI_SCK            (46)
#define PIN_QSPI_CS             (47)
#define PIN_QSPI_IO0            (44)
#define PIN_QSPI_IO1            (45)
#define PIN_QSPI_IO2            (7)
#define PIN_QSPI_IO3            (5)

#define EXTERNAL_FLASH_DEVICES MX25R1635F
#define EXTERNAL_FLASH_USE_QSPI

////////////////////////////////////////////////////////////////////////////////
// Builtin LEDs (active LOW)

#define LED_RED                 (13)
#define LED_BLUE                (14)
#define LED_GREEN               (15)

#define LED_BUILTIN             (-1)
#define LED_PIN                 LED_BUILTIN
#define LED_STATE_ON            LOW

////////////////////////////////////////////////////////////////////////////////
// Builtin buttons

#define PIN_BUTTON1             (42)   // user button, to ground when pressed
#define BUTTON_PIN              PIN_BUTTON1
#define PIN_USER_BTN            BUTTON_PIN

#define PIN_TOUCH               (11)   // capacitive touch pad (TTP223), HIGH when touched

////////////////////////////////////////////////////////////////////////////////
// LoRa

#define USE_SX1262
#define LORA_CS                 (24)
#define SX126X_DIO1             (20)
#define SX126X_BUSY             (17)
#define SX126X_RESET            (25)
#define SX126X_DIO2_AS_RF_SWITCH
#define SX126X_DIO3_TCXO_VOLTAGE 1.8
// P0.21 is driven by the SX1262 (DIO3, TCXO supply): never drive it from
// the MCU.

////////////////////////////////////////////////////////////////////////////////
// Display (GDEH0154D67 / DEPG0150BN, 200x200, on SPI1)

#define DISP_MISO               (38)
#define DISP_MOSI               (29)
#define DISP_SCLK               (31)
#define DISP_CS                 (30)
#define DISP_DC                 (28)
#define DISP_RST                (2)
#define DISP_BUSY               (3)
#define DISP_BACKLIGHT          (43)   // HIGH = backlight on

////////////////////////////////////////////////////////////////////////////////
// GPS (L76K, unused by this firmware)

#define PIN_GPS_RX              (40)
#define PIN_GPS_TX              (41)
#define GPS_EN                  (34)   // standby: LOW lets the GPS sleep
#define PIN_GPS_RESET           (37)
#define PIN_GPS_PPS             (36)
