#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/pgmspace.h>

#include "lcd.h"
#include "uart.h"
#include "light_ws2812.h"

#define EEPROM_START_ADDR (void*)0x10

typedef struct {
	char nombre[4];
	uint8_t minutos;
	uint8_t segundos;
} RegistroJugador;

struct cRGB leds[5];
const char banco_palabras[][6] PROGMEM = {
	"MICRO", "PUNTO", "RELOJ", "CABLE", "PLACA",
	"DATOS", "FUEGO", "ARBOL", "PERRO", "GATOS",
	"NIEVE", "COCHE", "TECLA", "MUNDO", "PIZZA",
	"AMIGO", "AVION", "BARCO", "BEBER", "BESOS",
	"BOTON", "BRISA", "BURRO", "CALOR", "CAMPO",
	"CANTO", "CARRO", "CASAS", "CEBRA", "CENAS",
	"CERCA", "CIELO", "CINTA", "CLARO", "CLASE",
	"COLOR", "COMER", "COPAS", "CORAL", "CORRE",
	"CORTA", "CREMA", "CRUCE", "CUERO", "CURVA",
	"DANZA", "DEDOS", "DIETA", "DINOS", "DOBLE",
	"DOLAR", "DROGA", "DUCHA", "DULCE", "ECHAR",
	"ERROR", "ESPIA", "FACIL", "FALTA", "FAROL",
	"FERIA", "FIBRA", "FIESTA", "FLACO", "FLOTA",
	"FLUJO", "FONDO", "FORMA", "FRUTA", "FUERA",
	"FUMAR", "GAFAS", "GANAR", "GIRAR", "GLOBO",
	"GOLPE", "GORRO", "GRANO", "GRITO", "GRUPO",
	"GUSTO", "HABLA", "HIELO", "HOGAR", "HORNO",
	"HOTEL", "HUEVO", "HUMOR", "IDEAL", "IGUAL",
	"JABON", "JAMAS", "JARDI", "JOVEN", "JUEGO",
	"JUNTO", "LABIO", "LAPIZ", "LARGO", "LATIR",
	"LECHE", "LENTO", "LIBRO", "LIMON", "LINDA",
	"LLAVE", "LLENO", "LUCHA", "LUNAS", "MADRE",
	"MAGIA", "MANGO", "MANOS", "MARCO", "MARTE",
	"MAYOR", "MEDIO", "MENTA", "METAL", "METRO",
	"MIELO", "MINAS", "MITAD", "MODOS", "MONTE",
	"MORIR", "MOTOR", "MUELA", "MUJER", "MURAL",
	"MUSGO", "NACER", "NADAR", "NEGRO", "NOBLE",
	"NOCHE", "NORTE", "NOTAS", "NUBES", "NUEVO",
	"OASIS", "OBRAS", "OCASO", "OJERA", "OLIVA",
	"ONDAS", "OPERA", "ORDEN", "OVEJA", "PADRE",
	"PAGAR", "PALMA", "PANEL", "PAPEL", "PARDO",
	"PARTE", "PASAR", "PASTA", "PATIO", "PAUSA",
	"PECES", "PECHO", "PEDAL", "PEGAR", "PELAR",
	"PELTA", "PENAS", "PERLA", "PESCA", "PIANO",
	"PIEZA", "PILAR", "PINTO", "PISAR", "PLAYA",
	"PLUMA", "POBRE", "PODER", "POLLO", "PONER",
	"PORTA", "PRADO", "PRESA", "PRIMO", "PRISA",
	"PROFE", "PULSO", "QUESO", "QUITA", "RADIO",
	"RAMPA", "RANGO", "RAPTO", "RASGO", "RATON",
	"RECIO", "RECTA", "REGLA", "REINA", "RENTA",
	"RITMO", "ROBOT", "ROCAS", "ROLLO", "RONDA",
	"ROTAR", "RUBIO", "RUIDO", "RURAL", "SABER",
	"SALIR", "SALON", "SALTO", "SANTO", "SELVA",
	"SERIE", "SIGNO", "SILLA", "SITIO", "SOBRA",
	"SOLAR", "SONAR", "SOPLA", "SUBIR", "SUCIO",
	"SUELO", "SUERO", "SURCO", "TABLA", "TACTO",
	"TARDE", "TECHO", "TELON", "TEMOR", "TENIS",
	"TEXTO", "TIGRE", "TINTO", "TIRAR", "TOCAR",
	"TOMAR", "TORRE", "TRAJE", "TRAMA", "TRENO",
	"TRIGO", "TRUCO", "TURNO", "UNICO", "USADO",
	"VACIO", "VALOR", "VELAS", "VENDA", "VENIR",
	"VERBO", "VERDE", "VIAJE", "VIDEO", "VIEJO",
	"VIGOR", "VIRUS", "VISTA", "VITAL", "VOCAL",
	"YOGUR", "ZORRO"
};
#define TOTAL_PALABRAS (sizeof(banco_palabras) / sizeof(banco_palabras[0]))

