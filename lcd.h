#ifndef _LCD_H_
#define _LCD_H_

#include <avr/io.h>
#include <util/delay.h>

static inline void latch(void) {
	PORTB |= 0x02;
	_delay_us(2);
	PORTB &= ~0x02;
	_delay_us(2);
}

static inline void lcd_cmd(unsigned char cmd) {
	PORTB &= ~0x01;
	PORTD = (PORTD & 0x0F) | (cmd & 0xF0);
	latch();
	PORTD = (PORTD & 0x0F) | ((cmd & 0x0F) * 16);
	latch();
	_delay_us(50);
}

static inline void lcd_clear(void) {
	lcd_cmd(0x01);
	_delay_ms(2);
}

static inline void lcd_char(unsigned char single) {
	PORTB |= 0x01;
	PORTD = (PORTD & 0x0F) | (single & 0xF0);
	latch();
	PORTD = (PORTD & 0x0F) | ((single & 0x0F) * 16);
	latch();
	_delay_us(50);
}

static inline void lcd_init(void) {
	_delay_ms(50);
	
	DDRD |= 0xF0;
	DDRB |= 0x03;
	PORTB &= ~0x03;
	
	PORTD = (PORTD & 0x0F) | 0x30;
	latch();
	_delay_ms(5);
	
	latch();
	_delay_us(150);
	
	latch();
	_delay_us(150);
	
	PORTD = (PORTD & 0x0F) | 0x20;
	latch();
	_delay_ms(2);
	lcd_cmd(0x28);
	lcd_cmd(0x0C);
	lcd_cmd(0x06);
	lcd_clear();
}

static inline void lcd_string(unsigned char *str) {
	unsigned char i=0;
	while(str[i]!='\0') {
		if(i==16) lcd_cmd(0xC0);
		lcd_char(str[i]);
		i++;
	}
}

static inline void lcd_string_signed(char* str) {
	unsigned char i=0;
	while(str[i]!='\0') {
		if(i==16) lcd_cmd(0xC0);
		lcd_char(str[i]);
		i++;
	}
}

static inline void lcd_showvalue(unsigned char num) {
	unsigned char H=0,T=0,O=0;
	H=num/100;
	T=(num - (H*100))/10;
	O=(num - (H*100) - (T*10));
	lcd_char(H+48);
	lcd_char(T+48);
	lcd_char(O+48);
}

static inline void lcd_gotoxy(unsigned char row,unsigned char column) {
	if(row==0) lcd_cmd(0x80+column);
	else if(row==1) lcd_cmd(0xC0+column);
}

#endif
