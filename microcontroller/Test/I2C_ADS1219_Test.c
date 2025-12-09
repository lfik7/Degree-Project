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

#include "esp_attr.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "freertos/idf_additions.h"
#include "portmacro.h"

static const char *TAG = "I2C";
static const char *TAG_ADS1219_conf = "ADS1219 configuration";
//static const char *TAG_ADS1219 = "ADS1219";
static const char *TAG_Process = "Pocess";

#define I2C_MASTER_SCL_IO           22					        /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           21					        /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0		                    /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ          400000						/*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       50

#define GPIO_RDY			19					/*!< Pin to monitoring the RDY pin from ADS1219 */

#define ADS1219_ADDR         0x40		        /*!< Address of the ADS1219 ADC */
#define COMMAND_RESET   	 0x06		        /*!< Command to reset the ADC */
#define COMMAND_START_SYNC 	 0x08		        /*!< Command to start or sync the ADC */
#define COMMAND_POWERDOWN	 0x02				/*!< Command to powerdown the ADC */
#define COMMAND_RDATA		 0x10				/*!< Command to read the data (sample) */
#define COMMAND_READ_CONFREG 0x10				/*!< Command to read the configuration register */
#define COMMAND_READ_STATREG 0x14				/*!< Command to read the status register */
#define COMMAND_WRIT_CONFREG 0x40				/*!< Command to write in the configuration register */
#define CONFIG_SEL_CH0		 0x60				/*!< Value to configure the 3 MSB ADS1219 configure register to select channel 0 */
#define CONFIG_SEL_CH1		 0x80				/*!< Value to configure the 3 MSB ADS1219 configure register to select channel 1 */
#define CONFIG_SEL_CH2		 0xA0				/*!< Value to configure the 3 MSB ADS1219 configure register to select channel 2 */
#define CONFIG_SEL_CH3		 0xC0				/*!< Value to configure the 3 MSB ADS1219 configure register to select channel 3 */
#define CONFIG_GAIN_1		 0x00				/*!< Value to configure the 4 bit of ADS1219 configure register to select gain factor of 1 */
#define CONFIG_GAIN_4		 0x10				/*!< Value to configure the 4 bit of ADS1219 configure register to select gain factor of 4 */
#define CONFIG_SPS_20		 0x00				/*!< Value to configure the 2,3 bits of ADS1219 configure register to select 20 sps */
#define CONFIG_SPS_90		 0x04				/*!< Value to configure the 2,3 bits of ADS1219 configure register to select 90 sps */
#define CONFIG_SPS_330		 0x08				/*!< Value to configure the 2,3 bits of ADS1219 configure register to select 330 sps */
#define CONFIG_SPS_1000		 0x0D				/*!< Value to configure the 2,3 bits of ADS1219 configure register to select 1000 sps */
#define CONFIG_SINGLE_SHOT	 0x00				/*!< Value to configure the 1 bit of ADS1219 configure register to select single-shot */
#define CONFIG_CONTINUOUS	 0x02				/*!< Value to configure the 1 bit of ADS1219 configure register to select continuous */
#define CONFIG_REF_INTERN	 0x00				/*!< Value to configure the 0 bit of ADS1219 configure register to select the internal reference */
#define CONFIG_REF_EXTERN	 0x01				/*!< Value to configure the 0 bit of ADS1219 configure register to select the external reference */

#define DEFAULT_CONFIG		 0x09				/*!< No selected channel, factor gain in 1, 330 sps, continuous, external reference || the idea is only do or operations to change/switch the channel */

#define Timer_time			1000000

bool get_sample = false, Finish_timer = false;


/**
 * @brief Configure de ADS1219 (write in the configure register)
 */
static esp_err_t ADS1219_configuration(i2c_master_dev_handle_t dev_handle, uint8_t *Conf_selected);

/**
 * @brief Write a command to the ADS1219 (Reset, Start/Sync, Powerdown)
 */
static esp_err_t ADS1219_write_commnad(i2c_master_dev_handle_t dev_handle, uint8_t *command);

/**
 * @brief Read a taken sample by the ADS1219
 */
static esp_err_t ADS1219_read_sample(i2c_master_dev_handle_t dev_handle, uint8_t *data_buff);

/**
 * @brief Read the ADS1219 configuration
 */
static esp_err_t ADS1219_read_consiguration(i2c_master_dev_handle_t dev_handle, uint8_t *conf_buff);

/**
 * @brief Determine the current ADS1219 configuration
 */
void ADS1219_determine_consiguration(uint8_t *conf_buff);

/**
 * @brief i2c master initialization
 */
static void i2c_master_init(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle);

/**
 * @brief pin interrupt function
 */
//void IRAM_ATTR Redy_Sample(void * args);

/**
 * @brief timer interrupt function
 */
void IRAM_ATTR Callback_Timer1(void * args);
	
