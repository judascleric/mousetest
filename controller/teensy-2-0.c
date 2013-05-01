//teensy-2-0.c

#include "teensy-2-0.h"

#include "../twi/twi_teensy-2-0.h"

#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>

// ----------------------------------------------------------------------------

// processor frequency (from <http://www.pjrc.com/teensy/prescaler.html>)
#define  CPU_PRESCALE(n)  (CLKPR = 0x80, CLKPR = (n))
#define  CPU_16MHz        0x00
#define  CPU_8MHz         0x01
#define  CPU_4MHz         0x02
#define  CPU_2MHz         0x03
#define  CPU_1MHz         0x04
#define  CPU_500kHz       0x05
#define  CPU_250kHz       0x06
#define  CPU_125kHz       0x07
#define  CPU_62kHz        0x08

#define TWI_FREQ 400000UL

// ----------------------------------------------------------------------------

volatile static uint8_t sElapsedMs = 0;

// ----------------------------------------------------------------------------

/* returns
 * - success: 0
 */
uint8_t teensy_init(void) {
	// CPU speed : should match F_CPU in makefile
	#if F_CPU != 16000000
		#error "Expecting different CPU frequency"
	#endif
	CPU_PRESCALE(CPU_16MHz);

	// PD2 as interrupt for N35P112
	DDRD &=~ (1 << 2); //Input
	PORTD |= (1 << 2); //Use Pullup

	// PD3 as reset for N35P112
	DDRD |= (1 << 3); //Output

	// PB7 as pushbutton for N35P112
	DDRB &=~ (1 << 7); //Input
	PORTB &=~ (1 << 7); //No Pullup

	// I2C (TWI)
	uint8_t twiPrescaler = TWI_BIT_PRESCALE_1;
	uint8_t twiBitRate = TWI_BITLENGTH_FROM_FREQ(1, TWI_FREQ);
	TWI_Init(twiPrescaler, twiBitRate);

	return 0;  // success
}

uint8_t teensy_configure_interrupts(void)
{
	cli();

	// PD2 as interrupt for N35P112
	EIFR |= (1 << INTF2);
	EIMSK |= (1 << INT2);

	// enable timer0 interrupt
	TCCR0A = 0x00; // Normal Counter mode
	TCCR0B |= (1 << CS01) | (1<<CS00); // Prescaler = 64: 64 * 1/16,000,000(F_CPU) = 0.000004 s = 4 us
	TCNT0 = 6; // Preload timer count at 6, which will overflow in 250 cycles. 4us*250 = 1000us = 1ms
	TIMSK0 |= 0x01; //Enable timer 0

	sei();
}

uint8_t teensy_get_elapsed_ms(void)
{
	return sElapsedMs;
}

ISR(TIMER0_OVF_vect)
{  
	cli();

	TCNT0=6;
	++sElapsedMs;

	sei();
}

