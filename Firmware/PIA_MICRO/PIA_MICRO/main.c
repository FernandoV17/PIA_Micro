/*
 * PIA_MICRO.c
 *
 * Created: 17/05/2025 03:24:13 p. m.
 * Author : fervi
 */ 

#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "DISPLAY_7SEG_MUX/DISPLAY_7SEG_MUX.h"
#include "BUTTON/BUTTON.h"

// Definiciones de hardware
#define LED_PORT     PORTB
#define LED_DDR      DDRB
#define LED_PIN      PB3

#define BUZZER_PORT  PORTC
#define BUZZER_DDR   DDRC
#define BUZZER_PIN   PC5

#define POT_ADC_CH   4  // ADC4 (PC4)

// Umbrales para las alertas
#define WARNING_THRESHOLD  20  // 20%
#define ALARM_THRESHOLD    80  // 80%
#define BUZZER_FREQ_LOW    500 // Hz
#define BUZZER_FREQ_HIGH   1000 // Hz

// Estados del sistema
typedef enum {
	SYSTEM_INIT,
	SYSTEM_READY,
	SYSTEM_HOLD
} system_state_t;

// Variables globales
volatile uint16_t adc_value = 0;
volatile uint8_t display_value = 0;
volatile uint8_t alarm_silenced = 0;
volatile system_state_t system_state = SYSTEM_INIT;
volatile uint8_t hold_value = 0;

// Prototipos de funciones locales
void init_ports(void);
void init_adc(void);
uint16_t read_adc(uint8_t channel);
void update_indicators(uint8_t value);
void buzzer_tone(uint16_t frequency, uint16_t duration_ms);
void display_percentage(uint8_t percentage);
void system_reset(void);

int main(void) {
	// Inicialización de hardware
	init_ports();
	init_adc();
	init_display_mux();
	timer1_init();
	buttons_init();
	
	// Mensaje inicial
	display_show_HI();
	
	// Habilitar interrupciones globales
	sei();
	
	while(1) {
		// Actualizar estado de botones
		buttons_update();
		
		// Máquina de estados del sistema
		switch(system_state) {
			case SYSTEM_INIT:
			if(button1_pressed()) {
				system_state = SYSTEM_READY;
			}
			break;
			
			case SYSTEM_READY:
			// Leer valor actual del potenciómetro
			adc_value = read_adc(POT_ADC_CH);
			display_value = (uint8_t)(((uint32_t)adc_value * 100) / 1023);
			
			// Manejar botón 2 (HOLD)
			if(button2_pressed()) {
				hold_value = display_value;
				system_state = SYSTEM_HOLD;
			}
			
			// Manejar botón 3 (RESET)
			if(button3_pressed()) {
				system_reset();
			}
			
			// Actualizar display
			display_percentage(display_value);
			break;
			
			case SYSTEM_HOLD:
			// Mostrar valor congelado
			display_percentage(hold_value);
			
			// Manejar botón 2 (sacar de HOLD)
			if(button2_pressed()) {
				system_state = SYSTEM_READY;
			}
			
			// Manejar botón 3 (RESET)
			if(button3_pressed()) {
				system_reset();
			}
			break;
		}
		
		// Control de alarmas (excepto en estado INIT)
		if(system_state != SYSTEM_INIT && !alarm_silenced) {
			update_indicators(display_value);
		}
		
		// Manejar silencio de alarmas con botón 1
		if(button1_pressed() && system_state != SYSTEM_INIT) {
			alarm_silenced = 1;
			BUZZER_PORT &= ~(1 << BUZZER_PIN);
			} else {
			// Reactivar alarmas después de 5 segundos
			static uint16_t counter = 0;
			if(alarm_silenced && ++counter >= 500) {  // 500 * 10ms = 5s
				counter = 0;
				alarm_silenced = 0;
			}
		}
		
		_delay_ms(10);  // Delay para estabilidad
	}
}

void system_reset(void) {
	system_state = SYSTEM_INIT;
	alarm_silenced = 0;
	LED_PORT &= ~(1 << LED_PIN);
	BUZZER_PORT &= ~(1 << BUZZER_PIN);
	display_show_HI();
}

void init_ports(void) {
	// Configurar LED
	LED_DDR |= (1 << LED_PIN);
	LED_PORT &= ~(1 << LED_PIN);
	
	// Configurar buzzer
	BUZZER_DDR |= (1 << BUZZER_PIN);
	BUZZER_PORT &= ~(1 << BUZZER_PIN);
}

void init_adc(void) {
	ADMUX = (1 << REFS0); // AVcc como referencia
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // ADC enable, prescaler 128
}

uint16_t read_adc(uint8_t channel) {
	ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);
	ADCSRA |= (1 << ADSC);
	while(ADCSRA & (1 << ADSC));
	return ADC;
}

void update_indicators(uint8_t value) {
	if(value < WARNING_THRESHOLD) {
		LED_PORT ^= (1 << LED_PIN);
		buzzer_tone(BUZZER_FREQ_LOW, 200);
		_delay_ms(200);
	}
	else if(value > ALARM_THRESHOLD) {
		LED_PORT ^= (1 << LED_PIN);
		buzzer_tone(BUZZER_FREQ_HIGH, 100);
		_delay_ms(100);
	}
	else {
		LED_PORT &= ~(1 << LED_PIN);
		BUZZER_PORT &= ~(1 << BUZZER_PIN);
	}
}

void delay_us_fixed(uint16_t us) {
	us *= 2;
	
	__asm__ __volatile__ (
	"1: sbiw %0,1" "\n\t"
	"brne 1b"
	: "=w" (us)
	: "0" (us)
	);
}

void buzzer_tone(uint16_t frequency, uint16_t duration_ms) {
	if(frequency == 0) return;
	
	uint32_t period_us = 1000000UL / frequency;
	uint16_t half_period_us = period_us / 2;
	uint32_t cycles = (duration_ms * 1000UL) / period_us;
	
	for(uint32_t i = 0; i < cycles; i++) {
		BUZZER_PORT |= (1 << BUZZER_PIN);
		delay_us_fixed(half_period_us);  
		BUZZER_PORT &= ~(1 << BUZZER_PIN);
		delay_us_fixed(half_period_us);  
	}
}

void display_percentage(uint8_t percentage) {
	uint8_t d1 = percentage / 100;
	uint8_t d2 = (percentage / 10) % 10;
	uint8_t d3 = percentage % 10;
	uint8_t d4 = 12;  // Apagado
	
	update_display(d1, d2, d3, d4);
}