// Define the Timer
esp_timer_handle_t Timer1;
// Create the structure for the Timer settings parameters
const esp_timer_create_args_t ConfigTimer = {
	.callback = Callback_Timer1,
	.arg = NULL,
	.name = "Timer1"
};

void app_main(void)
{
	ESP_LOGI(TAG_Process, "Running code!!!");
	/* Creating variables, structures, etc */
    uint8_t sample_raw[3], channels_options[4] = {CONFIG_SEL_CH0, CONFIG_SEL_CH1, CONFIG_SEL_CH2, CONFIG_SEL_CH3};
    uint32_t sample_frames_joined = 0;
    int sample_converted = 0;
    uint8_t conf_register = 0xff;
    uint8_t commands;
    uint16_t counter = 0;
    i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t dev_handle;
    
    /*GPIO init*/
    gpio_set_direction(GPIO_RDY, GPIO_MODE_INPUT);
//	gpio_set_intr_type(GPIO_RDY, GPIO_INTR_NEGEDGE);
//	gpio_install_isr_service(0 );
//	gpio_isr_handler_add(GPIO_RDY, Redy_Sample, NULL);
//	gpio_intr_enable(GPIO_RDY);
	ESP_LOGI(TAG_Process, "GPIO intialized!");
	
	/*Timer create and init*/
	esp_timer_create(&ConfigTimer, &Timer1);
	ESP_LOGI(TAG_Process, "Timer crated!");
    
    /*I2C init and process...*/
    i2c_master_init(&bus_handle, &dev_handle);
    ESP_LOGI(TAG, "I2C initialized successfully");
    vTaskDelay(10/portTICK_PERIOD_MS);

    /* Read the ADS1219 configuration. Should be 0x0 */
    ESP_ERROR_CHECK(ADS1219_read_consiguration(dev_handle, &conf_register));
    ESP_LOGI(TAG, "Configuration got: %d", conf_register);
    ADS1219_determine_consiguration(&conf_register);
    vTaskDelay(10/portTICK_PERIOD_MS);

    /* Customize the ADS1219 configuration */
    //conf_register = CONFIG_SEL_CH2 | CONFIG_GAIN_1 | CONFIG_SPS_20 | CONFIG_CONTINUOUS | CONFIG_REF_EXTERN;
    //conf_register = 0b11000011;
    conf_register = DEFAULT_CONFIG | CONFIG_SEL_CH0;
    ESP_LOGI(TAG, "Configuration desired: %d", conf_register);
    ADS1219_determine_consiguration(&conf_register);
    ESP_ERROR_CHECK(ADS1219_configuration(dev_handle, &conf_register));
    
    vTaskDelay(5000/portTICK_PERIOD_MS);
	
	/* Start the ADS1219 sampling and the timer */
	commands = COMMAND_START_SYNC;
	ESP_ERROR_CHECK(ADS1219_write_commnad(dev_handle, &commands));
	esp_timer_start_periodic(Timer1, Timer_time);
	ESP_LOGI(TAG, "ADS1219 and Timer1 started!");
	
	vTaskDelay(100/portTICK_PERIOD_MS);
	
    ESP_LOGI(TAG, "Checking the ADS1219 configuration...");
    ESP_ERROR_CHECK(ADS1219_read_consiguration(dev_handle, &conf_register));
    ESP_LOGI(TAG, "Configuration got: %d", conf_register);
    ADS1219_determine_consiguration(&conf_register);
	
	while(1){
		
		if(get_sample){
//			get_sample = false;
			if(gpio_get_level(GPIO_RDY) == 0){
				ESP_ERROR_CHECK(ADS1219_read_sample(dev_handle, &sample_raw[0]));
				sample_frames_joined = (0x0000 | sample_raw[0] << 16) | (sample_raw[1] << 8) | sample_raw[2];
				if(sample_frames_joined > 8388607){
					sample_frames_joined = sample_frames_joined | 0xFF000000;
				}
				sample_converted = (int)sample_frames_joined;
	//			ESP_LOGI(TAG_Process, "Channel %d \t :: \t Sample: %d", (int)(counter), sample_converted);
				switch (counter) {
					case 0:
						printf("\nSEN0193: \t");
						break;
					case 1:
						printf("MQ-2: \t\t");
						break;
					case 2:
						printf("MQ-3: \t\t");
						break;
					case 3:
						printf("MQ-135: \t");
						break;
				}
				printf("%d \n", sample_converted);
	
				if(counter < 3){
					counter ++;
					conf_register = DEFAULT_CONFIG | channels_options[counter];
					ESP_ERROR_CHECK(ADS1219_configuration(dev_handle, &conf_register));
					commands = COMMAND_START_SYNC;
					ESP_ERROR_CHECK(ADS1219_write_commnad(dev_handle, &commands));
				}else if(counter == 3){
					get_sample = false;
				}
			}
			vTaskDelay(pdMS_TO_TICKS(1));
		}
		
		if(Finish_timer){
			Finish_timer = false;
			get_sample = true;			
			counter = 0;
			
//			ESP_LOGI(TAG_Process, "Getting sample from ADS1219...");
			conf_register = DEFAULT_CONFIG | channels_options[counter];
			ESP_ERROR_CHECK(ADS1219_configuration(dev_handle, &conf_register));
			commands = COMMAND_START_SYNC;
			ESP_ERROR_CHECK(ADS1219_write_commnad(dev_handle, &commands));
		}
	}
}


