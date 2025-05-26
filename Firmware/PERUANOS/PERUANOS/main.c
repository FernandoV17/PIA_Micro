/*
 * PERUANOS.c
 * 
 * Control lineal de LED (PB3) y buzzer (PB2) con potenciómetro (PC4)
 * Ambos responden con el mismo valor PWM
 */

#include <avr/io.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include <avr/interrupt.h>
#include "DISPLAY_7SEG_MUX/DISPLAY_7SEG_MUX.h"

// Definición de pines
#define POT_PIN PINC4      // Potenciómetro en PC4
#define LED_PIN PINB3      // LED en PB3 (OC2A)
#define BUZZER_PIN PINB2   // Buzzer en PB2 (OC1B)

void setup(void);
void loop(void);
uint16_t read_pot(void);
void control_outputs(void);
void update_display(uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4);
void init_display_mux(void);
void timer9_init(void);

int main(void) {
	cli();
    setup();
	sei();
	update_display(10,11,12,10);
	_delay_ms(3000);
	update_display(13,1,5,14);

	
    while (1) {
        loop();
    }
}

void setup(void){
    DDRB |= (1 << LED_PIN);
    TCCR2A = (1 << COM2A1) | (1 << WGM21) | (1 << WGM20);
    TCCR2B = (1 << CS20);  // Sin prescaler
    
    DDRB |= (1 << BUZZER_PIN);
	TCCR1A = (1 << COM1B1) | (1 << WGM11) | (1 << WGM10);
	TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);  // Prescaler 8
	ICR1 = 1024;  
    
    ADMUX = (1 << REFS0) | (1 << MUX2);  // AVcc ref, ADC4
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

	PORTB |= (1 << BUZZER_PIN);  
	_delay_ms(500);
	PORTB &= ~(1 << BUZZER_PIN); 

	timer9_init();
	init_display_mux();
}

uint16_t read_pot(void){
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}

void control_outputs(void){
    uint16_t pot_value = read_pot();
    
    // Mismo valor para ambos (ajustado a cada PWM)
    OCR2A = pot_value / 4;       // 8-bit (0-255)
    OCR1B = pot_value;           // 10-bit (0-1023)
    
    if(pot_value < 5) {
        OCR2A = 0;
        OCR1B = 0;
	
//	update_display(	13,	pot_value%10, pot_value%100, pot_value%1000);
    }
    
    _delay_ms(20);
}

void loop(void){
    control_outputs();
}
