// n35p112.c

#include "n35p112.h"
#include "../twi/twi_teensy-2-0.h"

#include "../print.h"

#include <avr/interrupt.h>
#include <util/delay.h>

// ----------------------------------------------------------------------------

#define N35P112_TWI_ADDRESS (0x41 << 1)
#define N35P112_TWI_TIMEOUT_MS 10

#define MATRIX_DEBOUNCE_SCANS (5)
#define DEBOUNCE_MASK (0xFF >> (7 - MATRIX_DEBOUNCE_SCANS))
#define DEBOUNCE_UP (DEBOUNCE_MASK >> 1)
#define DEBOUNCE_DOWN (DEBOUNCE_UP ^ DEBOUNCE_MASK)


const uint8_t REG_SCALEFACTOR = 0x2D;
const uint8_t REG_CONTROL1 = 0x0F;
const uint8_t REG_JOY_X = 0x10;
const uint8_t REG_JOY_Y = 0x11; //also resets the interrupt flag
const uint8_t REG_JOY_X_POSITIVE_THRESHHOLD = 0x12;
const uint8_t REG_JOY_X_NEGATIVE_THRESHHOLD = 0x13;
const uint8_t REG_JOY_Y_POSITIVE_THRESHHOLD = 0x14;
const uint8_t REG_JOY_Y_NEGATIVE_THRESHHOLD = 0x15;

const uint8_t kJoyResetMs = 22;
const uint8_t kMidSensitivityThresh = 100;
const int8_t kMidSensitivity = 8;
const uint8_t kLowSensitivityThresh = 60;
const int8_t kLowEndSensitivity = 12;

// ----------------------------------------------------------------------------

// static data
volatile static int8_t sJoyX = 0;
volatile static int8_t sJoyY = 0;
volatile static int8_t sJoyOffsetX = 0;
volatile static int8_t sJoyOffsetY = 0;
volatile static int8_t sDeadZoneRadius = 0;
volatile static uint8_t sJoyChangeElapsedMs = 0;

static uint8_t sBtn = 0;
static uint8_t sBtnDebounceBuffer = 0;

// static function declarations
void _offset_calibrate(void);
void _set_deadzone (int8_t deadZoneRadius);

uint8_t n35p112_init(void)
{
	uint8_t ret = 0;
	uint8_t twiError;

	//print("n35p112_init()\n");

	// reset high
	PORTD |= (1<<3);
	_delay_ms(1);
	// reset low
	PORTD &=~ (1<<3);
	_delay_ms(1);
	// reset high
	PORTD |= (1<<3);
	_delay_ms(100);

	// Wait for the chip to finish Power On Reset
	uint8_t resetStatus = 0;
	while (resetStatus != 0xF0)// Check the reset has been done
	{
		//print("Reading Reset\n");
		twiError = TWI_ReadPacket(N35P112_TWI_ADDRESS, N35P112_TWI_TIMEOUT_MS, &REG_CONTROL1, 1, &resetStatus, 1);
		if (twiError != TWI_ERROR_NoError)
		{
			print("twiError = ");
			phex(twiError);
			print("\n");
		}
		else
		{
			_delay_ms(100);
		}
		resetStatus &= 0xFE;
		//print("resetStatus = ");
		//phex(resetStatus);
		//print("\n");
	}

	//print("Reset Complete\n");
	//_delay_ms(100);

	// Set the scaling factor for the hall effect sensor for 0.5mm knob travel distance
	const uint8_t scaleFactor = 0x06;
	twiError = TWI_WritePacket(N35P112_TWI_ADDRESS, N35P112_TWI_TIMEOUT_MS, &REG_SCALEFACTOR, 1, &scaleFactor, 1);
	if (twiError != TWI_ERROR_NoError)
	{
		print("twiError = ");
		phex(twiError);
		print("\n");
	}

	//print("n35p112_init() complete\n");

	return ret;
}

void n35p112_calibrate(void)
{
	_offset_calibrate();
	_set_deadzone(15);
}