char palabra_secreta[6];

volatile uint8_t segundos = 0;
volatile uint8_t minutos = 0;
volatile uint8_t actualizar_lcd = 0;
volatile uint32_t milisegundos = 0;

typedef enum {
	ESTADO_MENU,
	ESTADO_JUGANDO,
	ESTADO_GAME_OVER,
	ESTADO_INGRESO_NOMBRE
} EstadoJuego;

void timer0_init_millis() {
	TCCR0A |= (1 << WGM01);
	TCCR0B |= (1 << CS01) | (1 << CS00);
	OCR0A = 249;
	TIMSK0 |= (1 << OCIE0A);
}

ISR(TIMER0_COMPA_vect) {
	milisegundos++;
}

void timer1_init_cronometro() {
	TCCR1B |= (1 << WGM12);
	TCCR1B |= (1 << CS12);
	OCR1A = 62499;
	TIMSK1 |= (1 << OCIE1A);
}

ISR(TIMER1_COMPA_vect) {
	segundos++;
	if (segundos >= 60) {
		segundos = 0;
		minutos++;
	}
	actualizar_lcd = 1;
}

const char mapa_teclado[4][4] = {
	{'1', '2', '3', 'A'},
	{'4', '5', '6', 'B'},
	{'7', '8', '9', 'C'},
	{'*', '0', '#', 'D'}
};

const char mapa_t9[8][4] = {
	{'A','B','C','2'},
	{'D','E','F','3'},
	{'G','H','I','4'},
	{'J','K','L','5'},
	{'M','N','O','6'},
	{'P','Q','R','S'},
	{'T','U','V','8'},
	{'W','X','Y','Z'}
};

void teclado_init() {
	DDRC |= (1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC3);
	PORTC |= (1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC3);
	DDRB &= ~((1 << PB2) | (1 << PB3) | (1 << PB4) | (1 << PB5));
	PORTB |= (1 << PB2) | (1 << PB3) | (1 << PB4) | (1 << PB5);
}

char escanear_teclado_matriz() {
	for (uint8_t fila = 0; fila < 4; fila++) {
		PORTC |= (1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC3);
		PORTC &= ~(1 << fila);
		_delay_us(10);
		if (!(PINB & (1 << PB2))) return mapa_teclado[fila][0];
		if (!(PINB & (1 << PB3))) return mapa_teclado[fila][1];
		if (!(PINB & (1 << PB4))) return mapa_teclado[fila][2];
		if (!(PINB & (1 << PB5))) return mapa_teclado[fila][3];
	}
	return 0;
}

char tecla_anterior_fisica = 0;

char leer_teclado_raw() {
	char tecla_actual = escanear_teclado_matriz();
	if (tecla_actual != 0 && tecla_actual != tecla_anterior_fisica) {
		_delay_ms(50);
		if (escanear_teclado_matriz() == tecla_actual) {
			tecla_anterior_fisica = tecla_actual;
			return tecla_actual;
		}
	}
	if (tecla_actual == 0) {
		tecla_anterior_fisica = 0;
	}
	return 0;
}

#define BRILLO 40

void apagar_leds() {
	for(int i = 0; i < 5; i++) {
		leds[i].r = 0; leds[i].g = 0; leds[i].b = 0;
	}
	ws2812_setleds(leds, 5);
}

