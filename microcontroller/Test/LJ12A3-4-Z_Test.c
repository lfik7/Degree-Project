#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <driver/gpio.h>
#include <hal/gpio_types.h>
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h"
#include "portmacro.h"
#include "esp_err.h"
#include "esp_log.h"
#include "soc/gpio_num.h"


#define LJ12A3_OUT		GPIO_NUM_33

const char *TAG_LJ12A3 = "LJ12A3", *TAG_Process = "Process";

void app_main(void)
{
	ESP_LOGI(TAG_Process, "Program starting...");
	/* GPIO initialize */
	gpio_set_direction(LJ12A3_OUT, GPIO_MODE_INPUT);
	gpio_pulldown_dis(LJ12A3_OUT);
	
	ESP_LOGI(TAG_LJ12A3, "Input done!");
	
	ESP_LOGI(TAG_Process, "Initialization completed!");
	
	vTaskDelay(pdMS_TO_TICKS(1000));
	
	int LJ12A3_val = 0;
	
    while (true) {
		
		LJ12A3_val = gpio_get_level(LJ12A3_OUT);
		if(LJ12A3_val == 1){
        	printf("No se detecta objecto!\n");
		}else{
        	printf("Objeto detectado!\n");	
		}
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
