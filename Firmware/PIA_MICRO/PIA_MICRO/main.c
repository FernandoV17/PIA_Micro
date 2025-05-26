/*
 * PIA_MICRO.c
 *
 * Created: 17/05/2025 03:24:13 p. m.
 * Author : fervi
 */ 

/*
 * PIA_MICRO.c
 * Sistema de Monitoreo de Posición con Alerta Visual/Sonora
 * Autor: fervi
 */

#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "DISPLAY_7SEG_MUX/DISPLAY_7SEG_MUX.h"
#include "BUTTON/BUTTON.h"

// Configuración de hardware
#define LED_PORT     PORTB
#define LED_DDR      DDRB
#define LED_PIN      PB3   // OC2A (PWM)

#define BUZZER_PORT  PORTD
#define BUZZER_DDR   DDRD
#define BUZZER_PIN   PD3   // OC2B (PWM)

#define POT_ADC_CH   0     // ADC0 (PC0)

// Estados del sistema
typedef enum {
    SYSTEM_INIT,    // Estado inicial con mensaje HI
    SYSTEM_READY,   // Monitoreo activo
    SYSTEM_HOLD     // Valor congelado
} system_state_t;

// Variables globales
volatile uint8_t display_value = 0;
volatile system_state_t system_state = SYSTEM_INIT;
volatile uint8_t hold_value = 0;
void display_percentage(uint8_t percentage); 

void init_ports(void);
void init_adc(void);
uint16_t read_adc(void);
void update_pwm(uint8_t value);
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
        buttons_update();
        
        switch(system_state) {
            case SYSTEM_INIT:
                if(button1_pressed()) {
                    system_state = SYSTEM_READY;
                    TCCR2A |= (1 << COM2A1) | (1 << COM2B1);
                }
                break;
                
            case SYSTEM_READY: {
                // Leer y convertir valor del potenciómetro
                uint16_t adc_value = read_adc();
                display_value = (uint8_t)(((uint32_t)adc_value * 100) / 1023);
                
                // Actualizar PWM
                update_pwm(display_value);
                
                // Manejo de botones
                if(button2_pressed()) {
                    hold_value = display_value;
                    system_state = SYSTEM_HOLD;
                    TCCR2A &= ~((1 << COM2A1) | (1 << COM2B1));
                    LED_PORT &= ~(1 << LED_PIN);
                    BUZZER_PORT &= ~(1 << BUZZER_PIN);
                }
                if(button3_pressed()) system_reset();
                
                // Actualizar display
                display_percentage(display_value);
                break;
            }
                
            case SYSTEM_HOLD: {
                static uint16_t blink_counter = 0;
                
                // Parpadeo LED cada 500ms
                if(++blink_counter >= 50) {
                    blink_counter = 0;
                    LED_PORT ^= (1 << LED_PIN);
                }
                
                // Mantener valor en display
                display_percentage(hold_value);
                
                // Manejo de botones
                if(button2_pressed()) {
                    system_state = SYSTEM_READY;
                    TCCR2A |= (1 << COM2A1) | (1 << COM2B1);
                    update_pwm(hold_value);
                }
                if(button3_pressed()) system_reset();
                break;
            }
        }
        _delay_ms(10);
    }
}

void system_reset(void) {
    system_state = SYSTEM_INIT;
    TCCR2A &= ~((1 << COM2A1) | (1 << COM2B1));
    LED_PORT &= ~(1 << LED_PIN);
    BUZZER_PORT &= ~(1 << BUZZER_PIN);
    display_show_HI();
}

void init_ports(void) {
    // Configurar PWM
    LED_DDR |= (1 << LED_PIN);
    BUZZER_DDR |= (1 << BUZZER_PIN);
    
    // Timer2 - Modo PWM Rápido 8-bit
    TCCR2A = (1 << WGM21) | (1 << WGM20); // Fast PWM
    TCCR2B = (1 << CS22);                  // Prescaler 64 (F_PWM = 16MHz/64/256 = 976.56Hz)
    OCR2A = 0;
    OCR2B = 0;
}

void init_adc(void) {
    ADMUX = (1 << REFS0) |                 // Referencia AVcc
            (POT_ADC_CH & 0x0F);          // Canal ADC0
    ADCSRA = (1 << ADEN) |                 // Habilitar ADC
             (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Prescaler 128 (125KHz)
    DIDR0 = (1 << POT_ADC_CH);             // Deshabilitar entrada digital
}

uint16_t read_adc(void) {
    ADCSRA |= (1 << ADSC);                 // Iniciar conversión
    while(ADCSRA & (1 << ADSC));           // Esperar fin de conversión
    return ADC;
}

void update_pwm(uint8_t value) {
    OCR2A = (value * 255UL) / 100;         // LED brillo proporcional
    OCR2B = (value * 255UL) / 100;         // Buzzer volumen proporcional
}

void display_percentage(uint8_t percentage) {
    uint8_t d1 = percentage / 100;
    uint8_t d2 = (percentage / 10) % 10;
    uint8_t d3 = percentage % 10;
    update_display(d1, d2, d3, 12);        // 4to dígito apagado
}