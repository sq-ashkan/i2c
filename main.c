#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdio.h>
#include "./lcd_text_handler.c"

// TWI Status Macros from Vorlesung
#define TW_STATUS (TWSR & 0xF8)
#define TW_START 0x08
#define TW_REP_START 0x10
#define TW_MT_SLA_ACK 0x18
#define TW_MT_DATA_ACK 0x28
#define TW_MR_SLA_ACK 0x40

// HTU31 I2C Adresse
#define HTU31_ADDRESS 0x40
#define HTU31_READ_SERIAL 0x0A // Correct command for HTU31D

// TWI/I2C Funktionen
void twi_init(void)
{
    TWBR = 0x12; // 100kHz bei 16MHz
    TWSR = 0x00; // Prescaler = 1
    TWDR = 0xFF;
    TWCR = (1 << TWEN); // TWI aktivieren
}

uint8_t twi_start(uint8_t address)
{
    uint8_t twst;

    // Start Condition
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)))
        ;

    // Check Status
    twst = TW_STATUS;
    if ((twst != TW_START) && (twst != TW_REP_START))
        return 1;

    // Send Address
    TWDR = address;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)))
        ;

    // Check Status
    twst = TW_STATUS;
    if ((twst != TW_MT_SLA_ACK) && (twst != TW_MR_SLA_ACK))
        return 1;

    return 0; // Success
}

void twi_write(uint8_t data)
{
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)))
        ;
}

uint8_t twi_read_ack(void)
{
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
    while (!(TWCR & (1 << TWINT)))
        ;
    return TWDR;
}

uint8_t twi_read_nack(void)
{
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)))
        ;
    return TWDR;
}

void twi_stop(void)
{
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
    while (TWCR & (1 << TWSTO))
        ;
}

// HTU31 Serial Number lesen (32-bit)
uint32_t read_htu31_serial_number(void)
{
    uint32_t serial_number = 0;
    uint8_t data[4];

    // Command senden
    twi_start(HTU31_ADDRESS << 1); // Write
    twi_write(HTU31_READ_SERIAL);  // 0x0A command
    twi_stop();

    _delay_ms(10); // Warten für Sensor

    // Serial Number lesen (4 bytes)
    twi_start((HTU31_ADDRESS << 1) | 1); // Read
    data[0] = twi_read_ack();            // MSB
    data[1] = twi_read_ack();
    data[2] = twi_read_ack();
    data[3] = twi_read_nack(); // LSB with NACK
    twi_stop();

    // Combine to 32-bit number
    serial_number = ((uint32_t)data[0] << 24) |
                    ((uint32_t)data[1] << 16) |
                    ((uint32_t)data[2] << 8) |
                    data[3];

    return serial_number;
}

int main(void)
{
    char buffer[32];

    // Initialisierung
    twi_init();
    _delay_ms(100);

    // Serial Number lesen
    uint32_t serial_number = read_htu31_serial_number();

    // Format and display
    sprintf(buffer, "SN: %lu", serial_number);
    lcd_text_handler(buffer);

    while (1)
    {
        // Fertig
    }

    return 0;
}