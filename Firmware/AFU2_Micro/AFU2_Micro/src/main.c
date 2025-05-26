#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "DISPLAY_7SEG_MUX/DISPLAY_7SEG_MUX.h"

// Definición de pines
#define BUZZER_PIN  PC5    // Buzzer debe permanecer en PC5
#define LED_PIN     PB3    // LED en PB3 (OC2A)
#define POT_PIN     PC0    // ADC0
#define BUTTON0_PIN PB0    // PCINT0
#define BUTTON1_PIN PB1    // PCINT1

// Estados del sistema
typedef enum {
	STATE_IDLE,
	STATE_START,
	STATE_HOLD
} SystemState;

// Variables globales
volatile SystemState currentState = STATE_IDLE;
volatile uint16_t adcValue = 0;
volatile uint8_t button0Pressed = 0;
volatile uint8_t button1Pressed = 0;
volatile uint16_t holdPosition = 0;
volatile uint32_t millis_count = 0;

// Prototipos de funciones
void init_adc(void);
void init_pwm(void);
void init_interrupts(void);
uint16_t read_adc(void);
void set_led_brightness(uint8_t brightness);
void set_buzzer_tone(uint16_t frequency);
void show_welcome_message(void);
void show_position_value(uint16_t position);
uint32_t millis(void);

// Timer0 overflow interrupt para millis()
ISR(TIMER0_OVF_vect) {
	millis_count++;
}

uint32_t millis() {
	uint32_t m;
	cli();
	m = millis_count;
	sei();
	return m;
}

int main(void) {
	// Inicialización de hardware
	init_display_mux();  // Inicializar display
	init_adc();
	init_pwm();
	init_interrupts();
	
	// Configurar pines
	DDRB |= (1 << LED_PIN);      // LED como salida
	DDRC |= (1 << BUZZER_PIN);   // Buzzer como salida
	
	// Configurar Timer0 para millis() (prescaler 64)
	TCCR0B = (1 << CS01) | (1 << CS00);
	TIMSK0 = (1 << TOIE0);
	
	// Mostrar mensaje de bienvenida
	set_led_brightness(100);
	show_welcome_message();
	
	// Habilitar interrupciones globales
	sei();
	
	while(1) {
		switch(currentState) {
			case STATE_IDLE:
			if(button0Pressed) {
				button0Pressed = 0;
				currentState = STATE_START;
			}
			break;
			
			case STATE_START:
			if(button0Pressed) {
				button0Pressed = 0;
				currentState = STATE_IDLE;
				show_welcome_message();
				set_led_brightness(0);
				set_buzzer_tone(0);  // Apagar buzzer
				break;
			}
			
			if(button1Pressed) {
				currentState = STATE_HOLD;
				holdPosition = ((uint32_t)adcValue * 100) / 1023;
				button1Pressed = 0;
				break;
			}
			
			adcValue = read_adc();
			uint16_t position = ((uint32_t)adcValue * 100) / 1023;
			
			show_position_value(position);
			set_led_brightness(position);
			set_buzzer_tone(100 + (position * 10));
			break;
			
			case STATE_HOLD:
			show_position_value(holdPosition);
			set_led_brightness(holdPosition);
			set_buzzer_tone(100 + (holdPosition * 10));
			
			if(button1Pressed) {
				currentState = STATE_START;
				button1Pressed = 0;
			}
			break;
		}
		_delay_ms(50);
	}
}

void init_adc(void) {
	ADMUX = (1 << REFS0);  // AVcc como referencia
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Prescaler 128
}

uint16_t read_adc(void) {
	ADMUX = (1 << REFS0) | (POT_PIN & 0x07);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADC;
}

void init_pwm(void) {
	// Configurar Timer2 para LED (PB3 - OC2A)
	TCCR2A = (1 << COM2A1) | (1 << WGM21) | (1 << WGM20); // Fast PWM
	TCCR2B = (1 << CS22);  // Prescaler 64
	
	// Configurar Timer1 para buzzer en PC5 (usando OC1A redirigido)
	// Solución alternativa para PC5 sin PWM hardware:
	// Usaremos Timer1 en modo CTC para toggle manual
	TCCR1A = 0;
	TCCR1B = (1 << WGM12) | (1 << CS11); // CTC mode, prescaler 8
	OCR1A = 100; // Valor inicial
}

void set_led_brightness(uint8_t brightness) {
	OCR2A = (brightness * 255) / 100;
}

void set_buzzer_tone(uint16_t frequency) {
	if(frequency == 0) {
		TCCR1B &= ~(1 << CS11); // Detener timer
		PORTC &= ~(1 << BUZZER_PIN); // Asegurar pin bajo
		} else {
		// Calcular valor OCR1A para la frecuencia deseada
		OCR1A = (F_CPU / (2UL * 8 * frequency)) - 1;
		TCCR1B |= (1 << CS11); // Iniciar timer
		
		// En la ISR del Timer1 haremos el toggle manual
	}
}

// ISR para Timer1 Compare Match A
ISR(TIMER2_COMPA_vect) {
	PORTC ^= (1 << BUZZER_PIN); // Alternar estado del buzzer
}

void init_interrupts(void) {
	// Configurar interrupciones por cambio de pin para botones
	PCICR |= (1 << PCIE0);
	PCMSK0 |= (1 << PCINT0) | (1 << PCINT1);
	
	// Habilitar interrupción para Timer1 Compare Match A
	TIMSK1 |= (1 << OCIE1A);
}

ISR(PCINT0_vect) {
	static uint32_t lastDebounceTime0 = 0, lastDebounceTime1 = 0;
	uint32_t now = millis();
	
	if (!(PINB & (1 << BUTTON0_PIN)) && (now - lastDebounceTime0) > 50) {
		button0Pressed = 1;
		lastDebounceTime0 = now;
	}
	
	if (!(PINB & (1 << BUTTON1_PIN)) && (now - lastDebounceTime1) > 50) {
		button1Pressed = 1;
		lastDebounceTime1 = now;
	}
}

void show_welcome_message(void) {
	// Mensaje "HI--"
	update_display(10, 11, 12, 10); 
	_delay_ms(2000);
	update_display(13, 1, 5, 14);   
}

void show_position_value(uint16_t position) {
	uint8_t digit1 = position / 100;
	uint8_t digit2 = (position / 10) % 10;
	uint8_t digit3 = position % 10;
	
	update_display(digit1, digit2, digit3, 16); // Mostrar guión en el 4to dígito
}