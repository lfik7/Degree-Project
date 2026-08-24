/*
 * Aquisition_Manager.c
 *
 *  Created on: 26/12/2025
 *      Author: Lotfi Dalal
 */

#include "Acquisition_Manager.h"
#include "Globals.h"
#include "LJ12A3_4_Z.h"
#include "esp_bit_defs.h"
#include "esp_err.h"
#include "esp_private/sar_periph_ctrl.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "instruments/ADS1219.h"
#include "instruments/HX711.h"
#include "portmacro.h"
#include "esp_timer.h"
#include <stdatomic.h>
#include "math.h"


#define TAKE_VARIABLES_SAMPLE_BIT					BIT1
#define SEND_VARIABLES_SAMPLE_BIT					BIT2
#define CONTINUOUSLY_ACQUIRING_PRESSURE_BIT			BIT3
#define STOP_CONTINUOUSLY_ACQUIRING_PRESSURE_BIT	BIT4



static const char * Tag_Acquisition = "Acquisition";

bool LJ12A3_change_state_flag = false;
static uint16_t sample_interval = 0;
static uint16_t sample_quantity_total = 0;
static uint16_t sample_counter = 0;
static float MQ3_RL = 0.950, MQ3_R0 = 0.226;
static float MQ135_RL = 19.73, MQ135_R0 = 2.508; 
static VariablesData_t variables_data_sum = {.timestamp = 0, .temp = 0, .humidity = 0, .pressure = 0, 
											.gas_co2 = 0, .gas_oh = 0, .gas_nit = 0};
static VariablesData_t variables_data_last_measure = {.timestamp = 0, .temp = 0, .humidity = 0, .pressure = 0, 
											.gas_co2 = 0, .gas_oh = 0, .gas_nit = 0};
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


static void LJ12A3_change_state();				// Use only flags in this function
static float Acquisition_get_pressure();
static void Acquisition_continuously_acquiring_pressure();
static void Acquisition_continuously_converting_pressure();
static void Acquisition_send_variables_sample();
static int32_t Acquisition_read_oversampling_ADS1219(uint8_t channel);
static float Acquisition_get_humidity_percentage();
static float Acquisition_get_MQSensor_Resistance(float Vref, float RL, float VRL);
static float Acquisition_get_MQSensor_R0(float RS, float ratio);
static float Acquisition_get_Ratio_MQsensor(float R0, float Rs);
static uint32_t Acquisition_get_ppm_alcohol();
static uint32_t Acquisition_get_ppm_CO2(uint32_t alcohol_ppm);

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
	
	xTaskCreate(Acquisition_main_task, "Main loop", 3072, NULL, 5, &xMainTaskLoopHandel);
	
	
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
	
	sample_interval = sample_interval_in_seconds;
	sample_quantity_total = (uint16_t)(sample_interval/60);
	esp_timer_start_periodic(timer_sample, 60*1000000ULL);
	
	ESP_LOGI(Tag_Acquisition, "Acquisition started!");
	
	atomic_fetch_or(&acquisition_task_flags, TAKE_VARIABLES_SAMPLE_BIT);
	xTaskNotify(xMainTaskLoopHandel, 1, eIncrement);
	
	
}


void Acquisition_set_sample_interval(long long sample_interval_in_seconds){
	sample_interval = sample_interval_in_seconds;
	sample_quantity_total = (uint16_t) sample_interval/60;
	if (sample_counter >= sample_quantity_total) {
		atomic_fetch_or(&acquisition_task_flags, SEND_VARIABLES_SAMPLE_BIT);
		xTaskNotify(xMainTaskLoopHandel, 1, eIncrement);
	}
}

void Acquisition_set_pressure_thresholds(pressureThresholds_t thresholds){
	pressure_threshold = thresholds;
}

