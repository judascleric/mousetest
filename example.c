/* Mouse example with debug channel, for Teensy USB Development Board
 * http://www.pjrc.com/teensy/usb_mouse.html
 * Copyright (c) 2009 PJRC.COM, LLC
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "controller/teensy-2-0.h"
#include "controller/n35p112.h"
#include "usb_mouse_debug.h"
#include "print.h"

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#define LED_CONFIG	(DDRD |= (1<<6))
#define LED_ON		(PORTD &= ~(1<<6))
#define LED_OFF		(PORTD |= (1<<6))
#define CPU_PRESCALE(n)	(CLKPR = 0x80, CLKPR = (n))

int main(void)
{
	int8_t x, y;

	teensy_init();

	// Initialize the USB, and then wait for the host to set configuration.
	// If the Teensy is powered without a PC connected to the USB port,
	// this will wait forever.
	usb_init();
	while (!usb_configured()) /* wait */ ;

	// Wait an extra second for the PC's operating system to load drivers
	// and do whatever it does to actually be ready for input
	_delay_ms(3000);

	print("USB Initialized.\n");
	_delay_ms(1000);

	n35p112_init();

	// Vales Measured from working script
	//TWCR = 69(0x45)
	//TWSR = 248(0xF8)
	//TWBR = 72(0x48)
	print("TWCR = ");
	phex(TWCR);
	print(" (45 expected)\n");
	print("TWSR = ");
	phex(TWSR);
	print(" (F8 expected)\n");
	print("TWBR = ");
	phex(TWBR);
	print(" (48 expected)\n");

	//print("teensy_configure_interrupts().\n");
	_delay_ms(100);
	teensy_configure_interrupts();

	print("n35p112_calibrate().\n");
	_delay_ms(100);
	n35p112_calibrate();

	print("Initialized.\n");
	while (1) {
		x = n35p112_get_x();
		y = n35p112_get_y();
		//usb_mouse_move(x, y, 0);
		print("mouse move: x=");
		phex(x);
		print(", y=");
		phex(y);
		print("\n");
		_delay_ms(500);
	}

	// This sequence creates a right click
	//usb_mouse_buttons(0, 0, 1);
	//_delay_ms(10);
	//usb_mouse_buttons(0, 0, 0);
}

