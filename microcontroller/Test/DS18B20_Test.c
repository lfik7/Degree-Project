/*
	Project to handle DS18B20 temperature sensor
	
	This project is focused in getting the temperature value. Here it doesn't
	use the alarm (not setting the TH and TL), nether write the memory, or 
	read the ROM. Mainly temperature convertion and read the scratch (for 
	obtain the temperature "value")
	
	DS18B20 works by the TRANSACTION SEQUENCE:
	
	1. Initialization (Master does reset action, and DS18B20 does presence action)
	2. ROM Function Command (Read, Match, Skip, Search, Alarm search)
	3. Memory Function Command (Write scratch, Read scratch, Copy scratch, Convert T, Recall E2, Read Power Supply)
	4. Transaction/Data
	
	¡¡Data shall send first the LSB to the MSB!!
	
	In this project, only the skip command (and may be Read command) is used in 
	the second stedp (ROM Function Command), and only convert T and read scratch
	in the third step.
	
	When have two DS18B20 connected, is necessary know the ROM before use bought
	at same time (connecting only first one and using read command in step 2, and
	next connect only the second one and repeat the previous steps). With the 
	match command (in the second step (ROM Function Command)) can "select" the 
	desired DS18B20.
	
	In the case more than one DS18B20 are connected, and can't be disconnected/
	unplugged, is possible know the ROMs using the search command in the second
	step.  

*/


#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <driver/gpio.h>
#include <hal/gpio_types.h>
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h"
#include "portmacro.h"
#include "esp_err.h"
#include "esp_log.h"


#define DS18B20_DQ 		GPIO_NUM_4

#define SKIP_ROM_COMM	0xCC
#define READ_SCRA_COMM	0xBE
#define CONV_T_COMM		0x44
#define WRIT_SCRA_COMM	0x4E

#define RES_9_BIT		0x1F		// (b'00011111)
#define RES_10_BIT		0x3F		// (b'00111111)
#define RES_11_BIT		0x5F		// (b'01011111)
#define RES_12_BIT		0x7F		// (b'01111111)
#define RES_SELECT		RES_12_BIT

const char *TAG_Process = "Process", *TAG_DS18B20 = "DS18B20";


/* Functions prototype */

esp_err_t  Initialization_DS18B20();
void  Write_Byte_DS18B20(uint8_t);
uint8_t  Read_Byte_DS18B20();
esp_err_t  ROM_Function_DS18B20();					// Only send the skip ROM command
esp_err_t  Memory_Function_DS18B20(uint8_t *);
void do_Basic_Sequence_DS18B20(uint8_t *);			// Function to resume the first three steps of the TRANSACTION SEQUENCE
void do_Read_Scratch_DS18B20(uint8_t *);			// Return uint8_t [9] || the 9 bytes of the scratch
void do_Temperature_Convertion_DS18B20();	
void do_Write_Scratch_DS18B20(uint8_t*);			// Receive one vector (uint8_t [3]) with TH,TL, and config registers
void resolution_Select_DS18B20(uint8_t*);			// Receive one variable uint8_t to select 9, 10, 11 or 12 bit resolution
float get_Temperature_Value();						// Calls do_Temperature_Convertion_DS18B20 and do_Read_Scratch_DS18B2, and convert the output to a temperature value (in °C)
void print_Resolution_selected_DS18B20(uint8_t *);	// Get the resolution value and print it


void app_main(void)
{
	ESP_LOGI(TAG_Process, "Starting the setting configurations!");
	
	/* GPIO configuration */
	ESP_ERROR_CHECK(gpio_set_direction(DS18B20_DQ, GPIO_MODE_INPUT_OUTPUT_OD));
	ESP_LOGI(TAG_Process, "Pin DS18B20 DQ setting as input and output (open drain)");
	
	uint8_t resolution = RES_SELECT;
	resolution_Select_DS18B20(&resolution);
	ESP_LOGI(TAG_Process, "Resolution of DS18B20 established!");
	print_Resolution_selected_DS18B20(&resolution);
	
//	float temperature = 0.0;
	
    while (true) {
		
		printf("Temperature: %.4f deg C\n", get_Temperature_Value());
		
		vTaskDelay(pdMS_TO_TICKS(2250));
		
    }
}



/* Functions definition */

esp_err_t  Initialization_DS18B20(){
//	int response = 1;
	
	ESP_ERROR_CHECK(gpio_set_level(DS18B20_DQ, 0));
	esp_rom_delay_us(800);			// Waiting time to send the reset action
	ESP_ERROR_CHECK(gpio_set_level(DS18B20_DQ, 1));
	esp_rom_delay_us(90);			// Waiting time to catch the DS18B20 response
//	response = gpio_get_level(DS18B20_DQ);
	
//	if(response){
	if(gpio_get_level(DS18B20_DQ)){
		ESP_LOGE(TAG_DS18B20, "No DS18B20 found!");
		return ESP_ERR_INVALID_RESPONSE;
	}
	
	esp_rom_delay_us(390);
	
	if(!gpio_get_level(DS18B20_DQ)){
		ESP_LOGE(TAG_DS18B20, "Data line in low level much time!");
		return ESP_FAIL;
	}
	
//	ESP_LOGI(TAG_DS18B20, "Initialization successful!");
	return ESP_OK;
}

