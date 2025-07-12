#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>

void init_adc(void)
{
    // Reference voltage = AVCC (REFS1=0, REFS0=1)
    ADMUX |= (1 << REFS0);
    ADMUX &= ~(1 << REFS1);

    // Select ADC0 (PORTF.0) - Clear all MUX bits
    ADMUX &= ~((1 << MUX0) | (1 << MUX1) | (1 << MUX2) | (1 << MUX3) | (1 << MUX4));
    ADCSRB &= ~(1 << MUX5);

    // Enable ADC
    ADCSRA |= (1 << ADEN);

    // Set prescaler = 64 (for 8MHz -> 125kHz ADC clock)
    ADCSRA &= ~(1 << ADPS0);
    ADCSRA |= (1 << ADPS1) | (1 << ADPS2);
}

uint16_t read_adc(void)
{
    // Start conversion
    ADCSRA |= (1 << ADSC);

    // Wait for conversion to complete
    while (ADCSRA & (1 << ADSC))
        ;

    // Return ADC result (0-1023)
    return ADCW;
}

void variable_delay(uint16_t adc_value) {
    switch(adc_value >> 8) {  // Divide by 256 using bit shift
        case 0: _delay_ms(100); break;  // Fast
        case 1: _delay_ms(300); break;  // Medium
        case 2: _delay_ms(600); break;  // Slow
        default: _delay_ms(900); break; // Very slow
    }
}

int main(void)
{
    DDRC = 0xFF;
    PORTC = 0xFF;
    init_adc(); // Initialize ADC
    uint8_t pattern = 0x01;
    while (1)
    {
        uint16_t pot_value = read_adc(); // Read potentiometer
        PORTC = ~pattern;          // Turn on current LED
        variable_delay(pot_value); // Variable delay based on pot
        pattern <<= 1; // Shift to next LED position
        if (pattern == 0x00)
        {
            pattern = 0x01; // Reset to first LED
        }
        PORTC = 0xFF; // Turn off all LEDs when button released
    }
    return 0;
}