/*
 * h9asc
 * Antennas switch controller
 *
 * Created by SQ8KFH on 2017-08-07.
 *
 * Copyright (C) 2017-2020 Kamil Palkowski. All rights reserved.
 */

#ifndef _LOGIC_H_
#define _LOGIC_H_

#include <avr/io.h>
#include "can/h9msg.h"


extern uint16_t switch_node_id;        // reg #11
extern uint8_t number_of_antennas;     // reg #12
extern uint16_t power_switch_node_id;  // reg #13

extern uint8_t tx_antenna;
extern uint8_t rx_antenna;
extern uint8_t active_antenna;
extern uint8_t antenna_split;
extern uint8_t power_switch;

void logic_init(void);

void check_power_switch(void);
void power_switch_on(void);
void power_switch_off(void);

void check_antenna(void);

void switch_anttena(uint8_t ant);
void switch_rxtx(void);

void process_power_switch_msg(h9msg_t *cm);
void process_antenna_switch_msg(h9msg_t *cm);

void process_ptt(void);

void save_variables_in_eeprom(void);
void set_switch_id(uint16_t old_id);
void set_power_switch_id(uint16_t old_id);
void set_number_of_antennas(uint8_t old_ant);

#endif //_LOGIC_H_
