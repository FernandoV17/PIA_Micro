/*
 * mux_display_7seg.c
 *
 * Created: 15/04/2025 11:10:43 a. m.
 *  Author: fervi
 */ 

#define F_CPU 16000000UL
#include "DISPLAY_7SEG_MUX.h"
#include <avr/interrupt.h>
#include <util/delay.h>

/*
   Mapa de bits para display de 7 segmentos (CÁTODO COMÚN)

    Segmento:   Bit:    Visual:
       A         0       ---
      F B      5   1    |   |
       G         6       ---
      E C      4   2    |   |
       D         3       ---

   Orden de bits: 0bGFEDCBA0
*/

const uint8_t segment_map[] = {
	0b01111110, // 0  A B C D E F
	0b00001100, // 1    B C
	0b10110110, // 2  A B   D E   G
	0b10011110, // 3  A B C D     G
	0b11001100, // 4    B C     F G
	0b11011010, // 5  A   C D   F G
	0b11111010, // 6  A   C D E F G
	0b00001110, // 7  A B C
	0b11111110, // 8  A B C D E F G
	0b11011110, // 9  A B C D   F G
	0b10000000, // -              G
	0b11101100, // H    B C   E F G
	0b00001100, // I    B C
    0b01110000, // L      D E F
    0b11100000  // T  A F G
};




volatile uint8_t display_buffer[4] = {0};
volatile uint8_t current_digit = 0;

void init_display_mux(void) {
	display_7SEG_DDRX = 0xFF;
	display_mux_DDRX |= (1 << DIG1) | (1 << DIG2) | (1 << DIG3) | (1 << DIG4);
	display_mux_PORTX &= ~((1 << DIG1) | (1 << DIG2) | (1 << DIG3) | (1 << DIG4));
}

void update_display(uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4) {
	display_mux_PORTX &= ~((1 << DIG1) | (1 << DIG2) | (1 << DIG3) | (1 << DIG4));
	display_7SEG_PORTX = 0x00;
	_delay_ms(2);
	display_buffer[0] = d1;
	display_buffer[1] = d2;
	display_buffer[2] = d3;
	display_buffer[3] = d4;
}


ISR(TIMER1_COMPA_vect) {
	// 1. Apagar todos los dígitos (poner en LOW los pines del mux)
	display_mux_PORTX &= ~((1 << DIG1) | (1 << DIG2) | (1 << DIG3) | (1 << DIG4));
	
	// 2. Preparar segmentos para el dígito actual
	display_7SEG_PORTX = segment_map[display_buffer[current_digit]];
	
	// 3. Activar solo el dígito actual (HIGH en el pin del mux correspondiente)
	switch(current_digit) {
		case 0: display_mux_PORTX |= (1 << DIG1); break;
		case 1: display_mux_PORTX |= (1 << DIG2); break;
		case 2: display_mux_PORTX |= (1 << DIG3); break;
		case 3: display_mux_PORTX |= (1 << DIG4); break;
	}
	
	// 4. Rotar al siguiente dígito
	current_digit = (current_digit + 1) % 4;
}

void timer1_init(void) {
	TCCR1A = 0;
	TCCR1B = (1 << WGM12) | (1 << CS11); // Modo CTC, prescaler 8
	OCR1A = 9999; // 16MHz/(8 * 10000) = 200Hz (5ms ciclo completo)
	TIMSK1 = (1 << OCIE1A);
}