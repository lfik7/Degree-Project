/*
 * Aquisition_Manager.c
 *
 *  Created on: 26/12/2025
 *      Author: Lotfi Dalal
 */

#include "Acquisition_Manager.h"
#include "LJ12A3_4_Z.h"
#include "esp_bit_defs.h"
#include "esp_err.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "portmacro.h"
#include "esp_timer.h"
#include <stdatomic.h>


#define TAKE_VARIABLES_SAMPLE_BIT					BIT1
#define CONTINUOUSLY_ACQUIRING_PRESSURE_BIT			BIT2
#define STOP_CONTINUOUSLY_ACQUIRING_PRESSURE_BIT	BIT3



static const char * Tag_Acquisition = "Acquisition";

bool LJ12A3_change_state_flag = false;
static long long sample_interval = 0;
static func_callback_t variables_callback = NULL;
static func_callback_t door_callback = NULL;
static func_callback_t upper_pressure_threshold_callback = NULL; 
static func_callback_t lower_pressure_threshold_callback = NULL;
static VariablesData_t* variables_data= NULL;
static doorState_t* door_data = NULL;
static pressureThresholds_t pressure_threshold;

static atomic_int_fast32_t stack_raw_pressure = 0;
static atomic_int_fast16_t acquisition_task_flags = 0; 
static atomic_bool continuously_acquiring_pressure = false;
static atomic_bool HX711_busy = false;

static TaskHandle_t xMainTaskLoopHandel = NULL;

static void Acquisition_main_task();			// state machine. Responsible for notify (callbacks)

static void Acquisition_main_variables_task(void*);	// Manages the time to sample and fill the variables structure
//static void Acquisition_gases_variables_loop(void*);	// Manages only the gases variables (determine the gases and their "proportion")


static void LJ12A3_change_state();				// Use only flags in this function
static float Acquisition_get_pressure();
static void Acquisition_continuously_acquiring_pressure();
static void Acquisition_continuously_converting_pressure();

 /* Timer configuartion */
static void IRAM_ATTR Callback_timer_sample(void* args);
	
// Define the Timer
static esp_timer_handle_t timer_sample;
// Create the structure for the Timer settings parameters
static const esp_timer_create_args_t ConfigTimer = {
	.callback = Callback_timer_sample,
	.arg = NULL,
	.name = "timer_sample"
};



esp_err_t Acquisition_init(VariablesData_t* _variables_data, doorState_t* _door_data){
	ESP_LOGI(Tag_Acquisition,"Initializing component...");
	ADS1219_create(ADS1219_SCL, ADS1219_SDA, ADS1219_RDY);
	ESP_ERROR_CHECK(ADS1219_init());
	ESP_ERROR_CHECK_WITHOUT_ABORT(ADS1219_start());
	DS18B20_Create(DS18B20_DQ);
	DS18B20_Select_Resolution(12);
	HX711_Crate(HX711_SCK, HX711_DOUT);
	LJ12A3_create(LJ12A3_OUT, true);
	LJ12A3_set_Callback(LJ12A3_change_state);

	
	esp_timer_create(&ConfigTimer, &timer_sample);
	
	variables_data = _variables_data;
	door_data = _door_data;
	
	return ESP_OK;
}

void Acquisition_set_callbacks(func_callback_t _variables_callback, func_callback_t _door_callback){
		
	variables_callback = _variables_callback;
	door_callback = _door_callback;
}

void Acquisition_set_pressure_thresholds_callbacks(func_callback_t _upper_pressure_threshold_callback, func_callback_t _lower_pressure_threshold_callback){
	upper_pressure_threshold_callback = _upper_pressure_threshold_callback;
	lower_pressure_threshold_callback = _lower_pressure_threshold_callback;
}


void Acquisition_start(long long sample_interval_in_seconds){
	
//	TickType_t start_time = xTaskGetTickCount();
	sample_interval = sample_interval_in_seconds;
	
	xTaskCreate(Acquisition_main_task, "Main loop", 3072, NULL, 5, &xMainTaskLoopHandel);
	
	esp_timer_start_periodic(timer_sample, sample_interval*1000000);
	
	xTaskNotify(xMainTaskLoopHandel, 1, eIncrement);
	
	ESP_LOGI(Tag_Acquisition, "Acquisition started!");
	
}


void Acquisition_set_sample_interval(long long sample_interval_in_seconds){
	sample_interval = sample_interval_in_seconds;
	esp_timer_restart(timer_sample, sample_interval * 1000000ULL);
}

void Acquisition_set_pressure_thresholds(pressureThresholds_t thresholds){
	pressure_threshold = thresholds;
}

void Callback_timer_sample(void* args){
//	get_sample = true;
	atomic_fetch_or(&acquisition_task_flags, TAKE_VARIABLES_SAMPLE_BIT);
	vTaskNotifyGiveFromISR(xMainTaskLoopHandel, NULL);
}

