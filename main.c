#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdio.h>
#include "./lcd_text_handler.c"
#include "./crc8_checker.c"

// TWI Status Macros from Vorlesung
#define TW_STATUS (TWSR & 0xF8)
#define TW_START 0x08
#define TW_REP_START 0x10

#define TW_MT_SLA_ACK 0x18 // for Adress: Data byte has been transmitted; ACK has been received
#define TW_MR_SLA_ACK 0x40 // for Adress: Slave Address has been transmitted; ACK has been received - page 252 full data sheet

#define TW_MT_DATA_ACK 0x28 // for Data: Data byte has been transmitted; ACK has been received

// HTU31 I2C Adresse
#define HTU31_ADDRESS 0x40
#define HTU31_READ_SERIAL 0x0A // from data sheet page 8

int twi_init(void)
{
    TWBR = 0x12;                                       // Bitrate setzen (100kHz) // Formel von Seite: 16
    TWSR = (0 << TWPS1) | (0 << TWPS0);                // Prescaler-Bits setzen // prescaler 1 means no divider
    TWDR = 0xFF;                                       // Standardinhalt im Datenregister // need to have vlaue when want to send or recieve data
    TWCR = (1 << TWEN) |                               // Enable TWI-Interface ;release TWI pins.
           (0 << TWIE) | (0 << TWINT) |                // Disable Interrupt.
           (0 << TWEA) | (0 << TWSTA) | (0 << TWSTO) | // No Signal requests. / slave mode ACK  / START condition / STOP condition
           (0 << TWWC);
    return 0; // Collision
}

uint8_t twi_start(uint8_t address)
{
    uint8_t twst;

    // Start Condition
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN); // remove interrupt flag - start condition bit sent - TWEN shoudl be set again so that it stays active!
    while (!(TWCR & (1 << TWINT)))
        ;

    // Check Status
    twst = TW_STATUS;
    if ((twst != TW_START) && (twst != TW_REP_START)) // Check START/REP_START
        return 1;

    // Send Address
    TWDR = address;
    TWCR = (1 << TWINT) | (1 << TWEN); // after this line data will be sent TWINT on one!
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
    TWCR = (1 << TWINT) | (1 << TWEN); // remove interrupt flag - TWEN shoudl be set again so that it stays active!
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
    TWCR = (1 << TWINT) | (1 << TWEN) | (TWEA << 0); // TWEA=0 for NACK
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
uint32_t read_htu31_serial_number_with_crc(void)
{
    uint32_t serial_number = 0;
    uint8_t data[4];

    // Command senden
    twi_start(HTU31_ADDRESS << 1); // Write LSB set zero
    twi_write(HTU31_READ_SERIAL);  // 0x0A command
    twi_stop();

    _delay_ms(60); // Warten für Sensor

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

    // Initialize TWI/I2C interface
    twi_init();
    _delay_ms(100);

    // *****
    // Task one: Serial Number lesen
    // *****
    uint32_t full_data = read_htu31_serial_number_with_crc();
    uint32_t serial_number = full_data >> 8; // Extract 24-bit serial number
    uint8_t crc_received = full_data & 0xFF; // Extract CRC byte

    sprintf(buffer, "SN: %lu", serial_number); // Format serial number for display
    lcd_text_handler(buffer);                  // Show on LCD
    _delay_ms(3000);                           // Wait 3 seconds

    // *****
    // Task Two: CRC Check
    // *****
    uint8_t calculated_crc = crc8_calculate_serial(serial_number, 3); // Calculate CRC for 3 bytes
    calculated_crc == crc_received ? lcd_text_handler("CRC OK") : lcd_text_handler("CRC failed");
    _delay_ms(2000); // Wait 2 seconds

    // *****
    // Task Three: Temperature and Humidity Reading
    // *****

    // Send conversion command with OSR=2 for both T and RH
    twi_start(HTU31_ADDRESS << 1); // Start I2C with write mode
    twi_write(0x54);               // Conversion command: 01010100 (OSR_RH=2, OSR_T=2)
    twi_stop();                    // Stop I2C
    _delay_ms(60);                 // Wait for conversion to complete

    // Send read command
    twi_start(HTU31_ADDRESS << 1); // Start I2C with write mode
    twi_write(0x00);               // Read T & RH command
    twi_stop();                    // Stop I2C
    _delay_ms(1000);               // Small delay before reading

    // read command
    twi_start((HTU31_ADDRESS << 1) | 1); // Start I2C with read mode // Read temperature and humidity data

    uint8_t temp_msb = twi_read_ack(); // Temperature high byte
    uint8_t temp_lsb = twi_read_ack(); // Temperature low byte
    uint8_t temp_crc = twi_read_ack(); // Temperature CRC

    uint8_t hum_msb = twi_read_ack();  // Humidity high byte
    uint8_t hum_lsb = twi_read_ack();  // Humidity low byte
    uint8_t hum_crc = twi_read_nack(); // Humidity CRC (last byte with NACK)

    twi_stop(); // Stop I2C

    // Combine bytes to 16-bit values
    uint16_t temp_raw = ((uint16_t)temp_msb << 8) | temp_lsb; // Combine temperature bytes
    uint16_t hum_raw = ((uint16_t)hum_msb << 8) | hum_lsb;    // Combine humidity bytes

    sprintf(buffer, "T_RAW: %u", temp_raw);
    lcd_text_handler(buffer);
    _delay_ms(2000);

    sprintf(buffer, "H_RAW: %u", hum_raw);
    lcd_text_handler(buffer);
    _delay_ms(2000);

    // Check temperature CRC
    uint8_t temp_data[2] = {temp_msb, temp_lsb};
    uint8_t calc_temp_crc = crc8_calculate(temp_data, 2);
    calc_temp_crc == temp_crc ? lcd_text_handler("Temp CRC: OK") : lcd_text_handler("Temp CRC: FAIL");
    _delay_ms(2000);

    // Check humidity CRC
    uint8_t hum_data[2] = {hum_msb, hum_lsb};
    uint8_t calc_hum_crc = crc8_calculate(hum_data, 2);
    calc_hum_crc == hum_crc ? lcd_text_handler("Hum CRC: OK") : lcd_text_handler("Hum CRC: FAIL");
    _delay_ms(2000);

    // // Apply conversion formulas from datasheet
    float temperature = -40.0 + (165.0 * temp_raw / 65535.0); // T = -40 + 165 * ST / (2^16 - 1)
    float humidity = 100.0 * hum_raw / 65535.0;               // RH = 100 * SRH / (2^16 - 1)

    // // Display temperature
    sprintf(buffer, "Temp: %.2f C", temperature); // Format temperature with 2 decimal places
    lcd_text_handler(buffer);                     // Show on LCD
    _delay_ms(2000);                              // Wait 2 seconds

    // // Display humidity
    sprintf(buffer, "Hum: %.2f %%", humidity); // Format humidity with 2 decimal places
    lcd_text_handler(buffer);                  // Show on LCD
    _delay_ms(2000);                           // Wait 2 seconds

    return 0;
}