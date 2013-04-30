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

// --- helpers
#define  SET    |=
#define  CLEAR  &=~

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
	DDRD CLEAR (1 << 2); //Input
	PORTD SET (1 << 2); //Use Pullup

	// PD3 as reset for N35P112
	DDRD SET (1 << 3); //Output

	// I2C (TWI)
	uint8_t twiPrescaler = TWI_BIT_PRESCALE_1;
	uint8_t twiBitRate = TWI_BITLENGTH_FROM_FREQ(1, TWI_FREQ);
	TWI_Init(twiPrescaler, twiBitRate);

	return 0;  // success
}

uint8_t teensy_configure_interrupts(void)
{
	// PD2 as interrupt for N35P112
	EIFR SET (1 << INTF2);
	EIMSK SET (1 << INT2);

	sei();
}

