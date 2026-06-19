#pragma once
#include <stdint.h>
#include <stddef.h>

// SPI & Pin config
#define LORA_SPI        spi1
#define LORA_SPI_FREQ   1000000

#define PIN_MOSI        11
#define PIN_MISO        12
#define PIN_SCK         10
#define PIN_CS          3
#define PIN_RESET       15
#define PIN_BUSY        2
#define PIN_DIO1        20
#define PIN_BUTTON      4

// SX126x Commands
#define CMD_SET_SLEEP               0x84
#define CMD_SET_STANDBY             0x80
#define CMD_SET_FS                  0xC1
#define CMD_SET_TX                  0x83
#define CMD_SET_RX                  0x82
#define CMD_SET_PACKET_TYPE         0x8A
#define CMD_SET_RF_FREQUENCY        0x86
#define CMD_SET_TX_PARAMS           0x8E
#define CMD_SET_MODULATION_PARAMS   0x8B
#define CMD_SET_PACKET_PARAMS       0x8C
#define CMD_SET_DIO_IRQ_PARAMS      0x08
#define CMD_GET_IRQ_STATUS          0x12
#define CMD_CLR_IRQ_STATUS          0x02
#define CMD_WRITE_BUFFER            0x0E
#define CMD_READ_BUFFER             0x1E
#define CMD_SET_BUFFER_BASE_ADDR    0x8F
#define CMD_GET_RX_BUFFER_STATUS    0x13
#define CMD_SET_PA_CONFIG           0x95
#define CMD_SET_REGULATOR_MODE      0x96
#define CMD_CALIBRATE_IMAGE         0x98
#define CMD_WRITE_REGISTER          0x0D
#define CMD_READ_REGISTER           0x1D

// IRQ Masks (SX126x)
// NOTE: these must match the SX126x datasheet IRQ mask bit positions.
// Correct values depend on the SX126x IRQ mask table.
// If RX/TX still doesn’t work after this, the masks must be corrected.
#define IRQ_TX_DONE                 0x0004
#define IRQ_RX_DONE                 0x0002
#define IRQ_CRC_ERROR               0x0040
#define IRQ_TIMEOUT                 0x0400

// Packet type
#define PACKET_TYPE_LORA            0x01

// Standby modes
#define STDBY_RC                    0x00
#define STDBY_XOSC                  0x01

void     sx126x_init();
void     sx126x_send(const uint8_t* data, uint8_t len);
int      sx126x_receive(uint8_t* buf, uint8_t* len);
void     sx126x_start_rx();
uint16_t sx126x_get_irq();
void     sx126x_clear_irq(uint16_t mask);
