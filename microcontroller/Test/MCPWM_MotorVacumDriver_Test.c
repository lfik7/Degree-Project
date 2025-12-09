/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "hal/mcpwm_types.h"
//#include "driver/mcpwm_prelude.h"	// The original comes wit this
#include <driver/mcpwm_timer.h>		// Added by user
#include <driver/mcpwm_oper.h>		// Added by user
#include <driver/mcpwm_cmpr.h>		// Added by user
#include <driver/mcpwm_gen.h>		// Added by user

static const char *TAG = "example";


#define GPIO_OUTPUT        		25        	// GPIO connects to the PWM signal line
#define TIMEBASE_RESOLUTION_HZ 	1000000  	// 1MHz, 1us per tick
#define TIMEBASE_PERIOD        	1000    	// 20000 ticks, 20ms


void app_main(void)
{
    ESP_LOGI(TAG, "Create timer and operator");
    mcpwm_timer_handle_t timer = NULL;
    mcpwm_timer_config_t timer_config = {
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = TIMEBASE_RESOLUTION_HZ,
        .period_ticks = TIMEBASE_PERIOD,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    };
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer));

    mcpwm_oper_handle_t oper = NULL;
    mcpwm_operator_config_t operator_config = {
        .group_id = 0, // operator must be in the same group to the timer
    };
    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &oper));

    ESP_LOGI(TAG, "Connect timer and operator");
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper, timer));

    ESP_LOGI(TAG, "Create comparator and generator from the operator");
    mcpwm_cmpr_handle_t comparator = NULL;
    mcpwm_comparator_config_t comparator_config = {
        .flags.update_cmp_on_tez = true,
    };
    ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &comparator_config, &comparator));

    mcpwm_gen_handle_t generator = NULL;
    mcpwm_generator_config_t generator_config = {
        .gen_gpio_num = GPIO_OUTPUT,
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(oper, &generator_config, &generator));

    ESP_LOGI(TAG, "Set generator action on timer and compare event");
    // go high on counter empty
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator,
                                                              MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    // go low on compare threshold
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator,
                                                                MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator, MCPWM_GEN_ACTION_LOW)));

    ESP_LOGI(TAG, "Enable and start timer");
    ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));

   	uint16_t value = 10;
   	
    while (1) {
        //ESP_LOGI(TAG, "value: %d", 0);
        ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, value));
        //Add delay, since it takes time for servo to rotate, usually 200ms/60degree rotation under 5V power supply
        vTaskDelay(pdMS_TO_TICKS(100));
        value += 10;
        if(value >= 1000){
			/* The next code is for deleting the mcpwm module, in case the engineer wants to do it,
			** for example, if wants to delete because the module wont be used in long time and need
			** resources, but might be better not doing, for the process of allocate and deallocate. 
			 */
			/*ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_STOP_EMPTY));
			value = 0;
			ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, value));
			
			vTaskDelay(pdMS_TO_TICKS(10));
			
			ESP_ERROR_CHECK(mcpwm_timer_disable(timer));
			ESP_ERROR_CHECK(mcpwm_del_timer(timer));
			ESP_ERROR_CHECK(mcpwm_del_comparator(comparator));
			ESP_ERROR_CHECK(mcpwm_del_generator(generator));
			ESP_ERROR_CHECK(mcpwm_del_operator(oper));
			
			vTaskDelay(pdMS_TO_TICKS(5000));
			
			ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer));
		    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &oper));
		    ESP_LOGI(TAG, "Connect timer and operator");
		    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper, timer));
		    ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &comparator_config, &comparator));
		    ESP_ERROR_CHECK(mcpwm_new_generator(oper, &generator_config, &generator));
		    ESP_LOGI(TAG, "Set generator action on timer and compare event");
		    // go high on counter empty
		    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator,
		                                                              MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
		    // go low on compare threshold
		    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator,
		                                                                MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator, MCPWM_GEN_ACTION_LOW)));
		    ESP_LOGI(TAG, "Enable and start timer");
		    ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
		    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));*/
		    
		    value = 0;
			ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, value));
			
			vTaskDelay(pdMS_TO_TICKS(5000));
			value = 10;
		}
    }
}