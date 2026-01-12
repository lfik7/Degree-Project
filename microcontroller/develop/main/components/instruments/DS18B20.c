/*
 * DS18B20.c
 *
 *  Created on: 27/12/2025
 *      Author: Lotfi Dalal
 */

#include "DS18B20.h"
#include "soc/gpio_num.h"



const char*TAG_DS18B20 = "DS18B20";

static gpio_num_t DS18B20_GPIO;
static uint8_t DS18B20_resolution = 12;




void DS18B20_Create(gpio_num_t gpio){
	
	DS18B20_GPIO = gpio;
	ESP_ERROR_CHECK(gpio_set_direction(DS18B20_GPIO, GPIO_MODE_INPUT_OUTPUT_OD));
//	ESP_LOGI(TAG_DS18B20, "Pin DS18B20 DQ setting as input and output (open drain)");
}

void DS18B20_Select_Resolution(uint8_t resolution){
	uint8_t resolution_value = DS18B20_RES_12_BIT;
	
	switch (resolution) {
		case 9:
			resolution_value = DS18B20_RES_9_BIT;
			break;
		case 10:
			resolution_value = DS18B20_RES_10_BIT;
			break;
		case 11:
			resolution_value = DS18B20_RES_11_BIT;
			break;
		case 12:
			resolution_value = DS18B20_RES_12_BIT;
			break;
		default:
			ESP_LOGE("DS18B20","Resolution of %d not valid. Will be set 12 bit resolution!", (int)resolution);
			resolution_value = DS18B20_RES_12_BIT;
			break;
	}
	
	DS18B20_resolution =  resolution_value;
	DS18B20_send_Resolution(&resolution_value);
//	ESP_LOGI(TAG_DS18B20, "Resolution of DS18B20 established!");
	
}

esp_err_t  DS18B20_Initialize(){
	
	gpio_set_level(DS18B20_GPIO, 0);
	esp_rom_delay_us(800);			// Waiting time to send the reset action
	gpio_set_level(DS18B20_GPIO, 1);
	esp_rom_delay_us(90);			// Waiting time to catch the DS18B20 response
//	response = gpio_get_level(DS18B20_GPIO);
	
	if(gpio_get_level(DS18B20_GPIO)){
		ESP_LOGE(TAG_DS18B20, "No DS18B20 found!");
		return ESP_ERR_INVALID_RESPONSE;
	}
	
	esp_rom_delay_us(390);
	
	if(!gpio_get_level(DS18B20_GPIO)){
		ESP_LOGE(TAG_DS18B20, "Data line in low level much time!");
		return ESP_FAIL;
	}
	
//	ESP_LOGI(TAG_DS18B20, "Initialization successful!");
	return ESP_OK;
}

void  DS18B20_Write_Byte(uint8_t Byte_Send){
	
	for(uint8_t i = 8; i > 0; i--){
		esp_rom_delay_us(3);		// Waiting time between bits sending
		gpio_set_level(DS18B20_GPIO, 0);
		if((Byte_Send & 0x01)){
			esp_rom_delay_us(10);	// Waiting time to send 1
			gpio_set_level(DS18B20_GPIO, 1);
			esp_rom_delay_us(80); 
		}else{
			esp_rom_delay_us(90);	// Waiting time to send 0
			gpio_set_level(DS18B20_GPIO, 1);
		}
		
		Byte_Send >>= 1;
	}
	esp_rom_delay_us(3);			// Waiting time to avoid reading/writing errors
}

uint8_t  DS18B20_Read_Byte(){
	static uint8_t data = 0;
	
	for(uint8_t i = 0; i < 8; i++){
		data >>= 1;
		esp_rom_delay_us(3);			// Waiting time between bits reading 
		gpio_set_level(DS18B20_GPIO, 0);
		esp_rom_delay_us(3);			// Waiting time to release/stop driving the data line
		gpio_set_level(DS18B20_GPIO, 1);
		esp_rom_delay_us(7);			// Waiting time to check the data line
		if(gpio_get_level(DS18B20_GPIO)){
			data |= 0x80;
		}
		esp_rom_delay_us(80);
		if(!gpio_get_level(DS18B20_GPIO)){	// Verify DS18B20 release/stop driving the data line
			ESP_LOGE(TAG_DS18B20,"Data line low level 60 us after the read time slot was initiated");
		}
	}
	
	esp_rom_delay_us(3);			// Waiting time to avoid reading/writing errors
		
	return data;
}

