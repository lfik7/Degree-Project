/*
 * ADS1219.c
 *
 *  Created on: 27/12/2025
 *      Author: Lotfi Dalal
 */

#include "ADS1219.h"
#include "driver/i2c_types.h"
#include "esp_err.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "hal/i2c_types.h"
#include "soc/gpio_num.h"


static const char *TAG = "I2C";
static const char *TAG_ADS1219 = "ADS1219";


static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t dev_handle;
static gpio_num_t ADS_RDY, ADS_SCL, ADS_SDA;
static i2c_port_num_t i2c_port_num = I2C_MASTER_NUM;
static i2c_addr_bit_len_t ADS_ADDR_LEN = I2C_ADDR_BIT_LEN_7;
static uint16_t ADS_ADDR = ADS1219_ADDR;
static i2c_clock_source_t i2c_clk_src = I2C_CLK_SRC_DEFAULT;
static uint32_t scl_speed = I2C_MASTER_FREQ_HZ;


void ADS1219_create(gpio_num_t i2c_gpio_scl, gpio_num_t i2c_gpio_sda, gpio_num_t i2c_gpio_rdy)
{
	ADS_RDY = i2c_gpio_rdy;
	ADS_SCL = i2c_gpio_scl;
	ADS_SDA = i2c_gpio_sda;
	
	i2c_port_num = I2C_MASTER_NUM;
	ADS_ADDR_LEN = I2C_ADDR_BIT_LEN_7;
	ADS_ADDR = ADS1219_ADDR;
	i2c_clk_src = I2C_CLK_SRC_DEFAULT;
	scl_speed = I2C_MASTER_FREQ_HZ;
}

void ADS1219_configure_bus(i2c_port_num_t port, i2c_clock_source_t clk_src)
{
	i2c_port_num = port;
    i2c_clk_src = clk_src;
}

void ADS1219_configure_device(i2c_addr_bit_len_t addr_len_ADS, uint16_t addr_ADS, uint32_t scl_speed_ADS)
{
	ADS_ADDR_LEN = addr_len_ADS;
    ADS_ADDR = addr_ADS;
    scl_speed = scl_speed_ADS;
	
}

void i2c_master_init()
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = i2c_port_num,
        .sda_io_num = ADS_SDA,
        .scl_io_num = ADS_SCL,
        .clk_source = i2c_clk_src,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true, 
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    i2c_device_config_t dev_config = {
        .dev_addr_length = ADS_ADDR_LEN,
        .device_address = ADS_ADDR,
        .scl_speed_hz = scl_speed,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle));
}

esp_err_t ADS1219_init()
{
	
	ESP_ERROR_CHECK(gpio_set_direction(ADS_RDY, GPIO_MODE_INPUT));
	
    i2c_master_init();
    ESP_LOGI(TAG, "I2C initialized successfully");
    vTaskDelay(10/portTICK_PERIOD_MS);
    
    return ESP_OK;
}

esp_err_t ADS1219_start()
{
	uint8_t conf_register, commands;
    conf_register = ADS1219_DEFAULT_CONFIG | ADS1219_CONFIG_SEL_CH0;
    ESP_LOGI(TAG, "Configuration desired: %d", conf_register);
    ADS1219_determine_consiguration(&conf_register);
    ESP_ERROR_CHECK_WITHOUT_ABORT(ADS1219_configure(&conf_register));
    
    vTaskDelay(500/portTICK_PERIOD_MS);
	
	/* Start the ADS1219 sampling and the timer */
	commands = ADS1219_COMMAND_START_SYNC;
	ESP_ERROR_CHECK_WITHOUT_ABORT(ADS1219_write_commnad(&commands));
	ESP_LOGI(TAG, "ADS1219 started!");
	return ESP_OK;
}

esp_err_t ADS1219_configure(uint8_t *Conf_selected)
{
	if (dev_handle == NULL){
		ESP_LOGE(TAG, "dev_handle is NULL!");
	}
	uint8_t data[] ={
		ADS1219_COMMAND_WRIT_CONFREG,
		*Conf_selected,
	};
	
	return i2c_master_transmit(dev_handle, data, 2, I2C_MASTER_TIMEOUT_MS);
}

esp_err_t ADS1219_write_commnad(uint8_t *command)
{
    return i2c_master_transmit(dev_handle, command, 1, I2C_MASTER_TIMEOUT_MS);
}

esp_err_t ADS1219_read_sample(uint8_t *data_buff)
{
    uint8_t Read_command = ADS1219_COMMAND_RDATA;
    return i2c_master_transmit_receive(dev_handle, &Read_command, 1, data_buff, 3, I2C_MASTER_TIMEOUT_MS);
}

