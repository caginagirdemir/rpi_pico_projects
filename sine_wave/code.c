#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

//DDS parameters
#define two32 4294967296.0
#define Fs 40000

//the DDS units
volatile unsigned int phase_accum_main;
volatile unsigned int phase_incr_main = (800.0*two32)/Fs;

//SPI data
uint16_t DAC_data; // output value

//DAC parameters
// A-channel
#define DAC_config_chan_A 0b0011000000000000
// B-channel
#define DAC_config_chan_B 0b1011000000000000

//SPI configurations
#define PIN_MISO  4
#define PIN_CS    5
#define PIN_SCK   6
#define PIN_MOSI  7
#define SPI_PORT  spi0

//DDS sine table
#define sine_table_size 256
volatile int sin_table[sine_table_size];

//Timer ISR
bool repeating_timer_callback(struct repeating_timer *t)
{
  //DDS phase and sine table lookup
  phase_accum_main += phase_incr_main;
  DAC_data = (DAC_config_chan_A | ((sin_table[phase_accum_main>>24] + 2048) & 0xffff)) ;
  spi_write16_blocking(SPI_PORT, &DAC_data, 1);
  return true;
}

int main()
{
  //Initialize stdio
  stdio_init_all();
  printf("Hello, DAC!\n");

  //Initialize SPI channel (channel, baud rate set to 20MHz)
  spi_init(SPI_PORT, 20000000);
  //Format (channel, data bits per transfer, polarity, phase, order)
  spi_set_format(SPI_PORT, 16, 0, 0, 0);

  //Map SPI signals to GPIO ports
  gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
  gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
  gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
  gpio_set_function(PIN_CS, GPIO_FUNC_SPI);

  //build the sine lookup table
  //scaled to produce values between 0 and 4096
  int i;
  for (i = 0; i < sine_table_size; i++)
    sin_table[i] = (int)(2047*sin((float)i*6.283/(float)sine_table_size));
  
  struct repeating_timer timer;

  //Create a repeating timer that calls repeating_timer_callback.
  //If the delay is > 0 then this is the delay between the previous callback endingand the next starting
  //If the delay is negative then the next call to the callback will be exactly x us after the start of the call to the last callback

  //Negative delay so means we will call repeating_timer_callback, and call it again
  //25us (40kHz) later regardless of how long the callback took to execute
  add_repeating_timer_us(-25, repeating_timer_callback, NULL, &timer);
  while(1)
  {}
  return 0;
}