/*
 * h9scu.c
 *
 * Created: 2017-08-04 14:28:43
 * Author : SQ8KFH
 */ 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

#include "HD44780.h"
#include "keyboard.h"
#include "can/can.h"
#include "logic.h"

#define FLAG_BACKLIGHF 1

uint8_t flags = FLAG_BACKLIGHF;	// reg #10

/*
 * UI
 */
uint8_t screen = 0;
int8_t selected_menu_item = 0;

#define SCREEN_MAIN 0
#define SCREEN_MENU 1
#define SCREEN_EDIT_MENU 2

void backlight_print(void) {
	if (flags & FLAG_BACKLIGHF)
		LCD_WriteText(" ON");
	else
		LCD_WriteText(" OFF");
}

void backlight_change(uint8_t kbr) {
	if (kbr & KB_CW_BUTTON || kbr & KB_CCW_BUTTON) {
		flags ^= FLAG_BACKLIGHF;
		if (flags & FLAG_BACKLIGHF)
			LCD_BacklightOn();
		else
			LCD_BacklightOff();
	}
}

void switch_id_print(void) {
	char buf[17];
	snprintf(buf, 17, " %u", switch_node_id);
	LCD_WriteText(buf);
}

void switch_id_change(uint8_t kbr) {
	if (kbr & KB_CW_BUTTON) {
		++switch_node_id;
		switch_node_id = switch_node_id & (H9MSG_BROADCAST_ID);
	}
	else if (kbr & KB_CCW_BUTTON) {
		--switch_node_id;
        switch_node_id = switch_node_id & (H9MSG_BROADCAST_ID);
	}
}

void power_switch_id_print(void) {
	char buf[17];
	snprintf(buf, 17, " %u", power_switch_node_id);
	LCD_WriteText(buf);
}

void power_switch_id_change(uint8_t kbr) {
	if (kbr & KB_CW_BUTTON) {
		++power_switch_node_id;
		power_switch_node_id = power_switch_node_id & (H9MSG_BROADCAST_ID);
	}
	else if (kbr & KB_CCW_BUTTON) {
		--power_switch_node_id;
        power_switch_node_id = power_switch_node_id & (H9MSG_BROADCAST_ID);
	}
}

void num_of_ant_print(void) {
	char buf[17];
	snprintf(buf, 17, " %d", number_of_antennas);
	LCD_WriteText(buf);
}

void num_of_ant_change(uint8_t kbr) {
	if (kbr & KB_CW_BUTTON) {
		++number_of_antennas;
	}
	else if (kbr & KB_CCW_BUTTON) {
		--number_of_antennas;
	}
}

#define MENU_ITEMS_COUNT (sizeof(menu_items)/sizeof(menu_items[0]))

struct {
	char *item;
	void (*value_print)(void);
	void (*change_value)(uint8_t kbr);
} menu_items[] = {
	{"BACKLIGHT:",       backlight_print, backlight_change},
	{"SWITCH ID:",       switch_id_print, switch_id_change},
	{"NUM OF ANTENNAS:", num_of_ant_print, num_of_ant_change},
	{"POWER SWITCH ID:", power_switch_id_print, power_switch_id_change}
};

void main_screen_refresh(void) {
    char buf[17];
    snprintf(buf, 17, " TX Ant: %d", tx_antenna);
    if (active_antenna == 1)
        buf[0] = '\x7e';
    LCD_Home();
    LCD_WriteText(buf);
    if (power_switch) {
        LCD_GoTo(15, 0);
        LCD_WriteData('\x01'); //power
    }
    snprintf(buf, 17, " RX Ant: %d", rx_antenna);
    if (active_antenna == 2)
        buf[0] = '\x7e';
    LCD_GoTo(0, 1);
    LCD_WriteText(buf);
    if (antenna_split == 1) {
        LCD_GoTo(15, 1);
        LCD_WriteData('\x02'); //rxtx
    }
}

void menu_screen_refresh(void) {
	LCD_Home();
	LCD_WriteText(menu_items[selected_menu_item].item);
	LCD_GoTo(0, 1);
	menu_items[selected_menu_item].value_print();
    LCD_GoTo(0, 1);
}