esp_err_t ADS1219_read_consiguration(uint8_t *conf_buff)
{
    uint8_t Read_command = ADS1219_COMMAND_READ_CONFREG;
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
			ESP_LOGI(TAG_ADS1219, "Single ended -> input: channel 0");
			break;
		case 4:
			ESP_LOGI(TAG_ADS1219, "Single ended -> input: channel 1");
			break;
		case 5:
			ESP_LOGI(TAG_ADS1219, "Single ended -> input: channel 2");
			break;
		case 6:
			ESP_LOGI(TAG_ADS1219, "Single ended -> input: channel 3");
			break;
		default:
			ESP_LOGI(TAG_ADS1219, "Unknown multiplexer state!");
			break;
	}
	
	switch (Gain) {
		case 0:
			ESP_LOGI(TAG_ADS1219, "Factor gain in 1");
			break;
		case 1:
			ESP_LOGI(TAG_ADS1219, "Factor gain in 4");
			break;
		default:
			ESP_LOGI(TAG_ADS1219, "Unknown factor gain!");
			break;
	}
	
	switch (data_rate) {
		case 0:
			ESP_LOGI(TAG_ADS1219, "Data rate in 20 sps");
			break;
		case 1:
			ESP_LOGI(TAG_ADS1219, "Data rate in 90 sps");
			break;
		case 2:
			ESP_LOGI(TAG_ADS1219, "Data rate in 330 sps");
			break;
		case 3:
			ESP_LOGI(TAG_ADS1219, "Data rate in 1000 sps");
			break;
		default:
			ESP_LOGI(TAG_ADS1219, "Unknown data rate!");
			break;
	}
	
	switch (Conversion) {
		case 0:
			ESP_LOGI(TAG_ADS1219, "Single-shot conversion mode");
			break;
		case 1:
			ESP_LOGI(TAG_ADS1219, "Continuos conversion mode");
			break;
		default:
			ESP_LOGI(TAG_ADS1219, "Unknown conversion mode!");
			break;
	}
	
	switch (reference_volt) {
		case 0:
			ESP_LOGI(TAG_ADS1219, "Internal voltage reference");
			break;
		case 1:
			ESP_LOGI(TAG_ADS1219, "External voltage reference");
			break;
		default:
			ESP_LOGI(TAG_ADS1219, "Unknown voltage reference!");
			break;
	}
}



int32_t ADS1219_read_channel_raw(uint8_t channel)
{
//	ESP_LOGI("ADS1219", "Reading channel %d...", channel);
    uint8_t conf_register = 0xff, commands, channel_val = 0, sample_raw[3];
    int32_t sample_frames_joined = 0;
	//			ESP_LOGI(TAG_Process, "Getting sample from ADS1219...");
	
	switch (channel) {
		case 0:
			channel_val = ADS1219_CONFIG_SEL_CH0;
			break;
		case 1:
			channel_val = ADS1219_CONFIG_SEL_CH1;
			break;
		case 2:
			channel_val = ADS1219_CONFIG_SEL_CH2;
			break;
		case 3:
			channel_val = ADS1219_CONFIG_SEL_CH3;
			break;
		default:
			ESP_LOGE("ADS1219","Channel (%d) invalid. Will be set channel 0", (int)channel);
			channel_val = ADS1219_CONFIG_SEL_CH0;
	}
		
//	while(gpio_get_level(ADS_RDY) == 1) vTaskDelay(pdMS_TO_TICKS(1));
	conf_register = ADS1219_DEFAULT_CONFIG | channel_val;
//	ESP_LOGI("ADS1219", "Configuring ADS1219 in channel %d...", channel);
	ESP_ERROR_CHECK_WITHOUT_ABORT(ADS1219_configure(&conf_register));
	commands = ADS1219_COMMAND_START_SYNC;
//	ESP_LOGI("ADS1219", "Starting conversion in channel %d...", channel);
//	vTaskDelay(pdMS_TO_TICKS(2));
	ESP_ERROR_CHECK_WITHOUT_ABORT(ADS1219_write_commnad(&commands));
	
//	vTaskDelay(pdMS_TO_TICKS(1));
	
	while(gpio_get_level(ADS_RDY) == 1) vTaskDelay(pdMS_TO_TICKS(1));
	
//	ESP_LOGI("ADS1219", "Reading conversion for channel %d...", channel);
	ESP_ERROR_CHECK_WITHOUT_ABORT(ADS1219_read_sample(&sample_raw[0]));
	sample_frames_joined =  (0x0000 | sample_raw[0] << 16) | (sample_raw[1] << 8) | sample_raw[2];
	if(sample_frames_joined > 8388607){
		sample_frames_joined = sample_frames_joined | 0xFF000000;
	}
	
//	ESP_LOGI("ADS1219", "Channel %d read Ok!", channel);
	return sample_frames_joined;
}


float ADS1219_read_channel_voltage(uint8_t channel)
{
	int32_t sample_raw = 0;
	float sample_voltage = 0.0;
	
	sample_raw = ADS1219_read_channel_raw(channel);
	
	sample_voltage = sample_raw * 3.3 / 8388607;
	
	return sample_voltage;
}