uint8_t evaluar_intento(char* intento) {
	uint8_t aciertos = 0;
	uint8_t flags_secreta[5] = {0};
	char intento_temp[5];
	
	for(int i = 0; i < 5; i++) {
		intento_temp[i] = intento[i];
	}

	apagar_leds();

	for (int i = 0; i < 5; i++) {
		if (intento_temp[i] == palabra_secreta[i]) {
			leds[i].r = 0; leds[i].g = BRILLO; leds[i].b = 0;
			flags_secreta[i] = 1;
			intento_temp[i] = '*';
			aciertos++;
		}
	}

	for (int i = 0; i < 5; i++) {
		if (intento_temp[i] != '*') {
			uint8_t encontrada = 0;
			for (int j = 0; j < 5; j++) {
				if (flags_secreta[j] == 0 && intento_temp[i] == palabra_secreta[j]) {
					leds[i].r = BRILLO; leds[i].g = BRILLO; leds[i].b = 0;
					flags_secreta[j] = 1;
					encontrada = 1;
					break;
				}
			}
			if (!encontrada) {
				leds[i].r = BRILLO; leds[i].g = 0; leds[i].b = 0;
			}
		}
	}
	
	ws2812_setleds(leds, 5);
	
	return (aciertos == 5);
}

void guardar_ranking_eeprom(RegistroJugador nuevo) {
	RegistroJugador ranking[5];
	eeprom_read_block((void*)ranking, EEPROM_START_ADDR, sizeof(RegistroJugador) * 5);
	
	uint16_t tiempo_nuevo = (nuevo.minutos * 60) + nuevo.segundos;
	int posicion = -1;
	
	for(int i=0; i<5; i++) {
		uint8_t es_valido = (ranking[i].nombre[0] >= 'A' && ranking[i].nombre[0] <= 'Z');
		uint16_t tiempo_actual = (ranking[i].minutos * 60) + ranking[i].segundos;
		
		if (!es_valido || tiempo_nuevo <= tiempo_actual) {
			posicion = i;
			break;
		}
	}
	
	if (posicion != -1) {
		for(int i = 4; i > posicion; i--) {
			ranking[i] = ranking[i-1];
		}
		ranking[posicion] = nuevo;
		eeprom_write_block((const void*)ranking, EEPROM_START_ADDR, sizeof(RegistroJugador) * 5);
	}
}

void mostrar_ranking_uart() {
	uart0_puts("\r\n--- RANKING WORDLE ---\r\n");
	RegistroJugador reg;
	for(int i=0; i<5; i++) {
		eeprom_read_block((void*)&reg, (const void*)(0x10 + (i * sizeof(RegistroJugador))), sizeof(RegistroJugador));
		if(reg.nombre[0] >= 'A' && reg.nombre[0] <= 'Z') {
			char buffer[40];
			sprintf(buffer, "%d. %s - %02d:%02d\r\n", i+1, reg.nombre, reg.minutos, reg.segundos);
			uart0_puts(buffer);
			} else {
			char buffer[40];
			sprintf(buffer, "%d. ----\r\n", i+1);
			uart0_puts(buffer);
		}
	}
	uart0_puts("----------------------\r\n");
}