void n35p112_update(uint8_t elapsedMs)
{
	// If no interrupts were received during the self-timer sample period,
	//  assume the pointer is re-centered
	sJoyChangeElapsedMs += elapsedMs;
	if (sJoyChangeElapsedMs > kJoyResetMs)
	{
		// Is this needed? Do we get interupts every 20ms even if the cursor is
		//  not moving?
		sJoyX = 0;
		sJoyY = 0;
		sJoyChangeElapsedMs = 0;
	}

	// Debounce the switch
	uint8_t btnState = (PINB & (1<<7)) ? 0 : 1;
    sBtnDebounceBuffer = (sBtnDebounceBuffer << 1) | btnState;
    uint8_t debounceState = sBtnDebounceBuffer & DEBOUNCE_MASK;
	sBtn = btnState;
    //if (btnState != sBtn)
	//{ 
	//	//print("btnState = ");
	//	//phex(btnState);
	//	//print(", debounceState = ");
	//	//phex(debounceState);
	//	//print("\n");

	//	if(sBtn)
	//	{
	//		if (debounceState == DEBOUNCE_UP)
	//		{
	//			sBtn = 0;
	//		}
	//	}
	//	else if (debounceState == DEBOUNCE_DOWN)
	//	{
	//		sBtn = 1;
	//	}
	//}
}

int8_t n35p112_get_x(void)
{
	int8_t joyX;
	if (sJoyX > 0)
	{
		if (sJoyX < sDeadZoneRadius)
		{
			joyX = 0;
		}
		else if ((127 - (sJoyX - sDeadZoneRadius)) < sJoyOffsetX)
		{
			joyX = 127;
		}
		else
		{
			joyX = sJoyX - sDeadZoneRadius + sJoyOffsetX;
			if (joyX < 0)
			{
				joyX =0;
			}
			else if(joyX < kLowSensitivityThresh)
			{
				joyX /= kLowEndSensitivity;
			}
			else if(joyX < kMidSensitivityThresh)
			{
				joyX /= kMidSensitivity;
			}
		}
	}
	else
	{
		if (sJoyX > -sDeadZoneRadius)
		{
			joyX = 0;
		}
		else if ((-127 - (sJoyX + sDeadZoneRadius)) > sJoyOffsetX)
		{
			joyX = -127;
		}
		else
		{
			joyX = sJoyX + sDeadZoneRadius + sJoyOffsetX;
			if (joyX > 0)
			{
				joyX = 0;
			}
			else if(joyX > -kLowSensitivityThresh)
			{
				joyX /= kLowEndSensitivity;
			}
			else if(joyX > -kMidSensitivityThresh)
			{
				joyX /= kMidSensitivity;
			}
		}
	}
	return joyX;
}

int8_t n35p112_get_y(void)
{
	int8_t joyY;
	if (sJoyY > 0)
	{
		if (sJoyY < sDeadZoneRadius)
		{
			joyY = 0;
		}
		else if ((127 - (sJoyY - sDeadZoneRadius)) < sJoyOffsetY)
		{
			joyY = 127;
		}
		else
		{
			joyY = sJoyY - sDeadZoneRadius + sJoyOffsetY;
			if (joyY < 0)
			{
				joyY =0;
			}
			else if(joyY < kLowSensitivityThresh)
			{
				joyY /= kLowEndSensitivity;
			}
			else if(joyY < kMidSensitivityThresh)
			{
				joyY /= kMidSensitivity;
			}
		}
	}
	else
	{
		if (sJoyY > -sDeadZoneRadius)
		{
			joyY = 0;
		}
		else if ((-127 - (sJoyY + sDeadZoneRadius)) > sJoyOffsetY)
		{
			joyY = -127;
		}
		else
		{
			joyY = sJoyY + sDeadZoneRadius + sJoyOffsetY;
			if (joyY > 0)
			{
				joyY = 0;
			}
			else if(joyY > -kLowSensitivityThresh)
			{
				joyY /= kLowEndSensitivity;
			}
			else if(joyY > -kMidSensitivityThresh)
			{
				joyY /= kMidSensitivity;
			}
		}
	}
	return joyY;
}

uint8_t n35p112_get_btn(void)
{
	return sBtn;
}