void Acquisition_main_task(){
	uint16_t bits_ask = 0;
	bool countinuously_acquiring_pressure_already = false;
	
	while(1){
		uint32_t count = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		
		if(count > 0){
			bits_ask = atomic_exchange(&acquisition_task_flags, 0);
			
			if (bits_ask & TAKE_VARIABLES_SAMPLE_BIT) {
				xTaskCreate(Acquisition_main_variables_task, "Variables loop", 3072, NULL, 8, NULL);
			}
			
			if (bits_ask & CONTINUOUSLY_ACQUIRING_PRESSURE_BIT) {
				if (!countinuously_acquiring_pressure_already) {
					countinuously_acquiring_pressure_already = true;
					atomic_store(&continuously_acquiring_pressure, true);
					xTaskCreate(Acquisition_continuously_acquiring_pressure, "Continuously acquiring pressure", 1024, NULL, 10, NULL);
					xTaskCreate(Acquisition_continuously_converting_pressure, "Continuously converting pressure", 1024, NULL, 10, NULL);
				}
				else {
					ESP_LOGE(Tag_Acquisition, "The continuously acquiring pressure is already in process!");
				}
			}
			
			if (bits_ask & STOP_CONTINUOUSLY_ACQUIRING_PRESSURE_BIT){
				if (countinuously_acquiring_pressure_already) {
					atomic_store(&continuously_acquiring_pressure, false);
					countinuously_acquiring_pressure_already = false;
				}
				else {
					ESP_LOGE(Tag_Acquisition, "The continuously acquiring pressure is already finished!");
				}
			}

		}
	}
}


static void Acquisition_main_variables_task(void* pvParameters){
		
	int32_t Humid_CH_ADS1219 = 0;
	static int32_t MQ135_ADS1219 = 0, MQ3_ADS1219 = 0, MQ2_ADS1219 = 0; 
	float Humidity = 0.0, weight = 0.0, pressure = 0.0, gas_CO2 = 0.0, gas_OH = 0.0, gas_Nx = 0.0;
	time_t timestamp;
	
	MQ135_ADS1219 = ADS1219_read_channel_raw(0);
	MQ3_ADS1219 = ADS1219_read_channel_raw(1);
	MQ2_ADS1219 = ADS1219_read_channel_raw(2);
	Humid_CH_ADS1219 = ADS1219_read_channel_raw(3);
	
	// Here goes the conversion 
	gas_CO2 = 0.0;
	gas_OH = 0.0;
	gas_Nx = 0.0;
	
	// Do the humidity conversion 
	Humidity = (float)Humid_CH_ADS1219 * 0.0;
	
	while (atomic_load(&HX711_busy)) vTaskDelay(pdMS_TO_TICKS(1));
	atomic_store(&HX711_busy, true);
	weight = Acquisition_food_weight();
//	uint64_t time_init = esp_timer_get_time();
	pressure = Acquisition_get_pressure();
//	uint64_t time_end = esp_timer_get_time();
	atomic_store(&HX711_busy, false);
	
	variables_data->temp = DS18B20_get_Temperature_Value();
	variables_data->gas_co2 = gas_CO2;
	variables_data->gas_nit = gas_Nx;
	variables_data->gas_oh = gas_OH;
	variables_data->humidity = Humidity;
	variables_data->weight = weight;
	variables_data->pressure = pressure;
	
//	ESP_LOGI(Tag_Acquisition, "Pressure sensing time: %llu", (time_end - time_init));
	
	time(&timestamp);
	variables_data->timestamp = timestamp;
	if (variables_callback != NULL){
		variables_callback();
	}
	
	if (pressure > pressure_threshold.max) {
		if (upper_pressure_threshold_callback != NULL) upper_pressure_threshold_callback();
	}
	
	if (pressure < pressure_threshold.min) {
		if (lower_pressure_threshold_callback != NULL) lower_pressure_threshold_callback();
	}
	
	vTaskDelete(NULL);
}



void LJ12A3_change_state(){
	// Use only flags in this function
	time_t timestamp_door;
	LJ12A3_change_state_flag = true;
	door_data->state = !LJ12A3_detects_Object();
	time(&timestamp_door);
	door_data->timestamp = timestamp_door;
	if(door_callback != NULL){
		door_callback();
	}
}