int main(void) {
	uart0_init(UART_BAUD_SELECT(9600, F_CPU));
	lcd_init();
	timer0_init_millis();
	timer1_init_cronometro();
	apagar_leds();
	teclado_init();
	sei();
	
	EstadoJuego estado = ESTADO_MENU;
	uint8_t bandera_menu = 1;
	uint8_t num_intento = 1;
	char intento_actual[6] = "";
	uint8_t indice_letra = 0;
	char buffer_lcd[17];
	uint32_t ultimo_tiempo_tecla = 0;
	char ultima_tecla = 0;
	uint8_t indice_tap = 0;

	while (1) {
		switch (estado) {
			
			case ESTADO_MENU:
			if (bandera_menu == 1) {
				uart0_puts("\r\n======================\r\n");
				uart0_puts("    MENU PRINCIPAL    \r\n");
				uart0_puts("======================\r\n");
				uart0_puts("1. Jugar Wordle\r\n");
				uart0_puts("2. Ver Ranking\r\n");
				uart0_puts("Seleccione una opcion: \r\n");
				
				lcd_clear();
				lcd_gotoxy(0, 0);
				lcd_string_signed("1. Jugar Wordle");
				lcd_gotoxy(1, 0);
				lcd_string_signed("2. Ver Ranking");
				
				bandera_menu = 0;
			}

			char tecla_menu = leer_teclado_raw();
			uint16_t c = uart0_getc();
			char opcion = 0;
			
			if (tecla_menu == '1' || tecla_menu == '2') {
				opcion = tecla_menu;
				} else if ((c & UART_NO_DATA) == 0) {
				opcion = (char)c;
			}
			
			if (opcion == '1') {
				srand(milisegundos);
				uint8_t indice_aleatorio = rand() % TOTAL_PALABRAS;
				strcpy_P(palabra_secreta, banco_palabras[indice_aleatorio]);

				num_intento = 1;
				indice_letra = 0;
				intento_actual[0] = '\0';
				minutos = 0;
				segundos = 0;
				
				lcd_clear();
				lcd_gotoxy(1, 0);
				lcd_string_signed("Intento: 1");
				
				uart0_puts("\r\n--- ¡Juego Iniciado! ---\r\n");
				uart0_puts("Introduce letras desde el teclado:\r\n");

				estado = ESTADO_JUGANDO;
			}
			else if (opcion == '2') {
				mostrar_ranking_uart();
				bandera_menu = 1;
			}
			
			break;

			case ESTADO_JUGANDO:
			{
				char tecla = leer_teclado_raw();

				if (tecla != 0) {
					if (tecla >= '2' && tecla <= '9') {
						if (tecla == ultima_tecla && (milisegundos - ultimo_tiempo_tecla) < 1000) {
							indice_tap++;
						}
						else {
							if (ultima_tecla != 0 && indice_letra < 5) {
								indice_letra++;
							}
							if (indice_letra < 5) {
								ultima_tecla = tecla;
								indice_tap = 0;
							}
						}
						
						ultimo_tiempo_tecla = milisegundos;
						
						if (indice_letra < 5) {
							uint8_t array_index = tecla - '2';
							uint8_t letras_en_tecla = (tecla == '7' || tecla == '9') ? 4 : 3;
							indice_tap = indice_tap % letras_en_tecla;
							intento_actual[indice_letra] = mapa_t9[array_index][indice_tap];
							intento_actual[indice_letra + 1] = '\0';
							
							lcd_gotoxy(0, 0);
							lcd_string_signed(intento_actual);
						}
					}
					else if (tecla == '*') {
						if (ultima_tecla != 0) {
							ultima_tecla = 0;
							intento_actual[indice_letra] = '\0';
						}
						else if (indice_letra > 0) {
							indice_letra--;
							intento_actual[indice_letra] = '\0';
						}
						lcd_gotoxy(0, 0);
						lcd_string_signed(intento_actual);
						lcd_char(' ');
					}
					else if (tecla == '#') {
						if (ultima_tecla != 0 && indice_letra < 5) {
							indice_letra++;
							ultima_tecla = 0;
						}
						
						if (indice_letra == 5) {
							uart0_puts("Palabra enviada: ");
							uart0_puts(intento_actual);
							uart0_puts("\r\n");
							
							uint8_t es_correcta = evaluar_intento(intento_actual);
							
							if (es_correcta) {
								uart0_puts("¡FELICIDADES! Ganaste.\r\n");
								estado = ESTADO_GAME_OVER;
								} else {
								num_intento++;
								if (num_intento > 5) {
									uart0_puts("GAME OVER. La palabra era: ");
									uart0_puts(palabra_secreta);
									uart0_puts("\r\n");
									
									lcd_clear();
									lcd_string_signed("GAME OVER");
									lcd_gotoxy(1, 0);
									lcd_string_signed(palabra_secreta);
									
									_delay_ms(3000);
									
									estado = ESTADO_MENU;
									bandera_menu = 1;
									apagar_leds();
									} else {
									indice_letra = 0;
									intento_actual[0] = '\0';
									lcd_gotoxy(0, 0);
									lcd_string_signed("     ");
									lcd_gotoxy(1, 0);
									lcd_string_signed("Intento: ");
									lcd_char('0' + num_intento);
								}
							}
						}
					}
				}

				if (ultima_tecla != 0 && (milisegundos - ultimo_tiempo_tecla) > 1000) {
					if (indice_letra < 5) {
						indice_letra++;
						ultima_tecla = 0;
					}
				}
			}
			break;

			case ESTADO_GAME_OVER:
			lcd_clear();
			lcd_string_signed("!GANASTE!");
			lcd_gotoxy(1, 0);
			sprintf(buffer_lcd, "Tiempo %02d:%02d", minutos, segundos);
			lcd_string_signed(buffer_lcd);
			
			uart0_puts("\r\n¡Felicidades!\r\n");
			
			_delay_ms(3000);
			
			lcd_clear();
			lcd_gotoxy(0, 0);
			lcd_string_signed("Nombre:");
			lcd_gotoxy(1, 0);
			
			indice_letra = 0;
			intento_actual[0] = '\0';
			ultima_tecla = 0;
			indice_tap = 0;
			
			estado = ESTADO_INGRESO_NOMBRE;
			break;
			
			case ESTADO_INGRESO_NOMBRE:
			{
				char tecla = leer_teclado_raw();

				if (tecla != 0) {
					if (tecla >= '2' && tecla <= '9') {
						if (tecla == ultima_tecla && (milisegundos - ultimo_tiempo_tecla) < 1000) {
							indice_tap++;
						}
						else {
							if (ultima_tecla != 0 && indice_letra < 3) {
								indice_letra++;
							}
							if (indice_letra < 3) {
								ultima_tecla = tecla;
								indice_tap = 0;
							}
						}
						
						ultimo_tiempo_tecla = milisegundos;
						
						if (indice_letra < 3) {
							uint8_t array_index = tecla - '2';
							uint8_t letras_en_tecla = (tecla == '7' || tecla == '9') ? 4 : 3;
							indice_tap = indice_tap % letras_en_tecla;
							intento_actual[indice_letra] = mapa_t9[array_index][indice_tap];
							intento_actual[indice_letra + 1] = '\0';
							
							lcd_gotoxy(1, 0);
							lcd_string_signed(intento_actual);
						}
					}
					else if (tecla == '*') {
						if (ultima_tecla != 0) {
							ultima_tecla = 0;
							intento_actual[indice_letra] = '\0';
						}
						else if (indice_letra > 0) {
							indice_letra--;
							intento_actual[indice_letra] = '\0';
						}
						lcd_gotoxy(1, 0);
						lcd_string_signed(intento_actual);
						lcd_char(' ');
					}
					else if (tecla == '#') {
						if (ultima_tecla != 0 && indice_letra < 3) {
							indice_letra++;
							ultima_tecla = 0;
						}
						
						if (indice_letra == 3) {
							RegistroJugador nuevo_record;
							nuevo_record.minutos = minutos;
							nuevo_record.segundos = segundos;
							nuevo_record.nombre[0] = intento_actual[0];
							nuevo_record.nombre[1] = intento_actual[1];
							nuevo_record.nombre[2] = intento_actual[2];
							nuevo_record.nombre[3] = '\0';
							
							guardar_ranking_eeprom(nuevo_record);
							
							uart0_puts("\r\nRecord guardado. Volviendo al menu...\r\n");
							lcd_clear();
							lcd_gotoxy(0,0);
							lcd_string_signed("Guardado!");
							
							_delay_ms(2000);
							apagar_leds();
							estado = ESTADO_MENU;
							bandera_menu = 1;
						}
					}
				}

				if (ultima_tecla != 0 && (milisegundos - ultimo_tiempo_tecla) > 1000) {
					if (indice_letra < 3) {
						indice_letra++;
						ultima_tecla = 0;
					}
				}
			}
			break;
		}
	}
	return 0;
}
