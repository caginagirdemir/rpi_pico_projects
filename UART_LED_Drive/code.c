#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware/i2c.h"

#define LED_PIN 28
#define UART_ID uart0
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

volatile bool flag = false;

volatile uint16_t duration = 250;

char *str;

#define UART_TX_PIN 0
#define UART_RX_PIN 1

io_rw_32 *reg; // rx interrupt flag

struct repeating_timer timer;

bool repeating_timer_callback(struct repeating_timer *t);

// RX interrupt handler
void on_uart_rx() {
  int i = -1; //counter
  while(uart_is_readable(UART_ID)) {
    char ch = uart_getc(UART_ID); //get char from rx fifo
    str[++i] = ch; //fill str
    busy_wait_ms(1); //wait transmission
    uart_putc(UART_ID, ch); //echo
  }
  str[i] = '\0';
  hw_clear_bits(reg, 0x10); //put rx_irq down by manual because tool doesn't simulate it
  if(!strcmp(str, "stop"))
    cancel_repeating_timer(&timer);
  if(!strncmp(str, "set=", 4))
  {
    cancel_repeating_timer(&timer);
    char *dur_str;
    dur_str = (char *)malloc(10);
    dur_str = memchr(str, '=', strlen(str));
    dur_str++;
    uart_puts(UART_ID, dur_str);
    duration = atoi(dur_str);
    add_repeating_timer_ms(-duration, repeating_timer_callback, NULL, &timer);
  }
}

bool repeating_timer_callback(struct repeating_timer *t)
{
  gpio_put(LED_PIN, !(gpio_get(LED_PIN)));
  return 1;
}

int main() {
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    reg = (uint32_t *)0x40034044; //register address of interrupt flag reset
    
    str = (char *)malloc(10);
    uart_init(UART_ID, 9600);

    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    uart_set_fifo_enabled(UART_ID, true);
    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
    int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(UART0_IRQ, on_uart_rx);
    irq_set_enabled(UART0_IRQ, true);
    uart_set_irq_enables(UART_ID, true, false);
    uart_puts(UART_ID, "Subscribe my channel\n");
    uart_puts(UART_ID, "commands->stop/set=<duration_in_ms>\n");
    add_repeating_timer_ms(-duration, repeating_timer_callback, NULL, &timer);
    while (1)
    {
      tight_loop_contents();
    }
}