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
//#include "esp_attr.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "hal/gpio_types.h"
#include "portmacro.h"

#define SCK_HX711			GPIO_NUM_18
#define DOUT_HX711			GPIO_NUM_5

#define SEL_IN_A_GAIN_128	25
#define SEL_IN_B_GAIN_32	26
#define SEL_IN_A_GAIN_64	27
//#define SEL_DEF_CHN_GAIN	0
//#define SEL_LST_CHN_GAIN	1
#define DEF_CHN_GAIN		SEL_IN_A_GAIN_128

#define SAMPLES_PER_SECOND	80

static const char *TAG_Process = "Process", *TAG_HX711 = "HX711";
static uint8_t Last_Sel_Chn_Gain = DEF_CHN_GAIN;

//#if DEF_CHN_GAIN == SEL_IN_A_GAIN_128
// static uint8_t Last_Gain = 128;
//#elif DEF_CHN_GAIN == SEL_IN_B_GAIN_32
// static uint8_t Last_Gain = 32;
//#elif DEF_CHN_GAIN == SEL_IN_A_GAIN_64
// static uint8_t Last_Gain = 64;
//#endif

/* Functions prototypes */

void set_Power_Down_HX711(bool Power_Down);				
int32_t read_Response_HX711(uint8_t* Next_Chn_Gain);					// Return the ADC value, and get the configuration of the next sample
float convert_To_Voltage_HX711(int32_t* value);								 
void get_Bougth_Channels_HX711(float* Volt_Channels, bool Chn_A_gain);	// Volt_Channels shall an vector of size 2, [0] = chn A, [1] = chn B. The result will be inside of this variable || Chn_A_gain is used to know the channel A gain desired, true = 128, false = 64


void app_main(void)
{
	
    /*GPIO init*/
    gpio_set_direction(SCK_HX711, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_direction(DOUT_HX711, GPIO_MODE_INPUT);
//	gpio_set_intr_type(DOUT_HX711, GPIO_INTR_NEGEDGE);
//	gpio_install_isr_service(0 );
//	gpio_isr_handler_add(DOUT_HX711, Redy_Sample, NULL);
//	gpio_intr_enable(DOUT_HX711);

	ESP_LOGI(TAG_Process, "GPIOs SCK and DOUT configured!");
	
	float Voltages[2] = {0.0, 0.0};
	uint8_t counter = 0, pulses = 27;
	int32_t response = 0, response_AB[2] = {0.0, 0.0};
	
    while (true) {
		
		get_Bougth_Channels_HX711(Voltages, true);
		printf("Readed values:\n\tChannel A: %.9f V\n\tChannel B: %.9f V\n\n", Voltages[0], Voltages[1]);
		
		vTaskDelay(pdMS_TO_TICKS(1000));
	
//		if(!gpio_get_level(DOUT_HX711)){
//			if(pulses == SEL_IN_A_GAIN_128){
//				pulses = SEL_IN_B_GAIN_32;
//				response = read_Response_HX711(&pulses);
//				response_AB[0] = response; 
//				Voltages[0] += convert_To_Voltage_HX711(&response);
////				ESP_LOGI(TAG_HX711, "obtained response CHA: %ld", (long)response_AB[0]);
//			}else{
//				pulses = SEL_IN_A_GAIN_128;
//				response = read_Response_HX711(&pulses);
//				response_AB[1] = response;
//				Voltages[1] += convert_To_Voltage_HX711(&response);
////				ESP_LOGI(TAG_HX711, "obtained response CHB: %ld", (long)response_AB[1]);
//			}
//			counter ++;
//			if(counter == 40){
//				Voltages[0] = Voltages[0]/(counter/2);
//				Voltages[1] = Voltages[1]/(counter/2);
//				printf("Readed values:\n\tChannel A: %.9f V\n\tChannel B: %.9f V\n\n", Voltages[0], Voltages[1]);
//				Voltages[0] = 0.0;
//				Voltages[1] = 0.0;
//				counter = 0;
//			}
//		}
//		
//		vTaskDelay(pdMS_TO_TICKS(3));
		
    }
}


/* Functions definitions */

void set_Power_Down_HX711(bool Power_Down){
	gpio_set_level(SCK_HX711, Power_Down);
	
	if (Power_Down){
		esp_rom_delay_us(100);
		ESP_LOGI(TAG_HX711, "Powered down!");
	}else{
		ESP_LOGI(TAG_HX711, "Powered up!");
	}
}

int32_t read_Response_HX711(uint8_t* Next_Chn_Gain){
	
	if(gpio_get_level(SCK_HX711)){
		ESP_LOGE(TAG_HX711, "Is still in Powered Down mode!");
		return 0;
	}
	
	int32_t response = 0;
	uint8_t sign = 0;
	
	for(uint8_t i = 0; i < *Next_Chn_Gain; i++){
		gpio_set_level(SCK_HX711, 1);
		esp_rom_delay_us(10);
		gpio_set_level(SCK_HX711, 0);
		esp_rom_delay_us(10);
		if(i < 24){
			response =  response << 1;
			if (gpio_get_level(DOUT_HX711)){
				response = response | 0x00000001;
			}
		}
	}
	
	sign = response >> 23;  
	
	if(sign > 0){
		response = response | 0xFF000000;
	}	
	
	Last_Sel_Chn_Gain = *Next_Chn_Gain;
	
	return response;
}

float convert_To_Voltage_HX711(int32_t* value){
	float Voltage = 0.0;
	
	Voltage = (float) *value;
	
	Voltage = (Voltage / 15728639) * 3.3;
	
	return Voltage;
}

void get_Bougth_Channels_HX711(float* Volt_Channels, bool Chn_A_gain){
//	if(gpio_get_level(SCK_HX711)){
//		set_Power_Down_HX711(false);	
//		vTaskDelay(pdMS_TO_TICKS(1000));
//	}
	
	int32_t response = 0;
	uint8_t next_chn_gain = 0;
//	float Convertion_Time = 0;
	
//	#if SAMPLES_PER_SECOND == 80
//		Convertion_Time = 13;
//	#elif SAMPLES_PER_SECOND == 10
//		Convertion_Time = 110;
//	#else
//		#error "RES_SELECT is not set to a known resolution!"
//	#endif
	
	if(Chn_A_gain){
		next_chn_gain = SEL_IN_A_GAIN_128;
	}else{
		next_chn_gain = SEL_IN_A_GAIN_64;
	}
	
	while(gpio_get_level(DOUT_HX711)){
		vTaskDelay(pdMS_TO_TICKS(3));
	}
	
	read_Response_HX711(&next_chn_gain);
//	vTaskDelay(pdMS_TO_TICKS(Convertion_Time));

	while(gpio_get_level(DOUT_HX711)){
		vTaskDelay(pdMS_TO_TICKS(3));
	}
	
	next_chn_gain = SEL_IN_B_GAIN_32;
	response = read_Response_HX711(&next_chn_gain);
	ESP_LOGI(TAG_HX711, "obtained response: %ld", (long)response);
	Volt_Channels[0] = convert_To_Voltage_HX711(&response);
	
//	vTaskDelay(pdMS_TO_TICKS(Convertion_Time));

	while(gpio_get_level(DOUT_HX711)){
		vTaskDelay(pdMS_TO_TICKS(3));
	}
	
	response = read_Response_HX711(&next_chn_gain);
	ESP_LOGI(TAG_HX711, "obtained response: %ld", (long)response);
	Volt_Channels[1] = convert_To_Voltage_HX711(&response);	

//	set_Power_Down_HX711(true);
}
