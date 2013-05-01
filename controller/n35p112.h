// n35p112.h

#ifndef N35P112_H
#define N35P112_H

#include <stdint.h>

// --------------------------------------------------------------------

uint8_t n35p112_init(void);
void n35p112_calibrate(void);
void n35p112_update(uint8_t elapsedMs);
int8_t n35p112_get_x(void);
int8_t n35p112_get_y(void);
uint8_t n35p112_get_btn(void);

#endif //N35P112_H

