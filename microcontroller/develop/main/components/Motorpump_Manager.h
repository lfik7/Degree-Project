/*
 * Motobomba_Manager.h
 *
 *  Created on: 27/12/2025
 *      Author: Lotfi Dalal
 */

#ifndef MOTOBOMBA_MANAGER_H_
#define MOTOBOMBA_MANAGER_H_

/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "hal/mcpwm_types.h"
#include "Globals.h"
//#include "driver/mcpwm_prelude.h"	// The original comes wit this
#include <driver/mcpwm_timer.h>		// Added by user
#include <driver/mcpwm_oper.h>		// Added by user
#include <driver/mcpwm_cmpr.h>		// Added by user
#include <driver/mcpwm_gen.h>		// Added by user


#define MOTOBOMBAN_GPIO_OUTPUT        		25        	// GPIO connects to the PWM signal line
#define MOTOBOMBAN_TIMEBASE_RESOLUTION_HZ 	1000000  	// 1MHz, 1us per tick
#define MOTOBOMBAN_TIMEBASE_PERIOD        	1000    	// 1000 ticks, 1ms


void Motorpump_init(motorpumpState_t* _motorpump_data);
void Motorpump_set_callback(func_callback_t _motorpump_event_callback);
void Motorpump_turn_on();
void Motorpump_turn_off();





#endif /* MOTOBOMBA_MANAGER_H_ */
