#include "sx126x.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <cstdint>

#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"

#ifndef UINT8_MAX
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
#endif

// ─── Low-level helpers
// ────────────────────────────────────────────────────────

static void cs_low() { gpio_put(PIN_CS, 0); }
static void cs_high() { gpio_put(PIN_CS, 1); }

static void wait_busy() {
    // BUSY is HIGH while the chip is processing; wait until it goes LOW
    uint32_t timeout = 5000;
    while (gpio_get(PIN_BUSY) && timeout--) {
        sleep_us(100);
    }
    if (timeout == 0) {
        printf("[SX126x] BUSY timeout!\n");
    }
}

static void spi_write_cmd(const uint8_t* buf, size_t len) {
    wait_busy();
    cs_low();
    spi_write_blocking(LORA_SPI, buf, len);
    cs_high();
}

static void spi_read_cmd(const uint8_t* cmd, size_t cmd_len, uint8_t* resp,
                         size_t resp_len) {
    wait_busy();
    cs_low();
    spi_write_blocking(LORA_SPI, cmd, cmd_len);
    spi_read_blocking(LORA_SPI, 0x00, resp, resp_len);
    cs_high();
}

// ─── Register & buffer access
// ─────────────────────────────────────────────────

static void write_register(uint16_t addr, uint8_t value) {
    uint8_t buf[4] = {CMD_WRITE_REGISTER, (uint8_t)(addr >> 8),
                      (uint8_t)(addr & 0xFF), value};
    spi_write_cmd(buf, 4);
}

static uint8_t read_register(uint16_t addr) {
    uint8_t cmd[4] = {
        CMD_READ_REGISTER, (uint8_t)(addr >> 8), (uint8_t)(addr & 0xFF),
        0x00  // status byte
    };
    uint8_t resp[1];
    wait_busy();
    cs_low();
    spi_write_blocking(LORA_SPI, cmd, 4);
    spi_read_blocking(LORA_SPI, 0x00, resp, 1);
    cs_high();
    return resp[0];
}

static void set_buffer_base_addr(uint8_t tx_base, uint8_t rx_base) {
    uint8_t buf[3] = {CMD_SET_BUFFER_BASE_ADDR, tx_base, rx_base};
    spi_write_cmd(buf, 3);
}

static void write_buffer(uint8_t offset, const uint8_t* data, uint8_t len) {
    uint8_t buf[2 + 256];
    buf[0] = CMD_WRITE_BUFFER;
    buf[1] = offset;
    memcpy(&buf[2], data, len);
    spi_write_cmd(buf, 2 + len);
}

static void read_buffer(uint8_t offset, uint8_t* data, uint8_t len) {
    uint8_t cmd[3] = {CMD_READ_BUFFER, offset, 0x00};
    // 0x00 = status byte
    wait_busy();
    cs_low();
    spi_write_blocking(LORA_SPI, cmd, 3);
    spi_read_blocking(LORA_SPI, 0x00, data, len);
    cs_high();
}

// ─── Command wrappers
// ─────────────────────────────────────────────────────────

static void set_standby(uint8_t mode) {
    uint8_t buf[2] = {CMD_SET_STANDBY, mode};
    spi_write_cmd(buf, 2);
}

static void set_packet_type(uint8_t type) {
    uint8_t buf[2] = {CMD_SET_PACKET_TYPE, type};
    spi_write_cmd(buf, 2);
}

// Frequency: 915 MHz for US, change to 868000000 for EU
static void set_rf_frequency(uint32_t freq_hz) {
    // PLL step = 32MHz / 2^25
    uint32_t frf = (uint32_t)((double)freq_hz / (double)(32000000) * (1 << 25));
    uint8_t buf[5] = {CMD_SET_RF_FREQUENCY, (uint8_t)(frf >> 24),
                      (uint8_t)(frf >> 16), (uint8_t)(frf >> 8),
                      (uint8_t)(frf)};
    spi_write_cmd(buf, 5);
}

