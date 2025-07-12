#define F_CPU 8000000UL
#include <avr/io.h>
#include <avr/interrupt.h>

// Global variables accessible by interrupts
volatile uint8_t light_active = 0;      // Flag to control if light should run
volatile uint8_t pattern = 0x01;        // Current LED pattern (starts with first LED)
volatile uint16_t timer_reload = 15625; // Timer reload value for 1Hz (adjustable speed)

void init_timer1(void)
{
    // Clear Timer1 control registers to start fresh
    TCCR1A = 0;

    // Configure Timer1 in CTC mode with prescaler 1024
    TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10); // CTC mode, prescaler 1024

    // Set compare value for desired timing interval
    OCR1A = timer_reload;

    // Enable Timer1 Compare A interrupt
    TIMSK1 = (1 << OCIE1A);
}

void init_ext_int(void)
{
    // Configure INT2 (PORTD pin 2, S2) for falling edge detection
    EICRA |= (1 << ISC21);  // Set ISC21 = 1
    EICRA &= ~(1 << ISC20); // Clear ISC20 = 0 (falling edge)

    // Configure INT3 (PORTD pin 3, S1) for falling edge detection
    EICRA |= (1 << ISC31);  // Set ISC31 = 1
    EICRA &= ~(1 << ISC30); // Clear ISC30 = 0 (falling edge)

    // Enable both external interrupts
    EIMSK |= (1 << INT2) | (1 << INT3);
}

// Timer1 Compare A interrupt - handles LED timing automatically
ISR(TIMER1_COMPA_vect)
{
    // Only update LEDs if light is active
    if (light_active)
    {
        // Turn on current LED (invert pattern because LEDs are active low)
        PORTC = ~pattern;

        // Shift pattern to next LED position
        pattern <<= 1;

        // Reset pattern to first LED when all 8 LEDs have been used
        if (pattern == 0x00)
        {
            pattern = 0x01;
        }
    }
}

// External interrupt for S1 button - turns light sequence ON
ISR(INT3_vect)
{
    // Set flag to start light sequence
    light_active = 1;
}

// External interrupt for S2 button - turns light sequence OFF
ISR(INT2_vect)
{
    // Clear flag to stop light sequence
    light_active = 0;

    // Immediately turn off all LEDs
    PORTC = 0xFF;
}

int main(void)
{
    // Configure PORTC as output for LEDs
    DDRC = 0xFF;

    // Initialize all LEDs to OFF state (active low, so 0xFF = all off)
    PORTC = 0xFF;

    // Initialize Timer1 for automatic LED timing
    init_timer1();

    // Initialize external interrupts for button handling
    init_ext_int();

    // Enable global interrupts to start interrupt system
    sei();

    // Main loop does nothing - everything is handled by interrupts
    while (1)
    {
        // CPU is free to do other tasks or enter sleep mode
        // All LED control is managed by hardware interrupts
    }

    return 0;
}