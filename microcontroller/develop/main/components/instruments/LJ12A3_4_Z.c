/*
 * LJ12A3_4_Z.c
 *
 *  Created on: 27/12/2025
 *      Author: Lotfi Dalal
 */

#include "LJ12A3_4_Z.h"
#include "driver/gpio.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include <time.h>

const char *TAG_LJ12A3 = "LJ12A3";

static gpio_num_t LJ12A3_GPIO;
static bool interrupt_enabled = true, Target_detected = false;
static TaskHandle_t LJ12A3_Loop_handle = NULL;
static func_callback_t callback = NULL;


static void LJ12A3_task_Loop(void * pvParameters){
	// No usar printf o ESP_LOGx, consumen mucha RAM
	while(1){
		uint32_t count = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		
		if(count > 0){
			vTaskDelay(pdMS_TO_TICKS(50));
			
			bool detected = (bool)gpio_get_level(LJ12A3_GPIO);
			
			if(detected == Target_detected){
				if(callback){
					callback();
				}
			}

		}
	}
}

static void IRAM_ATTR LJ12A3_Inter_handler(void* args){
	Target_detected = (bool)gpio_get_level(LJ12A3_GPIO);
	vTaskNotifyGiveFromISR(LJ12A3_Loop_handle, NULL);
}

void LJ12A3_create(gpio_num_t gpio, bool enable_intr){
//	ESP_LOGI(TAG_LJ12A3,"Creating component...");
	LJ12A3_GPIO = gpio;
	interrupt_enabled = enable_intr;
	
	ESP_ERROR_CHECK(gpio_set_direction(LJ12A3_OUT, GPIO_MODE_INPUT));
	
	if (interrupt_enabled){
//		ESP_LOGI(TAG_LJ12A3,"Enabling interrupt and task...");
		ESP_ERROR_CHECK(gpio_set_intr_type(LJ12A3_GPIO, GPIO_INTR_ANYEDGE));
		ESP_ERROR_CHECK(gpio_install_isr_service(0)); // Install the isr service as default	
		ESP_ERROR_CHECK(gpio_isr_handler_add(LJ12A3_GPIO, LJ12A3_Inter_handler, NULL)); // Link the function to the pin (when the interrupt is on)
		ESP_ERROR_CHECK(gpio_intr_enable(LJ12A3_GPIO)); // Enable the interrupt
		
		vTaskDelay(pdMS_TO_TICKS(5));
		
		xTaskCreate(LJ12A3_task_Loop,"LJ12A3_Loop", 1536, NULL, 3, &LJ12A3_Loop_handle);
	}
}


void LJ12A3_set_Callback(func_callback_t func_callback){
	callback = func_callback;
}

bool LJ12A3_detects_Object(){
	return !(bool)gpio_get_level(LJ12A3_GPIO);
}




