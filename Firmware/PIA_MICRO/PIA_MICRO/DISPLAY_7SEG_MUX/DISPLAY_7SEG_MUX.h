/*
 * DISPLAY_7SEG_MUX.h
 *
 * Created: 17/05/2025 03:33:11 p. m.
 *  Author: fervi
 */ 


#ifndef DISPLAY_7SEG_MUX_H_
#define DISPLAY_7SEG_MUX_H_

#include <avr/io.h>

// Definiciones de pines
#define display_7SEG_DDRX   DDRD
#define display_7SEG_PORTX  PORTD

#define display_mux_DDRX    DDRC
#define display_mux_PORTX   PORTC

// Pines de control de dígitos (ajustar según conexión física)
#define DIG1 PC0
#define DIG2 PC1
#define DIG3 PC2
#define DIG4 PC3

// Prototipos de funciones
void init_display_mux(void);
void timer1_init(void);
void update_display(uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4);
void display_show_HI(void);

#endif /* DISPLAY_7SEG_MUX_H_ */