void Callback_timer_sample(void* args){
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
			
			if (bits_ask & SEND_VARIABLES_SAMPLE_BIT) {
				Acquisition_send_variables_sample();
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
	
	float Humidity = 0.0, pressure = 0.0;
	uint32_t gas_OH = 0, gas_CO2 = 0, gas_NOx = 0; 
	float temperature = 0.0;

	
	// Here goes the conversion 
	gas_OH = Acquisition_get_ppm_alcohol();
	gas_CO2 = Acquisition_get_ppm_CO2(gas_OH);
	gas_NOx = 0;
	
	// Do the humidity conversion 
	Humidity = Acquisition_get_humidity_percentage();

	pressure = Acquisition_get_pressure();

	
	temperature = DS18B20_get_Temperature_Value();
	
	if (temperature > 125) {	// Avoid incorrect measurement  
		if (variables_data_last_measure.temp != 0) {
			temperature = variables_data_last_measure.temp; 
		}
		else {
			while (temperature > 125) {
				vTaskDelay(pdMS_TO_TICKS(3000));
				temperature = DS18B20_get_Temperature_Value();
			}
		}
	}
	
	variables_data_sum.temp += temperature;
	variables_data_sum.gas_co2 += gas_CO2;
	variables_data_sum.gas_nit += gas_NOx;
	variables_data_sum.gas_oh += gas_OH;
	variables_data_sum.humidity += Humidity;
	variables_data_sum.pressure += pressure;
	
//	ESP_LOGI(Tag_Acquisition, "Pressure sensing time: %llu", (time_end - time_init));

	sample_counter ++;
	
	if (sample_counter == sample_quantity_total) {
		atomic_fetch_or(&acquisition_task_flags, SEND_VARIABLES_SAMPLE_BIT);
		xTaskNotify(xMainTaskLoopHandel, 1, eIncrement);
	}
	
	if (!Acquisition_door_state()) {
		if (pressure > pressure_threshold.max) {
			if (upper_pressure_threshold_callback != NULL) upper_pressure_threshold_callback();
		}
		
		if (pressure < pressure_threshold.min) {
			if (lower_pressure_threshold_callback != NULL) lower_pressure_threshold_callback();
		}
	}
	
	vTaskDelete(NULL);
}

void Acquisition_send_variables_sample() {
	time_t timestamp;	
	
	time(&timestamp);
	variables_data->timestamp = timestamp;
	
	variables_data->temp = variables_data_sum.temp / sample_counter;
	variables_data->humidity = variables_data_sum.humidity / sample_counter;
	variables_data->pressure = variables_data_sum.pressure / sample_counter;
	variables_data->gas_co2 = variables_data_sum.gas_co2 / sample_counter;
	variables_data->gas_oh = variables_data_sum.gas_oh / sample_counter;
	variables_data->gas_nit = variables_data_sum.gas_nit / sample_counter;
	
	variables_data_last_measure = *variables_data;
	
	variables_data_sum.temp = 0.0;
	variables_data_sum.humidity = 0.0;
	variables_data_sum.pressure = 0.0;
	variables_data_sum.gas_co2 = 0.0;
	variables_data_sum.gas_oh = 0.0;
	variables_data_sum.gas_nit = 0.0;
	
	sample_counter = 0;
	
	if (variables_callback != NULL){
		variables_callback();
	}
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
	
	while (atomic_load(&HX711_busy)) vTaskDelay(pdMS_TO_TICKS(1));
	atomic_store(&HX711_busy, true);
	for(iter = 0; iter < 32; iter++){
		stack_raw_response += HX711_read_channel_raw('B',32);
	}
	atomic_store(&HX711_busy, false);
	
	stack_raw_response >>= 5;
	
	// Here goes the conversion but, at this moment only will set in 0.0 the result
	
	weight = (85245.60081 * HX711_convert_To_Voltage(&stack_raw_response) - 212.940107);

	if (weight <= 0) return 0.0;
	
	return weight;
}


float Acquisition_get_pressure(){
	float pressure = 0;
	int32_t stack_raw_response = 0;
	uint8_t iter = 0;
	
	while (atomic_load(&HX711_busy)) vTaskDelay(pdMS_TO_TICKS(1));
	atomic_store(&HX711_busy, true);
	for(iter = 0; iter < 32; iter++){
		stack_raw_response += HX711_read_channel_raw('A',64);
	}
	atomic_store(&HX711_busy, false);
	
	stack_raw_response >>= 5;
	
	// Here goes the conversion but, at this moment only will set in 0.0 the result
	
	stack_raw_response = stack_raw_response * 0.013201062 + 1424.61603636;
	
	pressure = (float)stack_raw_response / 1000;

	variables_data->pressure = pressure;
	
	return pressure;
	
}


void Acquisition_check_pressure() {
	
	float pressure = 0.0;
	
	pressure = Acquisition_get_pressure();
	
	if (!Acquisition_door_state()) {
		if (pressure > pressure_threshold.max) {
			if (upper_pressure_threshold_callback != NULL) upper_pressure_threshold_callback();
		}
		
		if (pressure < pressure_threshold.min) {
			if (lower_pressure_threshold_callback != NULL) lower_pressure_threshold_callback();
		}
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
		stack_raw_response += HX711_read_channel_raw('A',64);
		atomic_store(&HX711_busy, false);
		iter ++;
		if (iter == 32) {
			iter = 0;
			stack_raw_response >>= 5;
			atomic_store(&stack_raw_pressure, stack_raw_response);
			stack_raw_response = 0;
			
			vTaskDelay(pdMS_TO_TICKS(1000));
		}
	}
	vTaskDelete(NULL);
}


void Acquisition_continuously_converting_pressure() {
	
	float pressure = 0;
	uint32_t response = 0;
	 
	while (atomic_load(&continuously_acquiring_pressure)) {
		if (atomic_load(&stack_raw_pressure) != 0) {
			response = atomic_exchange(&stack_raw_pressure, 0);
			pressure = (float)(response * 0.013201062 + 1424.61603636) / 1000;
			
			variables_data->pressure = pressure;
			
			if (pressure < pressure_threshold.min) {
				if (lower_pressure_threshold_callback != NULL) lower_pressure_threshold_callback();
				break;
			}
		}
		vTaskDelay(pdMS_TO_TICKS(10));
	}
	vTaskDelete(NULL);
}


int32_t Acquisition_read_oversampling_ADS1219(uint8_t channel) {
	uint8_t samp_count = 0;
	int32_t channel_sampling_stacking = 0;
	
	for (samp_count = 0; samp_count < 32; samp_count ++) {
		channel_sampling_stacking += ADS1219_read_channel_raw(channel);
	}
	
	channel_sampling_stacking = channel_sampling_stacking >> 5;
	
	return channel_sampling_stacking; 
}

static float Acquisition_get_humidity_percentage() {
	float Humidity = 0;
	
	Humidity = -0.000031719305965 * Acquisition_read_oversampling_ADS1219(3) + 177.3752344;
	
	if (Humidity < 0) {
		Humidity = 0;
	}
	
	return Humidity;
}


float Acquisition_get_MQSensor_Resistance(float Vref, float RL, float VRL)
{
	return ((Vref / VRL) * RL) - RL;
}


float Acquisition_get_MQSensor_R0(float RS, float ratio)
{
	return RS / ratio;
}


float Acquisition_get_Ratio_MQsensor(float R0, float Rs)
{
	return Rs/R0;
}


uint32_t Acquisition_get_ppm_alcohol() {
	float MQ3_volt = 0, MQ3_Rs = 0;
	double ratio = 0;
	int32_t alcohol_ppm = 0;
	
	MQ3_volt = Acquisition_read_oversampling_ADS1219(1) * 3.3 / 8388607;
	MQ3_Rs = Acquisition_get_MQSensor_Resistance(3.3, MQ3_RL, MQ3_volt);
	ratio = Acquisition_get_Ratio_MQsensor(MQ3_R0, MQ3_Rs);		// before the first time, is necessary to measurement the R0 of MQ3 
	
	alcohol_ppm = (-555046.00269 * log(ratio) + 984326.2093);
	
	if (alcohol_ppm < 0) return 0;
	
	return alcohol_ppm;
}



uint32_t Acquisition_get_ppm_CO2(uint32_t alcohol_ppm) {
	float MQ135_volt = 0, MQ135_Rs = 0;
	double alcohol_ratio = 0, ratio = 0;
	int32_t CO2_ppm = 0;
	
	alcohol_ratio = 3.64 - (3.5013975 * exp(alcohol_ppm * -1.2470394 * pow(10,-5)));
	
	MQ135_volt = Acquisition_read_oversampling_ADS1219(0) * 3.3 / 8388607;
	MQ135_Rs = Acquisition_get_MQSensor_Resistance(3.3, MQ135_RL, MQ135_volt);
	ratio = Acquisition_get_Ratio_MQsensor(MQ135_R0, MQ135_Rs);		// before the first time, is necessary to measurement the R0 of MQ3
	
	
	if (alcohol_ratio > 0) {
		ratio = ratio - alcohol_ratio;
	} 
	
	CO2_ppm = (1.9234367 * pow(10, 27)* pow(ratio , -43.7088057));
	
	if (CO2_ppm < 0) return 0;
	
	return CO2_ppm;
}










