/*
 * HX711.c
 *
 *  Created on: 27/12/2025
 *      Author: Lotfi Dalal
 */

#include "HX711.h"
#include "soc/gpio_num.h"



static const char *TAG_HX711 = "HX711";
static uint8_t Last_Sel_Chn_Gain = HX711_DEF_CHN_GAIN;
static gpio_num_t HX711_GPIO_SCK, HX711_GPIO_DOUT;



void HX711_Crate(gpio_num_t gpio_sck, gpio_num_t gpio_dout){
	HX711_GPIO_SCK = gpio_sck;
	HX711_GPIO_DOUT = gpio_dout;
	
    ESP_ERROR_CHECK(gpio_set_direction(HX711_GPIO_SCK, GPIO_MODE_INPUT_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(HX711_GPIO_DOUT, GPIO_MODE_INPUT));
}

void HX711_set_Power_Down(bool Power_Down){
	gpio_set_level(HX711_GPIO_SCK, Power_Down);
	
	if (Power_Down){
		esp_rom_delay_us(100);
		ESP_LOGI(TAG_HX711, "Powered down!");
	}else{
		ESP_LOGI(TAG_HX711, "Powered up!");
	}
}

int32_t HX711_read_Response(uint8_t* Next_Chn_Gain){
	
	
	int32_t response = 0;
	
	for(uint8_t i = 0; i < *Next_Chn_Gain; i++){
		gpio_set_level(HX711_GPIO_SCK, 1);
		esp_rom_delay_us(2);
		gpio_set_level(HX711_GPIO_SCK, 0);
		esp_rom_delay_us(2);
		if(i < 24){
			response =  response << 1;
			if (gpio_get_level(HX711_GPIO_DOUT)){
				response = response | 0x00000001;
			}
		}
	}
	
	
	if(response & 0x800000){
		response = response | 0xFF000000;
	}	
	
	
	return response;
}

float HX711_convert_To_Voltage(int32_t* value){
	float Voltage = 0.0;
	
	Voltage = (float) *value;
	
	Voltage = (Voltage / 15728639) * 3.3;
	
	return Voltage;
}

int32_t HX711_read_channel_raw(const char channel, uint8_t pga_gain){
	int32_t response = 0;
	uint8_t next_chn_gain = 0;
	if ((channel == 'a') || (channel == 'A')) {
		if (pga_gain == 128){
			next_chn_gain = HX711_SEL_IN_A_GAIN_128;
		} else if (pga_gain == 64) {
			next_chn_gain = HX711_SEL_IN_A_GAIN_64;
		} else {
			ESP_LOGE(TAG_HX711,"PGA value %d invalid for channel A. Will be set channel 128",pga_gain);
			next_chn_gain = HX711_SEL_IN_A_GAIN_128;
		}
	}else if ((channel == 'b') || (channel == 'B') ){
		if (pga_gain != 32){
			ESP_LOGE(TAG_HX711,"PGA value %d invalid for channel B. Will be set channel 32",pga_gain);
		}
		next_chn_gain = HX711_SEL_IN_B_GAIN_32;
	}else {
		ESP_LOGE(TAG_HX711,"Channel %c invalid. Will be set channel A",channel);
	}
	
	if (Last_Sel_Chn_Gain != next_chn_gain){
		while(gpio_get_level(HX711_GPIO_DOUT)){
			vTaskDelay(pdMS_TO_TICKS(3));
		}
		
		HX711_read_Response(&next_chn_gain);
	}
	
	while(gpio_get_level(HX711_GPIO_DOUT)){
		vTaskDelay(pdMS_TO_TICKS(3));
	}
	
	response = HX711_read_Response(&next_chn_gain);
	
	Last_Sel_Chn_Gain = next_chn_gain;
	
	return response;
}

void HX711_get_Bougth_Channels(float* Volt_Channels, bool Chn_A_gain){
	
	int32_t response = 0;
	uint8_t next_chn_gain = 0;
	
	if(Chn_A_gain){
		next_chn_gain = HX711_SEL_IN_A_GAIN_128;
	}else{
		next_chn_gain = HX711_SEL_IN_A_GAIN_64;
	}
	
	while(gpio_get_level(HX711_GPIO_DOUT)){
		vTaskDelay(pdMS_TO_TICKS(3));
	}
	
	HX711_read_Response(&next_chn_gain);

	while(gpio_get_level(HX711_GPIO_DOUT)){
		vTaskDelay(pdMS_TO_TICKS(3));
	}
	
	next_chn_gain = HX711_SEL_IN_B_GAIN_32;
	response = HX711_read_Response(&next_chn_gain);
//	ESP_LOGI(TAG_HX711, "obtained response: %ld", (long)response);
	Volt_Channels[0] = HX711_convert_To_Voltage(&response);
	

	while(gpio_get_level(HX711_GPIO_DOUT)){
		vTaskDelay(pdMS_TO_TICKS(3));
	}
	
	response = HX711_read_Response(&next_chn_gain);
//	ESP_LOGI(TAG_HX711, "obtained response: %ld", (long)response);
	Volt_Channels[1] = HX711_convert_To_Voltage(&response);	

}

