#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include "./lcd_enable_pulse.c" // lcd_enable_pulse()

int main(void)
{
    DDRL = 0xFF;
    DDRB = 0xE0;

    _delay_ms(50);

    PORTL =  
}