/*
 * DS18B20.h
 *
 *  Created on: 27/12/2025
 *      Author: Lotfi Dalal
 */

#ifndef DS18B20_H_
#define DS18B20_H_

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
#include "soc/gpio_num.h"


#define DS18B20_DQ 		GPIO_NUM_4

#define DS18B20_SKIP_ROM_COMM	0xCC
#define DS18B20_READ_SCRA_COMM	0xBE
#define DS18B20_CONV_T_COMM		0x44
#define DS18B20_WRIT_SCRA_COMM	0x4E

#define DS18B20_RES_9_BIT		0x1F		// (b'00011111)
#define DS18B20_RES_10_BIT		0x3F		// (b'00111111)
#define DS18B20_RES_11_BIT		0x5F		// (b'01011111)
#define DS18B20_RES_12_BIT		0x7F		// (b'01111111)
#define DS18B20_RES_SELECT		DS18B20_RES_12_BIT


/* Functions prototype */

void DS18B20_Create(gpio_num_t gpio);
void DS18B20_Select_Resolution(uint8_t resolution);
esp_err_t  DS18B20_Initialize();
void  DS18B20_Write_Byte(uint8_t);
uint8_t  DS18B20_Read_Byte();
esp_err_t  DS18B20_Skip_ROM();					// Only send the skip ROM command
esp_err_t  DS18B20_Memory_Function(uint8_t *);
void DS18B20_do_Basic_Sequence(uint8_t *);			// Function to resume the first three steps of the TRANSACTION SEQUENCE
void DS18B20_do_Read_Scratch(uint8_t *);			// Return uint8_t [9] || the 9 bytes of the scratch
void DS18B20_do_Temperature_Convertion();	
void DS18B20_do_Write_Scratch(uint8_t*);			// Receive one vector (uint8_t [3]) with TH,TL, and config registers
void DS18B20_send_Resolution(uint8_t*);			// Receive one variable uint8_t to select 9, 10, 11 or 12 bit resolution
float DS18B20_get_Temperature_Value();						// Calls do_Temperature_Convertion_DS18B20 and do_Read_Scratch_DS18B2, and convert the output to a temperature value (in °C)
void DS18B20_print_Resolution_selected(uint8_t *);	// Get the resolution value and print it


//void DS18B20_main_function(void);




#endif /* DS18B20_H_ */
