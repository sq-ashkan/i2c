void lcd_pulse(void)
{
    PORTB |= (1 << PB7); // L→H Enable line (set HIGH)
    _delay_us(20);
    PORTB &= ~(1 << PB7); // H→L Enable line (set LOW) - Data is latched on falling edge
    _delay_us(20);
}

void lcd_string(const char *data)
{
    while (*data != '\0')
    {
        PORTB |= (1 << PB5); // RS = 1 for data
        PORTL = *data++;
        lcd_pulse();
        _delay_us(41);
    }
}

void lcd_text_handler(const char *text)
{
    // *********
    // init part
    // *********
    DDRL = 0xFF;
    DDRB |= (1 << PB5) | // RS = 0 for command | RS = 1 for Data
            (1 << PB6) | // read oder Write | wir brauchen das hier nicht -> immer schreiben wir hier!
            (1 << PB7);  // H→L der Enable-Leitung ; Daten werden übernommen

    _delay_ms(15);

    // Soft Reset - 3 times
    PORTL = 0x30;
    lcd_pulse();
    _delay_ms(5);

    lcd_pulse();
    _delay_ms(1);

    lcd_pulse();
    _delay_ms(1);

    // Function Set
    PORTB &= ~(1 << PB5); // RS = 0 for command
    PORTL = 0x38;         // 4 und 5 -> line number | 3->font size
    lcd_pulse();
    _delay_us(37);

    // Display On
    PORTB &= ~(1 << PB5); // RS = 0 for command
    PORTL = 0x0E;         // 0 1  und 2 -> desplay on undcursor on
    lcd_pulse();
    _delay_us(37);

    // Entry Mode
    PORTB &= ~(1 << PB5); // RS = 0 for command
    PORTL = 0x06;         //  direction of cursor no shift just to right
    lcd_pulse();
    _delay_us(37);

    // Clear Display
    PORTB &= ~(1 << PB5); // RS = 0 for command
    PORTL = 0x01;
    lcd_pulse();
    _delay_ms(2);

    // Write string
    lcd_string(text);
}