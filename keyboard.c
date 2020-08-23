/*
 * h9asc
 * Antennas switch controller
 *
 * Created by SQ8KFH on 2017-08-07.
 *
 * Copyright (C) 2017-2020 Kamil Palkowski. All rights reserved.
 */ 

#include "keyboard.h"

uint8_t button_mask = 0xF0;

void
KB_Initalize(void)
{
	DDRC &= ~((1 << PC4) | (1 << PC5) | (1 << PC6) |(1 << PC7));
}

uint8_t
KB_get(void)
{
	static uint16_t key1_lock = 0;
	static uint16_t key2_lock = 0;
	static uint8_t encoder_val = 0;
	
	uint8_t button_press = 0;
	
	if (!key1_lock && !(PINC & (1 << PC4))) {
		button_mask |= KB_OK_BUTTON;
		key1_lock = 100;
	}
	else if (key1_lock && (PINC & (1 << PC4))) {
		--key1_lock;
		if (key1_lock == 0)
			button_press |= KB_OK_BUTTON;
	}
	
	if (!key2_lock && !(PINC & (1 << PC7))) {
		button_mask |= KB_ESC_BUTTON;
		key2_lock = 100;
	}
	else if (key2_lock && (PINC & (1 << PC7))) {
		--key2_lock;
		if (key2_lock == 0)
			button_press |= KB_ESC_BUTTON;
	}
	uint8_t tmp_val = 0;
	if (!(PINC & (1 << PC6))) {
		button_mask |= KB_CCW_BUTTON | KB_CW_BUTTON;
		tmp_val |= KB_CCW_BUTTON;
	}
	if (!(PINC & (1 << PC5))) {
		button_mask |= KB_CCW_BUTTON | KB_CW_BUTTON;
		tmp_val |= (1<<0);
	}
	
	if (encoder_val != tmp_val) {
		if ((tmp_val==1 && encoder_val==3)) { //CCW
			button_press |= KB_CCW_BUTTON;
		}
		else if ((tmp_val==3 && encoder_val==1)) { //CW
			button_press |= KB_CW_BUTTON;
		}
		encoder_val = tmp_val;
	}
	
	if (key1_lock)
		button_press |= KB_OK_BUTTON_FLAG;
	if (key2_lock)
		button_press |= KB_ESC_BUTTON_FLAG;
	
	return button_press & button_mask;
}

void
KB_drop_key(void)
{
	button_mask = 0xF0;
}
