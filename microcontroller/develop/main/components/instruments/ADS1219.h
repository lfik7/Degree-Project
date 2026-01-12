/*
 * ADS1219.h
 *
 *  Created on: 27/12/2025
 *      Author: Lotfi Dalal
 */

#ifndef ADS1219_H_
#define ADS1219_H_


/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* i2c - Simple Example

   Simple I2C example that shows how to initialize I2C
   as well as reading and writing from and to registers for a sensor connected over I2C.

   The sensor used in this example is a MPU9250 inertial measurement unit.
*/
#include <stdio.h>
#include <inttypes.h>
#include "driver/gpio.h"
#include "esp_err.h"
#include "freertos/projdefs.h"
#include "hal/gpio_types.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"

#include "driver/gpio.h"
#include "freertos/idf_additions.h"
#include "portmacro.h"
#include "soc/gpio_num.h"

//static const char *TAG = "I2C";
//static const char *TAG_ADS1219_conf = "ADS1219 configuration";
////static const char *TAG_ADS1219 = "ADS1219";
//static const char *TAG_Process = "Pocess";

#define I2C_MASTER_SCL_IO           22					        /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           21					        /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0		                    /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ          400000						/*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       -1

#define GPIO_RDY			19					/*!< Pin to monitoring the RDY pin from ADS1219 */

#define ADS1219_ADDR         		 0x40		        /*!< Address of the ADS1219 ADC */
#define ADS1219_COMMAND_RESET   	 0x06		        /*!< Command to reset the ADC */
#define ADS1219_COMMAND_START_SYNC 	 0x08		        /*!< Command to start or sync the ADC */
#define ADS1219_COMMAND_POWERDOWN	 0x02				/*!< Command to powerdown the ADC */
#define ADS1219_COMMAND_RDATA		 0x10				/*!< Command to read the data (sample) */
#define ADS1219_COMMAND_READ_CONFREG 0x10				/*!< Command to read the configuration register */
#define ADS1219_COMMAND_READ_STATREG 0x14				/*!< Command to read the status register */
#define ADS1219_COMMAND_WRIT_CONFREG 0x40				/*!< Command to write in the configuration register */
#define ADS1219_CONFIG_SEL_CH0		 0x60				/*!< Value to configure the 3 MSB ADS1219 configure register to select channel 0 */
#define ADS1219_CONFIG_SEL_CH1		 0x80				/*!< Value to configure the 3 MSB ADS1219 configure register to select channel 1 */
#define ADS1219_CONFIG_SEL_CH2		 0xA0				/*!< Value to configure the 3 MSB ADS1219 configure register to select channel 2 */
#define ADS1219_CONFIG_SEL_CH3		 0xC0				/*!< Value to configure the 3 MSB ADS1219 configure register to select channel 3 */
#define ADS1219_CONFIG_GAIN_1		 0x00				/*!< Value to configure the 4 bit of ADS1219 configure register to select gain factor of 1 */
#define ADS1219_CONFIG_GAIN_4		 0x10				/*!< Value to configure the 4 bit of ADS1219 configure register to select gain factor of 4 */
#define ADS1219_CONFIG_SPS_20		 0x00				/*!< Value to configure the 2,3 bits of ADS1219 configure register to select 20 sps */
#define ADS1219_CONFIG_SPS_90		 0x04				/*!< Value to configure the 2,3 bits of ADS1219 configure register to select 90 sps */
#define ADS1219_CONFIG_SPS_330		 0x08				/*!< Value to configure the 2,3 bits of ADS1219 configure register to select 330 sps */
#define ADS1219_CONFIG_SPS_1000		 0x0C				/*!< Value to configure the 2,3 bits of ADS1219 configure register to select 1000 sps */
#define ADS1219_CONFIG_SINGLE_SHOT	 0x00				/*!< Value to configure the 1 bit of ADS1219 configure register to select single-shot */
#define ADS1219_CONFIG_CONTINUOUS	 0x02				/*!< Value to configure the 1 bit of ADS1219 configure register to select continuous */
#define ADS1219_CONFIG_REF_INTERN	 0x00				/*!< Value to configure the 0 bit of ADS1219 configure register to select the internal reference */
#define ADS1219_CONFIG_REF_EXTERN	 0x01				/*!< Value to configure the 0 bit of ADS1219 configure register to select the external reference */

#define ADS1219_DEFAULT_CONFIG		 0x0D				/*!< No selected channel, factor gain in 1, 1000 sps, continuous, external reference || the idea is only do or operations to change/switch the channel */



void ADS1219_create(gpio_num_t i2c_gpio_scl, gpio_num_t i2c_gpio_sda, gpio_num_t i2c_gpio_rdy);
void ADS1219_configure_bus(i2c_port_num_t port, i2c_clock_source_t clk_src);
void ADS1219_configure_device(i2c_addr_bit_len_t addr_len_ADS, uint16_t addr_ADS, uint32_t scl_speed_ADS);
void i2c_master_init();
esp_err_t ADS1219_init();
esp_err_t ADS1219_start();
esp_err_t ADS1219_configure(uint8_t *Conf_selected);
esp_err_t ADS1219_write_commnad(uint8_t *command);
esp_err_t ADS1219_read_sample(uint8_t *data_buff);
esp_err_t ADS1219_read_consiguration(uint8_t *conf_buff);
void ADS1219_determine_consiguration(uint8_t *conf_buff);
int32_t ADS1219_read_channel_raw(uint8_t channel);
float ADS1219_read_channel_voltage(uint8_t channel);



//void ADS1219_main_function(void);


#endif /* ADS1219_H_ */