void Acquisition_test_sensors(SensorData_test_t* Sens_data){
	ESP_LOGI(Tag_Acquisition,"Testing sensors...");
	Sens_data->valores[0] = ADS1219_read_channel_raw(0);
	Sens_data->valores[1] = ADS1219_read_channel_raw(1);
	Sens_data->valores[2] = ADS1219_read_channel_raw(2);
	Sens_data->valores[3] = ADS1219_read_channel_raw(3);
	Sens_data->valores[4] = HX711_read_channel_raw('A',64);
	Sens_data->valores[5] = HX711_read_channel_raw('B', 32);
	Sens_data->temp = DS18B20_get_Temperature_Value();
	Sens_data->det_obj = LJ12A3_change_state_flag;
	LJ12A3_change_state_flag = false;
	
	printf("\n");
	printf("---------------------------------------------\n");
	printf("\tSensors data:\n");
	printf("\t\t%-20s%lu\n", "ADS1219 CH0:",(unsigned long)Sens_data->valores[0]);
	printf("\t\t%-20s%lu\n", "ADS1219 CH1:",(unsigned long)Sens_data->valores[1]);
	printf("\t\t%-20s%lu\n", "ADS1219 CH2:",(unsigned long)Sens_data->valores[2]);
	printf("\t\t%-20s%lu\n", "ADS1219 CH3:",(unsigned long)Sens_data->valores[3]);
	printf("\t\t%-20s%lu\n", "HX711 CHA:",(unsigned long)Sens_data->valores[4]);
	printf("\t\t%-20s%lu\n", "HX711 CHB:",(unsigned long)Sens_data->valores[5]);
	printf("\t\t%-20s%.4f\n", "DS18B20 Temp:",Sens_data->temp);
	printf("\t\t%-20s%-10s\n", "LJ12A3 estado:",(Sens_data->det_obj?"cambio":"no cambio"));
	printf("---------------------------------------------\n");
	printf("\n");
}


bool Acquisition_door_state(){
	return !LJ12A3_detects_Object();
}

float Acquisition_food_weight(){
	float weight = 0;
	int32_t stack_raw_response = 0;
	uint8_t iter = 0;
	
	for(iter = 0; iter < 16; iter++){
		stack_raw_response += HX711_read_channel_raw('A',128);
	}
	
	stack_raw_response >>= 4;
	
	// Here goes the conversion but, at this moment only will set in 0.0 the result
	
	weight = (float)stack_raw_response * 0.0;
	
	weight *= 10;
	
	weight = (float)(((int)(weight))/10);	// To convert single-decimal precision  
	
	return weight;
}


float Acquisition_get_pressure(){
	float pressure = 0;
	int32_t stack_raw_response = 0;
	uint8_t iter = 0;
	
	for(iter = 0; iter < 16; iter++){
		stack_raw_response += HX711_read_channel_raw('B',32);
	}
	
	stack_raw_response >>= 4;
	
	// Here goes the conversion but, at this moment only will set in 0.0 the result
	
	pressure = (float)stack_raw_response * 0.0;
	pressure = 78.5;
	
	return pressure;
	
}


void Acquisition_check_pressure() {
	
	float pressure = 0.0;
	
	pressure = Acquisition_get_pressure();
	
	if (pressure > pressure_threshold.max) {
		if (upper_pressure_threshold_callback != NULL) upper_pressure_threshold_callback();
	}
	
	if (pressure < pressure_threshold.min) {
		if (lower_pressure_threshold_callback != NULL) lower_pressure_threshold_callback();
	}
}



void Acquisition_continuously_acquiring_pressure_set_state(bool state) {
	if (state) {
		atomic_fetch_or(&acquisition_task_flags, CONTINUOUSLY_ACQUIRING_PRESSURE_BIT);
		xTaskNotify(xMainTaskLoopHandel, 1, eIncrement);
	}
	else {
		atomic_fetch_or(&acquisition_task_flags, STOP_CONTINUOUSLY_ACQUIRING_PRESSURE_BIT);
		xTaskNotify(xMainTaskLoopHandel, 1, eIncrement);
	}
	
}




void Acquisition_continuously_acquiring_pressure() {
	atomic_store(&stack_raw_pressure, 0);
	int32_t stack_raw_response = 0;
	uint8_t iter = 0;
	
	while (atomic_load(&continuously_acquiring_pressure)) {
		while (atomic_load(&HX711_busy)) vTaskDelay(pdMS_TO_TICKS(10));
		atomic_store(&HX711_busy, true);
		stack_raw_response += HX711_read_channel_raw('B',32);
		atomic_store(&HX711_busy, false);
		iter ++;
		if (iter == 8) {
			iter = 0;
			stack_raw_response >>= 3;
			atomic_store(&stack_raw_pressure, stack_raw_response);
//			xTaskNotify(xConvertingPressureTaskHandle, 1, eIncrement);
		}
	}
	vTaskDelete(NULL);
}


void Acquisition_continuously_converting_pressure() {
	
	float pressure = 0;
	uint32_t response = 0;
	 
	while (atomic_load(&continuously_acquiring_pressure)) {
//		count = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if (atomic_load(&stack_raw_pressure) != 0) {
			response = atomic_exchange(&stack_raw_pressure, 0);
			// Here goes the conversion but, at this moment only will set in 0.0 the result
			pressure = (float)response * 0.0; 
			pressure = 78.5; 
//			if (pressure > pressure_threshold.max) {
//				if (upper_pressure_threshold_callback != NULL) upper_pressure_threshold_callback();
//			}
			
			if (pressure < pressure_threshold.min) {
				if (lower_pressure_threshold_callback != NULL) lower_pressure_threshold_callback();
				break;
			}
		}
		vTaskDelay(pdMS_TO_TICKS(10));
	}
	vTaskDelete(NULL);
}