void  Write_Byte_DS18B20(uint8_t Byte_Send){
	
	for(uint8_t i = 8; i > 0; i--){
		esp_rom_delay_us(3);		// Waiting time between bits sending
		ESP_ERROR_CHECK(gpio_set_level(DS18B20_DQ, 0));
		if((Byte_Send & 0x01)){
			esp_rom_delay_us(10);	// Waiting time to send 1
			ESP_ERROR_CHECK(gpio_set_level(DS18B20_DQ, 1));
			esp_rom_delay_us(80); 
		}else{
			esp_rom_delay_us(90);	// Waiting time to send 0
			ESP_ERROR_CHECK(gpio_set_level(DS18B20_DQ, 1));
		}
		
		Byte_Send >>= 1;
	}
	esp_rom_delay_us(3);			// Waiting time to avoid reading/writing errors
}

uint8_t  Read_Byte_DS18B20(){
	static uint8_t data = 0;
	
	for(uint8_t i = 0; i < 8; i++){
		data >>= 1;
		esp_rom_delay_us(3);			// Waiting time between bits reading 
		ESP_ERROR_CHECK(gpio_set_level(DS18B20_DQ, 0));
		esp_rom_delay_us(3);			// Waiting time to release/stop driving the data line
		ESP_ERROR_CHECK(gpio_set_level(DS18B20_DQ, 1));
		esp_rom_delay_us(7);			// Waiting time to check the data line
		if(gpio_get_level(DS18B20_DQ)){
			data |= 0x80;
		}
		esp_rom_delay_us(80);
		if(!gpio_get_level(DS18B20_DQ)){	// Verify DS18B20 release/stop driving the data line
			ESP_LOGE(TAG_DS18B20,"Data line low level 60 us after the read time slot was initiated");
		}
	}
	
	esp_rom_delay_us(3);			// Waiting time to avoid reading/writing errors
		
	return data;
}

esp_err_t  ROM_Function_DS18B20(){
	uint8_t command = SKIP_ROM_COMM;
	
	Write_Byte_DS18B20(command);
	
//	ESP_LOGI(TAG_DS18B20, "ROM skipped successful");
	return ESP_OK;
}

esp_err_t  Memory_Function_DS18B20(uint8_t *command){
	
	Write_Byte_DS18B20(*command);
	
//	ESP_LOGI(TAG_DS18B20, "Memory function command send successful!");
	return ESP_OK;
}

void do_Basic_Sequence_DS18B20(uint8_t* Memory_Comand){
	
	Initialization_DS18B20();
	ROM_Function_DS18B20();
	Memory_Function_DS18B20(Memory_Comand);
	
}


void do_Read_Scratch_DS18B20(uint8_t * Scratch){
	uint8_t command = READ_SCRA_COMM;
	
	do_Basic_Sequence_DS18B20(&command);
	
	for(uint8_t i = 0; i < 9; i++){
		Scratch[i] = Read_Byte_DS18B20();
	}
}

void do_Temperature_Convertion_DS18B20(){
	uint8_t command = CONV_T_COMM;
	
	do_Basic_Sequence_DS18B20(&command);
	
	vTaskDelay(pdMS_TO_TICKS(800));
	if(!gpio_get_level(DS18B20_DQ)){
		ESP_LOGE(TAG_DS18B20, "Data line low level still, after 800 ms (max convertion time: 750 ms)");
	}
}
	
void do_Write_Scratch_DS18B20(uint8_t* Scratch){
	uint8_t command = WRIT_SCRA_COMM;
	
	do_Basic_Sequence_DS18B20(&command);
	
	for(uint8_t i = 0; i < 3; i++){
		Write_Byte_DS18B20(Scratch[i]);
	}
}

void resolution_Select_DS18B20(uint8_t * Resolution){
	uint8_t scratch[3]={
		0x00,
		0x00,
		*Resolution
	};
	
	
	
	do_Write_Scratch_DS18B20(scratch);
}

float get_Temperature_Value(){
	float temperature = 0.0;
	int  temperature_cod = 0;
	uint8_t scratch[9];
	
	do_Temperature_Convertion_DS18B20();
	
	do_Read_Scratch_DS18B20(scratch);
	
	temperature_cod = (scratch[1] << 8) | scratch[0];
	
	#if RES_SELECT == RES_9_BIT
		temperature_cod = (temperature_cod >> 3) | (0xE000 & temperature_cod);
		temperature = (float)(temperature_cod);
		temperature = temperature * 0.5;			// Conversion to °C
	#elif RES_SELECT == RES_10_BIT
		temperature_cod = (temperature_cod >> 2) | (0xC000 & temperature_cod);
		temperature = (float)(temperature_cod);
		temperature = temperature * 0.25;			// Conversion to °C
	#elif RES_SELECT == RES_11_BIT
		temperature_cod = (temperature_cod >> 1) | (0x8000 & temperature_cod);
		temperature = (float)(temperature_cod);
		temperature = temperature * 0.125;			// Conversion to °C
	#elif RES_SELECT == RES_12_BIT
		temperature = (float)(temperature_cod);
		temperature = temperature * 0.0625;			// Conversion to °C 
	#endif
	
	return temperature; 
}

void print_Resolution_selected_DS18B20(uint8_t * Resolution_selected){
	
	printf("Resolution selected: ");
	switch(*Resolution_selected){
		case RES_9_BIT:
			printf("9 bits! \n");				
			break;
		case RES_10_BIT:
			printf("10 bits! \n");
			break;
		case RES_11_BIT:
			printf("11 bits! \n");
			break;
		case RES_12_BIT:
			printf("12 bits! \n");
			break;
	}
	
}

