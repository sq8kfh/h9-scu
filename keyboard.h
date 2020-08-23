/*
 * h9asc
 * Antennas switch controller
 *
 * Created by SQ8KFH on 2017-08-07.
 *
 * Copyright (C) 2017-2020 Kamil Palkowski. All rights reserved.
 */ 

#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include <avr/io.h>

#define KB_OK_BUTTON (1 << 3)
#define KB_ESC_BUTTON (1 << 2)
#define KB_CW_BUTTON (1 << 0)
#define KB_CCW_BUTTON (1 << 1)
#define KB_OK_BUTTON_FLAG (1 << 5)
#define KB_ESC_BUTTON_FLAG (1 << 4)

void KB_Initalize(void);
uint8_t KB_get(void);
void KB_drop_key(void);

#endif /* KEYBOARD_H_ */
