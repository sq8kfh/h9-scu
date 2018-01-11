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
extern uint8_t power_switch;

void check_power_switch(void);
void power_switch_on(void);
void power_switch_off(void);

void check_antenna(void);

void switch_anttena(uint8_t ant);

void process_power_switch_msg(h9msg_t *cm);
void process_antenna_switch_msg(h9msg_t *cm);

#endif //_LOGIC_H_
