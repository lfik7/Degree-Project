/*
 * LJ12A3-4-Z.h
 *
 *  Created on: 27/12/2025
 *      Author: Lotfi Dalal
 */

#ifndef LJ12A3_4_Z_H_
#define LJ12A3_4_Z_H_

#include <stdbool.h>
#include <unistd.h>
#include <hal/gpio_types.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "portmacro.h"
#include "esp_err.h"
#include "esp_log.h"
#include "soc/gpio_num.h"
#include "Globals.h"


#define LJ12A3_OUT		GPIO_NUM_33


void LJ12A3_create(gpio_num_t gpio, bool enable_intr);
void LJ12A3_set_Callback(func_callback_t func_callback);
bool LJ12A3_detects_Object();




#endif /* MAIN_COMPONENTS_INSTRUMENTS_LJ12A3_4_Z_H_ */
