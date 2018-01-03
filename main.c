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

#define FLAG_BACKLIGHF 1

uint8_t flags = FLAG_BACKLIGHF;	// reg #10
uint16_t switch_node_id = 1;    // reg #11
uint8_t number_of_antennas = 8; // reg #12
uint16_t power_switch_node_id = 2;    // reg #13

uint8_t selected_antenna = 0;
uint8_t power_switch = 0;

void send_switch_request(uint8_t ant) {
	h9msg_t tmp;
	CAN_init_new_msg(&tmp);
	tmp.type = H9_TYPE_SET_REG;
	tmp.destination_id = switch_node_id;
	tmp.dlc = 2;
	tmp.data[0] = 0x0a;
	tmp.data[1] = ant;
	CAN_put_msg(&tmp);
}

void get_selected_antenna(void) {
	h9msg_t tmp;
	CAN_init_new_msg(&tmp);
	tmp.type = H9_TYPE_GET_REG;
	tmp.destination_id = switch_node_id;
	tmp.dlc = 1;
	tmp.data[0] = 0x0a;
	CAN_put_msg(&tmp);
}

void send_power_switch_request(uint8_t on) {
	h9msg_t tmp;
	CAN_init_new_msg(&tmp);
	tmp.type = H9_TYPE_SET_REG;
	tmp.destination_id = power_switch_node_id;
	tmp.dlc = 2;
	tmp.data[0] = 0x0a;
	tmp.data[1] = on;
	CAN_put_msg(&tmp);
}

void get_power_switch(void) {
	h9msg_t tmp;
	CAN_init_new_msg(&tmp);
	tmp.type = H9_TYPE_GET_REG;
	tmp.destination_id = power_switch_node_id;
	tmp.dlc = 1;
	tmp.data[0] = 0x0a;
	CAN_put_msg(&tmp);
}

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
		LCD_WriteText(" ON             ");
	else
		LCD_WriteText(" OFF            ");
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
	char buf[17] = "                ";
	int n = sprintf(buf, " 0x%X", switch_node_id);
	buf[n] = ' ';
	LCD_WriteText(buf);
}

void switch_id_change(uint8_t kbr) {
	if (kbr & KB_CW_BUTTON) {
		++switch_node_id;
		switch_node_id &= ((1 << H9_DESTINATION_ID_BIT_LENGTH) - 1);
		if (switch_node_id == ((1 << H9_DESTINATION_ID_BIT_LENGTH) - 1))
			switch_node_id = 1;
	}
	else if (kbr & KB_CCW_BUTTON) {
		--switch_node_id;
		switch_node_id &= ((1 << H9_DESTINATION_ID_BIT_LENGTH) - 1);
		if (switch_node_id == 0)
			switch_node_id = (1 << H9_DESTINATION_ID_BIT_LENGTH) - 2;
	}
}

void power_switch_id_print(void) {
	char buf[17] = "                ";
	int n = sprintf(buf, " 0x%X", power_switch_node_id);
	buf[n] = ' ';
	LCD_WriteText(buf);
}

void power_switch_id_change(uint8_t kbr) {
	if (kbr & KB_CW_BUTTON) {
		++power_switch_node_id;
		power_switch_node_id &= ((1 << H9_DESTINATION_ID_BIT_LENGTH) - 1);
		if (power_switch_node_id == ((1 << H9_DESTINATION_ID_BIT_LENGTH) - 1))
		power_switch_node_id = 1;
	}
	else if (kbr & KB_CCW_BUTTON) {
		--power_switch_node_id;
		power_switch_node_id &= ((1 << H9_DESTINATION_ID_BIT_LENGTH) - 1);
		if (power_switch_node_id == 0)
		power_switch_node_id = (1 << H9_DESTINATION_ID_BIT_LENGTH) - 2;
	}
}

void num_of_ant_print(void) {
	char buf[17] = "                ";
	int n = sprintf(buf, " %d", number_of_antennas);
	buf[n] = ' ';
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
	{"BACKLIGHT:      ", backlight_print, backlight_change},
	{"SWITCH ID:      ", switch_id_print, switch_id_change},
	{"NUM OF ANTENNAS:", num_of_ant_print, num_of_ant_change},
	{"POWER SWITCH ID:", power_switch_id_print, power_switch_id_change}
};

