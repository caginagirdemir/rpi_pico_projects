#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define LED_PIN 28

volatile uint16_t on_duration = 250;
volatile uint16_t off_duration = 250;

struct repeating_timer timer;

static const char *gpio_irq_str[] = {
        "LEVEL_LOW",  // 0x1
        "LEVEL_HIGH", // 0x2
        "EDGE_FALL",  // 0x4
        "EDGE_RISE"   // 0x8
};

static char event_str[128];

//function declarations
void gpio_event_string(char *buf, uint32_t events);

int64_t alarm_callback(alarm_id_t id, void *user_data)
{
  gpio_put(LED_PIN, !gpio_get(LED_PIN));
  return 0;
}

void gpio_callback(uint gpio, uint32_t events) {
    gpio_event_string(event_str, events);
    if(!(strcmp (event_str, "EDGE_RISE")))
    {
      add_alarm_in_ms(on_duration, alarm_callback, NULL, true);
    }
    else if(!(strcmp (event_str, "EDGE_FALL")))
    {
      add_alarm_in_ms(off_duration, alarm_callback, NULL, true);
    }
}

int main() {
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
  gpio_put(LED_PIN, 1);
  gpio_set_irq_enabled_with_callback(LED_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
  add_alarm_in_ms(off_duration, alarm_callback, NULL, false);


  while (true) {
    tight_loop_contents();
  }
}

void gpio_event_string(char *buf, uint32_t events) {
    for (uint i = 0; i < 4; i++) {
        uint mask = (1 << i);
        if (events & mask) {
            // Copy this event string into the user string
            const char *event_str = gpio_irq_str[i];
            while (*event_str != '\0') {
                *buf++ = *event_str++;
            }
        }
    }
    *buf++ = '\0';
}