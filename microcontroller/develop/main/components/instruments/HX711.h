/*
 * HX711.h
 *
 *  Created on: 27/12/2025
 *      Author: Lotfi Dalal
 */

#ifndef HX711_H_
#define HX711_H_

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <inttypes.h>
#include "freertos/projdefs.h"
#include "rom/uart.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "hal/gpio_types.h"
#include "portmacro.h"
#include "soc/gpio_num.h"

#define SCK_HX711			GPIO_NUM_18
#define DOUT_HX711			GPIO_NUM_5

#define HX711_SEL_IN_A_GAIN_128	25
#define HX711_SEL_IN_B_GAIN_32	26
#define HX711_SEL_IN_A_GAIN_64	27
#define HX711_DEF_CHN_GAIN		HX711_SEL_IN_A_GAIN_128

#define HX711_SAMPLES_PER_SECOND	80


/* Functions prototypes */

void HX711_Crate(gpio_num_t gpio_sck, gpio_num_t gpio_dout);
void HX711_set_Power_Down(bool Power_Down);				
int32_t HX711_read_Response(uint8_t* Next_Chn_Gain);					// Return the ADC value, and get the configuration of the next sample
float HX711_convert_To_Voltage(int32_t* value);
int32_t HX711_read_channel_raw(const char channel, uint8_t pga_gain);								 
void HX711_get_Bougth_Channels(float* Volt_Channels, bool Chn_A_gain);	// Volt_Channels shall an vector of size 2, [0] = chn A, [1] = chn B. The result will be inside of this variable || Chn_A_gain is used to know the channel A gain desired, true = 128, false = 64




#endif /* HX711_H_ */
