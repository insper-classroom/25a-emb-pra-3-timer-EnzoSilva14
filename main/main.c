#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"

const int PIN_TRIGGER = 16;
const int PIN_ECHO = 17;

volatile uint32_t ti = 0;
volatile uint32_t tf = 0;
volatile bool cabou_tempo = false;

bool iniciar_leitura = false;

void echo_callback(uint gpio, uint32_t events) {
    if (events & GPIO_IRQ_EDGE_RISE) {
        ti = to_us_since_boot(get_absolute_time()); 
    } else if (events & GPIO_IRQ_EDGE_FALL) {
        tf = to_us_since_boot(get_absolute_time()); 
    }
}

float calcular_distancia(uint32_t duracao) {
    return (duracao / 2.0) * 0.0343; 
}

int64_t callback_alarme(alarm_id_t id, void *user_data) {
    cabou_tempo = true;
    return 0;
}

void inicializar_rtc() {
    datetime_t t = {
        .year = 2023,
        .month = 3,
        .day = 15,
        .dotw = 3,
        .hour = 12,
        .min = 0,
        .sec = 0
    };
    rtc_init();
    rtc_set_datetime(&t);
}

void imprimir_medicao(float distancia) {
    datetime_t t;
    rtc_get_datetime(&t);
    printf("%02d:%02d:%02d - %.2f cm\n", t.hour, t.min, t.sec, distancia);

}

int main() {
    stdio_init_all();

    gpio_init(PIN_TRIGGER);
    gpio_set_dir(PIN_TRIGGER, GPIO_OUT);
    gpio_init(PIN_ECHO);
    gpio_set_dir(PIN_ECHO, GPIO_IN);

    gpio_set_irq_enabled_with_callback(PIN_ECHO, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &echo_callback);

    inicializar_rtc();
    alarm_id_t alarme;

    while (true) {
        int c = getchar_timeout_us(100); 
       
        if (c == 'S') {  
            iniciar_leitura = true;
            printf("Iniciando leituras...\n");
        } else if (c == 'P') {  
            iniciar_leitura = false;
            printf("Parando leituras...\n");
        }
        
        if (iniciar_leitura) {
            ti = 0;
            tf = 0;
            cabou_tempo = false;
            
            gpio_put(PIN_TRIGGER, 1);
            sleep_us(10);
            gpio_put(PIN_TRIGGER, 0);
            
            alarme = add_alarm_in_ms(1000, callback_alarme, NULL, false);
            
            while (tf == 0 && !cabou_tempo) {
                tight_loop_contents();
            }

            if (cabou_tempo) {
                datetime_t t;
                rtc_get_datetime(&t);
                printf("%02d:%02d:%02d - Falha\n", t.hour, t.min, t.sec);
            } else {
                cancel_alarm(alarme);
                uint32_t duracao = tf - ti;
                float distancia = calcular_distancia(duracao);
                imprimir_medicao(distancia);
            }
            
            sleep_ms(500);
        } else {
            sleep_ms(100);
        }
    }
}