static void set_pa_config() {
    // For SX1262 at +22 dBm: paDutyCycle=0x04, hpMax=0x07, deviceSel=0x00,
    // paLut=0x01
    uint8_t buf[5] = {CMD_SET_PA_CONFIG, 0x04, 0x07, 0x00, 0x01};
    spi_write_cmd(buf, 5);
}

static void set_tx_params(int8_t power, uint8_t ramp_time) {
    // power: -9 to +22 dBm for SX1262
    uint8_t buf[3] = {CMD_SET_TX_PARAMS, (uint8_t)power, ramp_time};
    spi_write_cmd(buf, 3);
}

static void set_modulation_params() {
    // SF7, BW 125kHz, CR 4/5, LDRO off
    // Bandwidth codes: 0x04 = 125 kHz
    // CR codes:        0x01 = 4/5
    uint8_t buf[9] = {
        CMD_SET_MODULATION_PARAMS,
        0x07,  // SF7
        0x04,  // BW 125 kHz
        0x01,  // CR 4/5
        0x00,  // LDRO off
        0x00,
        0x00,
        0x00,
        0x00  // reserved
    };
    spi_write_cmd(buf, 9);
}

static void set_packet_params(uint8_t payload_len) {
    // Preamble 12 symbols, explicit header, payload_len, CRC on, standard IQ
    uint8_t buf[7] = {
        CMD_SET_PACKET_PARAMS,
        0x00,
        0x0C,  // preamble length = 12
        0x00,  // explicit header
        payload_len,
        0x01,  // CRC on
        0x00   // standard IQ
    };
    spi_write_cmd(buf, 7);
}

static void set_dio_irq(uint16_t irq_mask, uint16_t dio1_mask) {
    uint8_t buf[9] = {CMD_SET_DIO_IRQ_PARAMS,
                      (uint8_t)(irq_mask >> 8),
                      (uint8_t)(irq_mask),
                      (uint8_t)(dio1_mask >> 8),
                      (uint8_t)(dio1_mask),
                      0x00,
                      0x00,
                      0x00,
                      0x00};
    spi_write_cmd(buf, 9);
}

static void calibrate_image(uint32_t freq_hz) {
    // Calibration params
    uint8_t f1, f2;
    if (freq_hz >= 868000000) {
        f1 = 0xD7;
        f2 = 0xDB;
    } else if (freq_hz >= 863000000) {
        f1 = 0xD7;
        f2 = 0xDB;
    } else {
        f1 = 0xC1;
        f2 = 0xC5;
    }
    uint8_t buf[3] = {CMD_CALIBRATE_IMAGE, f1, f2};
    spi_write_cmd(buf, 3);
}

void sx126x_init() {
    spi_init(LORA_SPI, LORA_SPI_FREQ);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);

    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    gpio_init(PIN_RESET);
    gpio_set_dir(PIN_RESET, GPIO_OUT);

    gpio_init(PIN_BUSY);
    gpio_set_dir(PIN_BUSY, GPIO_IN);

    gpio_init(PIN_DIO1);
    gpio_set_dir(PIN_DIO1, GPIO_IN);

    // Hard reset
    gpio_put(PIN_RESET, 0);
    sleep_ms(10);
    gpio_put(PIN_RESET, 1);
    sleep_ms(20);

    wait_busy();

    set_standby(STDBY_RC);
    set_packet_type(PACKET_TYPE_LORA);

    uint8_t reg_mode[2] = {CMD_SET_REGULATOR_MODE, 0x01};
    spi_write_cmd(reg_mode, 2);

    calibrate_image(868000000);
    set_rf_frequency(868000000);
    set_pa_config();
    set_tx_params(22, 0x04);
    set_modulation_params();

    // Over-current protection tweak
    write_register(0x08E7, 0x38);

    set_buffer_base_addr(0x00, 0x00);

    printf("[SX126x] Init complete\n");
}

