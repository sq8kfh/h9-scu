#include "logic.h"
#include "can/can.h"

#include <avr/io.h>

uint16_t switch_node_id = 32;          // reg #11
uint8_t number_of_antennas = 8;       // reg #12
uint16_t power_switch_node_id = 16;    // reg #13

uint8_t tx_antenna = 0;
uint8_t rx_antenna = 0;
uint8_t antenna_split = 0;
uint8_t active_antenna = 1;

uint8_t power_switch = 0;

#define LED_PORT PORTD
#define LED_RED_PIN PD1

#define PTT_INH_PORT PORTC
#define PTT_INH_PIN PC1

void set_tx_inh(void) {
    PTT_INH_PORT |= (1 << PTT_INH_PIN);
    LED_PORT |= (1 << LED_RED_PIN);
}

void cli_tx_inh(void) {
    if (active_antenna == 1 && tx_antenna) {
        PTT_INH_PORT &= ~(1 << PTT_INH_PIN);
        LED_PORT &= ~(1 << LED_RED_PIN);
    }
}

void logic_init(void) {
    DDRC &= ~(1 << PC0); //ptt_in
    PORTC |= (1 << PC0);

    set_tx_inh();
    PORTD |= (1 << PD7); //~PTT_BP RELAY
}

void check_power_switch(void) {
    h9msg_t tmp;
    CAN_init_new_msg(&tmp);
    tmp.type = H9MSG_TYPE_GET_REG;
    tmp.destination_id = power_switch_node_id;
    tmp.dlc = 1;
    tmp.data[0] = 0x0a;
    CAN_put_msg(&tmp);
}

void power_switch_on(void) {
    h9msg_t tmp;
    CAN_init_new_msg(&tmp);
    tmp.type = H9MSG_TYPE_SET_REG;
    tmp.destination_id = power_switch_node_id;
    tmp.dlc = 2;
    tmp.data[0] = 0x0a;
    tmp.data[1] = 1;
    CAN_put_msg(&tmp);
}

void power_switch_off(void) {
    h9msg_t tmp;
    CAN_init_new_msg(&tmp);
    tmp.type = H9MSG_TYPE_SET_REG;
    tmp.destination_id = power_switch_node_id;
    tmp.dlc = 2;
    tmp.data[0] = 0x0a;
    tmp.data[1] = 0;
    CAN_put_msg(&tmp);
}

void switch_anttena(uint8_t ant) {
    set_tx_inh();
    h9msg_t tmp;
    CAN_init_new_msg(&tmp);
    tmp.type = H9MSG_TYPE_SET_REG;
    tmp.destination_id = switch_node_id;
    tmp.dlc = 2;
    tmp.data[0] = 0x0a;
    tmp.data[1] = ant;
    CAN_put_msg(&tmp);
}

void switch_rxtx(void) {
    active_antenna = active_antenna == 1 ? 2 : 1;
    if (active_antenna == 1) {
        switch_anttena(tx_antenna);
        antenna_split = 0;
    }
    else if (active_antenna == 2) {
        switch_anttena(rx_antenna);
        antenna_split = 1;
    }
}

void check_antenna(void) {
    h9msg_t tmp;
    CAN_init_new_msg(&tmp);
    tmp.type = H9MSG_TYPE_GET_REG;
    tmp.destination_id = switch_node_id;
    tmp.dlc = 1;
    tmp.data[0] = 0x0a;
    CAN_put_msg(&tmp);
}

void process_power_switch_msg(h9msg_t *cm) {
    if (cm->dlc > 1 && cm->data[0] == 0x0a && (cm->type & 0xFC) == H9MSG_TYPE_REG_EXTERNALLY_CHANGED) { // match: H9_TYPE_REG: EXTERNALLY_CHANGED INTERNALLY_CHANGED VALUE_BROADCAST VALUE
        power_switch = cm->data[1];
    }
    else if (cm->source_id == switch_node_id && cm->type == H9MSG_TYPE_NODE_TURNED_ON) {
        check_power_switch();
    }
}

void process_antenna_switch_msg(h9msg_t *cm) {
    if (cm->dlc > 1 && cm->data[0] == 0x0a && (cm->type & 0xFC) == H9MSG_TYPE_REG_EXTERNALLY_CHANGED) { // match: H9_TYPE_REG: EXTERNALLY_CHANGED INTERNALLY_CHANGED VALUE_BROADCAST VALUE
        if (active_antenna == 1)
            tx_antenna = cm->data[1];
        else if (active_antenna == 2)
            rx_antenna = cm->data[1];
        cli_tx_inh();
    }
    else if (cm->source_id == switch_node_id && cm->type == H9MSG_TYPE_NODE_TURNED_ON) {
        check_antenna();
    }
}

void process_ptt(void) {
    static uint8_t ptt_key_lock = 0;
    if (ptt_key_lock < 100 && (PINC & (1 << PC0))) {
        ++ptt_key_lock;
    }
    else if (ptt_key_lock == 100 && (PINC & (1 << PC0))) {
        ++ptt_key_lock;
        if (antenna_split == 1 && active_antenna == 2) {
            switch_anttena(tx_antenna);
            active_antenna = 1;
        }
    }
    else if (100 < ptt_key_lock && ptt_key_lock < 200 && !(PINC & (1 << PC0))) {
        ++ptt_key_lock;
    }
    else if (ptt_key_lock == 200 && !(PINC & (1 << PC0))) {
        if (antenna_split == 1 && active_antenna == 1) {
            switch_anttena(rx_antenna);
            active_antenna = 2;
        }
        ptt_key_lock = 0;
    }
}
