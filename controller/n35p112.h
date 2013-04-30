// n35p112.h

#ifndef N35P112_H
#define N35P112_H

#include <stdint.h>

// --------------------------------------------------------------------

uint8_t n35p112_init(void);
void n35p112_calibrate(void);
uint8_t n35p112_update(uint8_t elapsedMs);
uint8_t n35p112_get_x(void);
uint8_t n35p112_get_y(void);

#endif //N35P112_H