static esp_err_t ADS1219_configuration(i2c_master_dev_handle_t dev_handle, uint8_t *Conf_selected)
{
	
	uint8_t data[] ={
		COMMAND_WRIT_CONFREG,
		*Conf_selected,
	};
	
	return i2c_master_transmit(dev_handle, &data[0], 2, I2C_MASTER_TIMEOUT_MS);
}

static esp_err_t ADS1219_write_commnad(i2c_master_dev_handle_t dev_handle, uint8_t *command)
{
    /*uint8_t write_buf[2] = {reg_addr, data};*/
    return i2c_master_transmit(dev_handle, command, 1, I2C_MASTER_TIMEOUT_MS);
}

static esp_err_t ADS1219_read_sample(i2c_master_dev_handle_t dev_handle, uint8_t *data_buff)
{
    /*uint8_t write_buf[2] = {reg_addr, data};*/
    uint8_t Read_command = COMMAND_RDATA;
    return i2c_master_transmit_receive(dev_handle, &Read_command, 1, data_buff, 3, I2C_MASTER_TIMEOUT_MS);
}

static esp_err_t ADS1219_read_consiguration(i2c_master_dev_handle_t dev_handle, uint8_t *conf_buff)
{
    /*uint8_t write_buf[2] = {reg_addr, data};*/
    uint8_t Read_command = COMMAND_READ_CONFREG;
    return i2c_master_transmit_receive(dev_handle, &Read_command, 1, conf_buff, 1, I2C_MASTER_TIMEOUT_MS);
}

void ADS1219_determine_consiguration(uint8_t *conf_buff)
{
    uint8_t multiplexer = (*conf_buff & 0xE0) >> 5;
    uint8_t Gain = (*conf_buff & 0x10) >> 4;
    uint8_t data_rate = (*conf_buff & 0x0D) >> 2;
    uint8_t Conversion = (*conf_buff & 0x02) >> 1;
    uint8_t reference_volt = *conf_buff & 0x01;    
    
    switch(multiplexer){
		case 3:
			ESP_LOGI(TAG_ADS1219_conf, "Single ended -> input: channel 0");
			break;
		case 4:
			ESP_LOGI(TAG_ADS1219_conf, "Single ended -> input: channel 1");
			break;
		case 5:
			ESP_LOGI(TAG_ADS1219_conf, "Single ended -> input: channel 2");
			break;
		case 6:
			ESP_LOGI(TAG_ADS1219_conf, "Single ended -> input: channel 3");
			break;
		default:
			ESP_LOGI(TAG_ADS1219_conf, "Unknown multiplexer state!");
			break;
	}
	
	switch (Gain) {
		case 0:
			ESP_LOGI(TAG_ADS1219_conf, "Factor gain in 1");
			break;
		case 1:
			ESP_LOGI(TAG_ADS1219_conf, "Factor gain in 4");
			break;
		default:
			ESP_LOGI(TAG_ADS1219_conf, "Unknown factor gain!");
			break;
	}
	
	switch (data_rate) {
		case 0:
			ESP_LOGI(TAG_ADS1219_conf, "Data rate in 20 sps");
			break;
		case 1:
			ESP_LOGI(TAG_ADS1219_conf, "Data rate in 90 sps");
			break;
		case 2:
			ESP_LOGI(TAG_ADS1219_conf, "Data rate in 330 sps");
			break;
		case 3:
			ESP_LOGI(TAG_ADS1219_conf, "Data rate in 1000 sps");
			break;
		default:
			ESP_LOGI(TAG_ADS1219_conf, "Unknown data rate!");
			break;
	}
	
	switch (Conversion) {
		case 0:
			ESP_LOGI(TAG_ADS1219_conf, "Single-shot conversion mode");
			break;
		case 1:
			ESP_LOGI(TAG_ADS1219_conf, "Continuos conversion mode");
			break;
		default:
			ESP_LOGI(TAG_ADS1219_conf, "Unknown conversion mode!");
			break;
	}
	
	switch (reference_volt) {
		case 0:
			ESP_LOGI(TAG_ADS1219_conf, "Internal voltage reference");
			break;
		case 1:
			ESP_LOGI(TAG_ADS1219_conf, "External voltage reference");
			break;
		default:
			ESP_LOGI(TAG_ADS1219_conf, "Unknown voltage reference!");
			break;
	}
}

static void i2c_master_init(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, bus_handle));

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = ADS1219_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &dev_config, dev_handle));
}

//void Redy_Sample(void * args){
////	get_sample = true;
//}

void Callback_Timer1(void * args){
	Finish_timer = true;
}







