/*
 * BUTTON.c
 *
 * Created: 26/04/2025 05:44:23 p. m.
 *  Author: fervi
 */ 

#include "BUTTON.h"

// Variables de estado
static uint8_t last_state = 0;
static uint8_t actual_state = 0;
static uint8_t debounced_state = 0;

#define DEBOUNCE_TIME 500

void buttons_init(void) {
	// Configurar pines como entrada
	DDRD &= ~(BUTTON1_MASK | BUTTON2_MASK);
	// Desactivar resistencias de pull-up
	PORTD &= ~(BUTTON1_MASK | BUTTON2_MASK);
	
	// Leer estado inicial
	actual_state = BUTTONS_READ;
	last_state = actual_state;
	debounced_state = actual_state;
}

void buttons_update(void) {
	// Aplicar debounce
	_delay_ms(DEBOUNCE_TIME);
	
	last_state = actual_state;
	actual_state = BUTTONS_READ;
	
	/* 
	* Usa mascaras de bits para aislar solo el boton X:
	* 1. ~BUTTONX_MASK: Invierte la máscara para limpiar (0) solo ese bit
	* 2. actual_state & BUTTONX_MASK: Captura el estado actual de ese boton
	* Combina ambos para actualizar solo el bit del boton X sin afectar los demas
	*/
	
	if ((actual_state & BUTTON1_MASK) == (last_state & BUTTON1_MASK)) {
		debounced_state = (debounced_state & ~BUTTON1_MASK) | (actual_state & BUTTON1_MASK);
	}
	
	if ((actual_state & BUTTON2_MASK) == (last_state & BUTTON2_MASK)) {
		debounced_state = (debounced_state & ~BUTTON2_MASK) | (actual_state & BUTTON2_MASK);
	}
}

uint8_t button1_pressed(void) {
	return !(debounced_state & BUTTON1_MASK);
}

uint8_t button2_pressed(void) {
	return !(debounced_state & BUTTON2_MASK);
}