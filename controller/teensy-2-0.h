//teensy-2-0.h

#ifndef TEENSY_2_0_H
#define TEENSY_2_0_H

#include <stdint.h>

// --------------------------------------------------------------------

uint8_t teensy_init(void);
uint8_t teensy_configure_interrupts(void);
uint8_t teensy_get_elapsed_ms(void);

#endif //TEENSY_2_0_H