void main_screen_refresh(void) {
	char buf[17] = "                ";
	int n = sprintf(buf, "Antenna: %d", selected_antenna);
	buf[n] = ' ';
	LCD_Home();
	LCD_WriteText(buf);
	n = sprintf(buf, "Power switch:%s", power_switch ? " ON" : "OFF");
	buf[16] = '\0';
	LCD_GoTo(0, 1);
	LCD_WriteText(buf);
	LCD_GoTo(0, 1);
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
	if (screen == SCREEN_MAIN) //main screen
	{
		uint8_t ant = selected_antenna;
		if (kb_tmp & KB_CW_BUTTON) {
			++ant;
			if (ant > number_of_antennas)
				ant = 1;
			send_switch_request(ant);
		}
		else if (kb_tmp & KB_CCW_BUTTON) {
			--ant;
			if (ant == 0)
				ant = number_of_antennas;
			send_switch_request(ant);
		}
		
		if (kb_tmp & KB_OK_BUTTON) {
			send_power_switch_request(!power_switch);
		}
		
		if (kb_tmp & KB_ESC_BUTTON) { //esc - dissable all (set to 0)
			send_switch_request(0);
		}
		
		if ((kb_tmp & (KB_OK_BUTTON | KB_ESC_BUTTON_FLAG)) == (KB_OK_BUTTON | KB_ESC_BUTTON_FLAG)) {
			KB_drop_key();
			screen = SCREEN_MENU;
			LCD_Clear();
			menu_screen_refresh();
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
	//CAN_set_mob(0, 0, H9_TYPE_REG_EXTERNALLY_CHANGED, ((1<<H9_TYPE_BIT_LENGTH)-1) ^ 0x03, 0, 0, switch_node_id, (1<<H9_SOURCE_ID_BIT_LENGTH)-1);
	CAN_set_mob_for_remote_node1(switch_node_id);
	CAN_set_mob_for_remote_node2(power_switch_node_id);
	sei();
	
	_delay_ms(100);
	CAN_send_turned_on_broadcast();
	
	if (flags & FLAG_BACKLIGHF)
		LCD_BacklightOn();
	else
		LCD_BacklightOff();
	
	get_selected_antenna();
	get_power_switch();
	LCD_Clear();
	
    while (1) {
		h9msg_t cm;
		if (CAN_get_msg(&cm)) {
			if (cm.source_id == switch_node_id && 
			   cm.dlc > 1 &&
			   cm.data[0] == 0x0a &&
			   (cm.type & 0xFC) == H9_TYPE_REG_EXTERNALLY_CHANGED) { // match: H9_TYPE_REG: EXTERNALLY_CHANGED INTERNALLY_CHANGED VALUE_BROADCAST VALUE
				selected_antenna = cm.data[1];
				main_screen_refresh();
			}
			else if (cm.source_id == switch_node_id &&
			         cm.type == H9_TYPE_NODE_TURNED_ON) {
				get_selected_antenna();
			}
			else if (cm.source_id == power_switch_node_id &&
				cm.dlc > 1 &&
				cm.data[0] == 0x0a &&
				(cm.type & 0xFC) == H9_TYPE_REG_EXTERNALLY_CHANGED) { // match: H9_TYPE_REG: EXTERNALLY_CHANGED INTERNALLY_CHANGED VALUE_BROADCAST VALUE
					power_switch = cm.data[1];
					main_screen_refresh();
			}
			else if (cm.source_id == power_switch_node_id && cm.type == H9_TYPE_NODE_TURNED_ON) {
				get_power_switch();
			}
			else if (cm.type == H9_TYPE_GET_REG && 
			         (cm.destination_id == can_node_id || cm.destination_id == H9_BROADCAST_ID)) {
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
			else if (cm.type == H9_TYPE_SET_REG && cm.destination_id == can_node_id) {
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
						switch_node_id = ((cm_res.data[1] << 8) | cm_res.data[0]) & ((1<<H9_DESTINATION_ID_BIT_LENGTH) - 1);
						
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
						power_switch_node_id = ((cm_res.data[1] << 8) | cm_res.data[0]) & ((1<<H9_DESTINATION_ID_BIT_LENGTH) - 1);
						
						cm_res.dlc = 3;
						cm_res.data[1] = (power_switch_node_id >> 8) & 0x01;
						cm_res.data[2] = (power_switch_node_id) & 0x01;
						CAN_put_msg(&cm_res);
						break;
				}
			}
		}
		ui();
	}
}