void sx126x_send(const uint8_t* data, uint8_t len) {
    set_standby(STDBY_RC);
    set_packet_params(len);
    set_buffer_base_addr(0x00, 0x00);
    write_buffer(0x00, data, len);

    set_dio_irq(IRQ_TX_DONE | IRQ_TIMEOUT, IRQ_TX_DONE | IRQ_TIMEOUT);

    uint8_t tx_cmd[4] = {CMD_SET_TX, 0x00, 0x00, 0x00};
    spi_write_cmd(tx_cmd, 4);

    printf("[SX126x] TX started (%d bytes)\n", len);

    uint16_t irq = sx126x_get_irq();
    printf("[SX126x] TX IRQ raw=0x%04X (DIO1=%d)\n", irq, gpio_get(PIN_DIO1));
    sx126x_clear_irq(0xFFFF);

    if (irq & IRQ_TX_DONE) {
        printf("[SX126x] TX done\n");
    } else if (irq & IRQ_TIMEOUT) {
        printf("[SX126x] TX timeout\n");
    } else {
        printf("[SX126x] TX IRQ not matched (check masks)\n");
    }
}

void sx126x_start_rx() {
    set_standby(STDBY_RC);
    set_packet_params(255);
    set_buffer_base_addr(0x00, 0x00);

    set_dio_irq(IRQ_RX_DONE | IRQ_TIMEOUT | IRQ_CRC_ERROR,
                IRQ_RX_DONE | IRQ_TIMEOUT | IRQ_CRC_ERROR);

    uint8_t rx_cmd[4] = {CMD_SET_RX, 0xFF, 0xFF, 0xFF};
    spi_write_cmd(rx_cmd, 4);

    printf("[SX126x] RX listening...\n");
}

int sx126x_receive(uint8_t* buf, uint8_t* len) {
    // Always poll IRQ status for maximum debug visibility.
    uint8_t dio1 = gpio_get(PIN_DIO1);

    uint16_t irq = sx126x_get_irq();
    printf("[SX126x] RX IRQ=0x%04X (DIO1=%d)\n", irq, gpio_get(PIN_DIO1));
    sx126x_clear_irq(irq);

    if (irq & IRQ_CRC_ERROR) {
        printf("[SX126x] CRC error\n");
        sx126x_start_rx();
        return -1;
    }

    if (irq & IRQ_TIMEOUT) {
        printf("[SX126x] RX timeout\n");
        sx126x_start_rx();
        return -1;
    }

    if (irq & IRQ_RX_DONE) {
        uint8_t cmd[2] = {CMD_GET_RX_BUFFER_STATUS, 0x00};

        wait_busy();
        cs_low();
        spi_write_blocking(LORA_SPI, cmd, 2);

        uint8_t raw[3];
        spi_read_blocking(LORA_SPI, 0x00, raw, 3);
        cs_high();

        uint8_t payload_len = raw[1];
        uint8_t rx_offset = raw[2];

        if (payload_len > 255) payload_len = 255;

        read_buffer(rx_offset, buf, payload_len);
        *len = payload_len;

        sx126x_start_rx();
        return 1;
    }

    return 0;
}

uint16_t sx126x_get_irq() {
    uint8_t cmd[2] = {CMD_GET_IRQ_STATUS, 0x00};
    uint8_t resp[3];
    wait_busy();
    cs_low();
    spi_write_blocking(LORA_SPI, cmd, 2);
    spi_read_blocking(LORA_SPI, 0x00, resp, 3);
    cs_high();
    return (uint16_t)((resp[1] << 8) | resp[2]);
}

void sx126x_clear_irq(uint16_t mask) {
    uint8_t buf[3] = {CMD_CLR_IRQ_STATUS, (uint8_t)(mask >> 8),
                      (uint8_t)(mask)};
    spi_write_cmd(buf, 3);
}