void ui(void) {
	uint8_t kb_tmp = KB_get();
	if (screen == SCREEN_MAIN) { //main screen
        static uint32_t esc_counter = 0;
        static uint32_t ok_counter = 0;
		int8_t ant = (active_antenna == 1) ? tx_antenna : rx_antenna;
		if (kb_tmp & KB_CW_BUTTON && active_antenna) {
			++ant;
			if (ant > number_of_antennas)
				ant = 1;
            switch_anttena((uint8_t)ant);
		}
		else if (kb_tmp & KB_CCW_BUTTON && active_antenna) {
			--ant;
			if (ant <= 0)
				ant = number_of_antennas;
            switch_anttena((uint8_t)ant);
		}

        if (kb_tmp == (KB_OK_BUTTON | KB_ESC_BUTTON_FLAG)) {
            KB_drop_key();
            screen = SCREEN_MENU;
            LCD_Clear();
            menu_screen_refresh();
        }
        else if (kb_tmp & KB_OK_BUTTON) {
            switch_rxtx();
		}
        else if (kb_tmp & KB_ESC_BUTTON) { //esc - dissable all (set to 0)
            switch_anttena(0);
		}
        else {
            if (kb_tmp & KB_OK_BUTTON_FLAG) {
                ok_counter++;
                if (ok_counter > 75000) {
                    KB_drop_key();
                    power_switch_on();
                    ok_counter = 0;
                }
            } else {
                ok_counter = 0;
            }
            if (kb_tmp & KB_ESC_BUTTON_FLAG) {
                esc_counter++;
                if (esc_counter > 75000) {
                    KB_drop_key();
                    power_switch_off();
                    esc_counter = 0;
                }
            } else {
                esc_counter = 0;
            }
        }
	}
	else if (screen == SCREEN_MENU) { //settings menu
		if (kb_tmp & KB_ESC_BUTTON) { //esc
			screen = SCREEN_MAIN;
			LCD_Clear();
			main_screen_refresh();
		}
		
		if (kb_tmp & KB_OK_BUTTON) { //enter to edit
			screen = SCREEN_EDIT_MENU;
			LCD_BlinkOn();
		}
		
		if (kb_tmp & KB_CW_BUTTON) {
			++selected_menu_item;
			if (selected_menu_item >= MENU_ITEMS_COUNT)
			    selected_menu_item = 0;
			menu_screen_refresh();
		}
		else if (kb_tmp & KB_CCW_BUTTON) {
			--selected_menu_item;
			if (selected_menu_item < 0)
			    selected_menu_item = MENU_ITEMS_COUNT-1;
			menu_screen_refresh();
		}
	}
	else if (screen == SCREEN_EDIT_MENU) { // edit menu
		if (kb_tmp & KB_OK_BUTTON || kb_tmp & KB_ESC_BUTTON) {
			screen = SCREEN_MENU;
			LCD_BlinkOff();
			menu_screen_refresh();
		}
		if (kb_tmp & KB_CW_BUTTON || kb_tmp & KB_CCW_BUTTON) {
			menu_items[selected_menu_item].change_value(kb_tmp);
			menu_screen_refresh();
		}
	}
}

int main(void) {
	DDRB = 0xff;
	DDRC = 0xff;
	DDRD = 0xff;
	DDRE = 0xff;
	
    LCD_Initalize();
	LCD_Home();
	LCD_WriteText("     h9-scu");
	LCD_GoTo(0, 1);
	LCD_WriteText("   by  SQ8KFH");
	
	KB_Initalize();
	
	CAN_init();
	CAN_set_mob_for_remote_node1(switch_node_id);
	CAN_set_mob_for_remote_node2(power_switch_node_id);
	sei();

	_delay_ms(100);
    logic_init();
	CAN_send_turned_on_broadcast();
	
	if (flags & FLAG_BACKLIGHF)
		LCD_BacklightOn();
	else
		LCD_BacklightOff();

    check_antenna();
    check_power_switch();
	
    while (1) {
		h9msg_t cm;
		if (CAN_get_msg(&cm)) {
			if (cm.source_id == switch_node_id) {
                process_antenna_switch_msg(&cm);
				main_screen_refresh();
			}
			else if (cm.source_id == power_switch_node_id) {
                process_power_switch_msg(&cm);
                main_screen_refresh();
			}
			else if (cm.type == H9MSG_TYPE_GET_REG &&
			         (cm.destination_id == can_node_id || cm.destination_id == H9MSG_BROADCAST_ID)) {
				h9msg_t cm_res;
				CAN_init_response_msg(&cm, &cm_res);
				cm_res.data[0] = cm.data[0];
				switch (cm.data[0]) {
					case 10:
						cm_res.dlc = 2;
						cm_res.data[1] = flags;
						CAN_put_msg(&cm_res);
						break;
					case 11:
						cm_res.dlc = 3;
						cm_res.data[1] = (switch_node_id >> 8) & 0x01;
						cm_res.data[2] = (switch_node_id) & 0x01;
						CAN_put_msg(&cm_res);
						break;
					case 12:
						cm_res.dlc = 2;
						cm_res.data[1] = number_of_antennas;
						CAN_put_msg(&cm_res);
						break;
					case 13:
						cm_res.dlc = 3;
						cm_res.data[1] = (power_switch_node_id >> 8) & 0x01;
						cm_res.data[2] = (power_switch_node_id) & 0x01;
						CAN_put_msg(&cm_res);
						break;
				}
			}
			else if (cm.type == H9MSG_TYPE_SET_REG && cm.destination_id == can_node_id) {
				h9msg_t cm_res;
				CAN_init_response_msg(&cm, &cm_res);
				cm_res.data[0] = cm.data[0];
				switch (cm.data[0]) {
					case 10:
						flags = cm.data[1];
						if (flags & FLAG_BACKLIGHF)
							LCD_BacklightOn();
						else
							LCD_BacklightOff();
						
						cm_res.dlc = 2;
						cm_res.data[1] = flags;
						CAN_put_msg(&cm_res);
						break;
					case 11:
						switch_node_id = ((cm_res.data[1] << 8) | cm_res.data[0]) & ((1<<H9MSG_DESTINATION_ID_BIT_LENGTH) - 1);
						
						cm_res.dlc = 3;
						cm_res.data[1] = (switch_node_id >> 8) & 0x01;
						cm_res.data[2] = (switch_node_id) & 0x01;
						CAN_put_msg(&cm_res);
						break;
					case 12:
						number_of_antennas = cm.data[1];
						cm_res.dlc = 2;
						cm_res.data[1] = number_of_antennas;
						CAN_put_msg(&cm_res);
					case 13:
						power_switch_node_id = ((cm_res.data[1] << 8) | cm_res.data[0]) & ((1<<H9MSG_DESTINATION_ID_BIT_LENGTH) - 1);
						
						cm_res.dlc = 3;
						cm_res.data[1] = (power_switch_node_id >> 8) & 0x01;
						cm_res.data[2] = (power_switch_node_id) & 0x01;
						CAN_put_msg(&cm_res);
						break;
				}
			}
		}
        process_ptt();
		ui();
	}
}