esp_err_t  DS18B20_Skip_ROM(){
	uint8_t command = DS18B20_SKIP_ROM_COMM;
	
	DS18B20_Write_Byte(command);
	
//	ESP_LOGI(TAG_DS18B20, "ROM skipped successful");
	return ESP_OK;
}

esp_err_t  DS18B20_Memory_Function(uint8_t *command){
	
	DS18B20_Write_Byte(*command);
	
//	ESP_LOGI(TAG_DS18B20, "Memory function command send successful!");
	return ESP_OK;
}

void DS18B20_do_Basic_Sequence(uint8_t* Memory_Comand){
	
	DS18B20_Initialize();
	DS18B20_Skip_ROM();
	DS18B20_Memory_Function(Memory_Comand);
	
}


void DS18B20_do_Read_Scratch(uint8_t * Scratch){
	uint8_t command = DS18B20_READ_SCRA_COMM;
	
	DS18B20_do_Basic_Sequence(&command);
	
	for(uint8_t i = 0; i < 9; i++){
		Scratch[i] = DS18B20_Read_Byte();
	}
}

void DS18B20_do_Temperature_Convertion(){
	uint8_t command = DS18B20_CONV_T_COMM;
	
	DS18B20_do_Basic_Sequence(&command);
	
	vTaskDelay(pdMS_TO_TICKS(800));
	if(!gpio_get_level(DS18B20_GPIO)){
		ESP_LOGE(TAG_DS18B20, "Data line low level still, after 800 ms (max convertion time: 750 ms)");
	}
}
	
void DS18B20_do_Write_Scratch(uint8_t* Scratch){
	uint8_t command = DS18B20_WRIT_SCRA_COMM;
	
	DS18B20_do_Basic_Sequence(&command);
	
	for(uint8_t i = 0; i < 3; i++){
		DS18B20_Write_Byte(Scratch[i]);
	}
}

void DS18B20_send_Resolution(uint8_t * Resolution){
	uint8_t scratch[3]={
		0x00,
		0x00,
		*Resolution
	};
	
	
	
	DS18B20_do_Write_Scratch(scratch);
}

float DS18B20_get_Temperature_Value(){
	float temperature = 0.0;
	int  temperature_cod = 0;
	uint8_t scratch[9];
	
	DS18B20_do_Temperature_Convertion();
	
	DS18B20_do_Read_Scratch(scratch);
	
	temperature_cod = (scratch[1] << 8) | scratch[0];
	
	switch (DS18B20_resolution){
		case DS18B20_RES_9_BIT:
		temperature_cod = (temperature_cod >> 3) | (0xE000 & temperature_cod);
		temperature = (float)(temperature_cod);
		temperature = temperature * 0.5;			// Conversion to °C
		break;
	case DS18B20_RES_10_BIT:
		temperature_cod = (temperature_cod >> 2) | (0xC000 & temperature_cod);
		temperature = (float)(temperature_cod);
		temperature = temperature * 0.25;			// Conversion to °C
		break;
	case DS18B20_RES_11_BIT:
		temperature_cod = (temperature_cod >> 1) | (0x8000 & temperature_cod);
		temperature = (float)(temperature_cod);
		temperature = temperature * 0.125;			// Conversion to °C
		break;
	case DS18B20_RES_12_BIT:
		temperature = (float)(temperature_cod);
		temperature = temperature * 0.0625;			// Conversion to °C
		break; 
	}
	
	return temperature; 
}

void DS18B20_print_Resolution_selected(uint8_t * Resolution_selected){
	
	printf("Resolution selected: ");
	switch(*Resolution_selected){
		case DS18B20_RES_9_BIT:
			printf("9 bits! \n");				
			break;
		case DS18B20_RES_10_BIT:
			printf("10 bits! \n");
			break;
		case DS18B20_RES_11_BIT:
			printf("11 bits! \n");
			break;
		case DS18B20_RES_12_BIT:
			printf("12 bits! \n");
			break;
	}
	
}



