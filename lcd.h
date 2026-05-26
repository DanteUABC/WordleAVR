#ifndef _LCD_H_
#define _LCD_H_

#include <avr/io.h>
#include <util/delay.h>

static inline void latch(void) {
	PORTB |= 0x02; // Pone EN Alto (Pin 9)
	_delay_us(2);  // 2 microsegundos para que la pantalla lea el pulso
	PORTB &= ~0x02; // Pone EN Bajo
	_delay_us(2);
}

static inline void lcd_cmd(unsigned char cmd) {
	PORTB &= ~0x01; // RS = 0 (Modo Comando)
	PORTD = (PORTD & 0x0F) | (cmd & 0xF0);
	latch();
	PORTD = (PORTD & 0x0F) | ((cmd & 0x0F) * 16);
	latch();
	_delay_us(50); // ESPERA VITAL: Tiempo para que la LCD procese el comando
}

// Moviendo lcd_clear ARRIBA para que lcd_init pueda usarla sin error
static inline void lcd_clear(void) {
	lcd_cmd(0x01);
	_delay_ms(2); // Limpiar pantalla siempre requiere 2ms
}

static inline void lcd_char(unsigned char single) {
	PORTB |= 0x01; // RS = 1 (Modo Dato)
	PORTD = (PORTD & 0x0F) | (single & 0xF0);
	latch();
	PORTD = (PORTD & 0x0F) | ((single & 0x0F) * 16);
	latch();
	_delay_us(50); // ESPERA VITAL: Tiempo para que la LCD dibuje la letra
}

static inline void lcd_init(void) {
	_delay_ms(50); // ESPERA VITAL: Dar tiempo a que el voltaje de la LCD se estabilice
	
	DDRD |= 0xF0; // Configurar Pines 4, 5, 6, 7 como salidas (Datos)
	DDRB |= 0x03; // Configurar Pines 8 y 9 como salidas (RS y EN)
	PORTB &= ~0x03; // Asegurar RS=0 y EN=0
	
	// --- Secuencia obligatoria de reinicio del chip HD44780 ---
	// Paso 1: Enviar el comando 0x30 tres veces seguidas
	PORTD = (PORTD & 0x0F) | 0x30;
	latch();
	_delay_ms(5); // Esperar más de 4.1ms
	
	latch();
	_delay_us(150); // Esperar más de 100us
	
	latch();
	_delay_us(150);
	
	// Paso 2: Cambiar a modo de 4 bits (Enviar 0x20)
	PORTD = (PORTD & 0x0F) | 0x20;
	latch();
	_delay_ms(2);
	// ---------------------------------------------------------
	
	// A partir de aquí, la LCD ya nos hace caso y podemos usar lcd_cmd()
	lcd_cmd(0x28); // Comando mágico: 4 bits, 2 líneas, fuente 5x8
	lcd_cmd(0x0C); // Display ON, Cursor OFF
	lcd_cmd(0x06); // Incrementar cursor
	lcd_clear();   // Ahora el compilador ya conoce lcd_clear y no dará error
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