void _offset_calibrate(void)
{
	uint8_t i;
	int8_t x_cal = 0;
	int8_t y_cal = 0;

	// Disable the MCU interrupts
	cli();

	// Set control register to configure the chip. Low Power Mode, 20ms wakeup
	//  time, interrupt enabled, interrupt active after sampling, normal mode.
	//  See N35P112 data sheet p.23
	uint8_t controlValue = 0x00;
	TWI_WritePacket(N35P112_TWI_ADDRESS, N35P112_TWI_TIMEOUT_MS, &REG_CONTROL1, 1, &controlValue, 1);
	_delay_ms(1);

	// Flush an unused Y_reg to reset the interrupt
	uint8_t dummyVal;
	TWI_ReadPacket(N35P112_TWI_ADDRESS, N35P112_TWI_TIMEOUT_MS, &REG_JOY_Y, 1, &dummyVal, 1);

	for (i=0; i<16; i++)// Read 16 times the coordinates and then average
	{
		while (PIND & (1<<2));// Wait until next interrupt (new coordinates)
		uint8_t xRegVal, yRegVal;
		TWI_ReadPacket(N35P112_TWI_ADDRESS, N35P112_TWI_TIMEOUT_MS, &REG_JOY_X, 1, &xRegVal, 1);
		TWI_ReadPacket(N35P112_TWI_ADDRESS, N35P112_TWI_TIMEOUT_MS, &REG_JOY_Y, 1, &yRegVal, 1);
		x_cal += (int8_t) xRegVal;
		y_cal += (int8_t) yRegVal;

	}
	// offset_X and offset_Y are global variables, used for each coordinate
	// readout in the interrupt routine
	sJoyOffsetX = -(x_cal>>4); // Average X: divide by 16
	sJoyOffsetY = -(y_cal>>4); // Average Y: divide by 16

	// Reenable the MCU interrupts
	sei();

	//print("calibrated to xOff = ");
	//phex(sJoyOffsetX);
	//print(", yOff = ");
	//phex(sJoyOffsetY);
	//print("\n");
}

// Set the deadzone. The chip will not generate interrupts if these threshholds
//  are not exceeded.
void _set_deadzone (int8_t deadZoneRadius)
{
	// Disable MCU interrupts
	cli();
	
	sDeadZoneRadius = deadZoneRadius;
	uint8_t xp = sJoyOffsetX + deadZoneRadius; // Xp register
	uint8_t xn = sJoyOffsetX - deadZoneRadius; // Xn register
	uint8_t yp = sJoyOffsetY + deadZoneRadius; // Yp register
	uint8_t yn = sJoyOffsetY - deadZoneRadius; // Yn register
	TWI_WritePacket(N35P112_TWI_ADDRESS, N35P112_TWI_TIMEOUT_MS, &REG_JOY_X_POSITIVE_THRESHHOLD, 1, &xp, 1);
	TWI_WritePacket(N35P112_TWI_ADDRESS, N35P112_TWI_TIMEOUT_MS, &REG_JOY_X_NEGATIVE_THRESHHOLD, 1, &xn, 1);
	TWI_WritePacket(N35P112_TWI_ADDRESS, N35P112_TWI_TIMEOUT_MS, &REG_JOY_Y_POSITIVE_THRESHHOLD, 1, &yp, 1);
	TWI_WritePacket(N35P112_TWI_ADDRESS, N35P112_TWI_TIMEOUT_MS, &REG_JOY_Y_NEGATIVE_THRESHHOLD, 1, &yn, 1);

	// Enable the MCU interrupts
	sei();

	//print("deadzone set to xNeg = ");
	//phex(xn);
	//print(", yNeg = ");
	//phex(yn);
	//print(", xPos = ");
	//phex(xp);
	//print(", yPos = ");
	//phex(yp);
	//print("\n");
}

ISR(INT2_vect)
{
	//int8_t X_temp, Y_temp;

	// Disable MCU interrupts
	cli();

	/* OPTIONAL: If the module is in a slow power mode (e.g. Wakeup mode
	   INT_function=1 with 320ms rate), configure to a higher rate with INTn for new
	   coordinates ready (e.g. INT_function = 0 with 20ms rate) */

	uint8_t xRegVal;
	uint8_t yRegVal;
	TWI_ReadPacket(N35P112_TWI_ADDRESS, N35P112_TWI_TIMEOUT_MS, &REG_JOY_X, 1, &xRegVal, 1);
	TWI_ReadPacket(N35P112_TWI_ADDRESS, N35P112_TWI_TIMEOUT_MS, &REG_JOY_Y, 1, &yRegVal, 1);
	sJoyX = (int8_t)xRegVal;
	sJoyY = (int8_t)yRegVal;
	sJoyChangeElapsedMs = 0;
	// Add the X and Y offset for correct recentering
	//X_temp = x_reg + offset_X;
	//Y_temp = y_reg + offset_Y;
	/* OPTIONAL: If X_temp and Y_temp are near the center since a few interrupts,
	   meaning the knob has been released, the module can be put back in a slow power
	   mode (e.g. Wakeup mode INT_function=1 with 320ms rate) */

	// Clear the interrupt flag
	EIFR |= (1 << INTF2);

	// Enable the MCU interrupts
	sei();
}
