/*
 * Aquisition_Manager.h
 *
 *  Created on: 26/12/2025
 *      Author: Lotfi Dalal
 */

#ifndef ACQUISITION_MANAGER_H_
#define ACQUISITION_MANAGER_H_


#include "ADS1219.h"
#include "DS18B20.h"
#include "HX711.h"
#include "LJ12A3_4_Z.h"
#include "esp_err.h"
#include <time.h>
#include "freertos/semphr.h"


#include "Globals.h"



#define ADS1219_SCL		         GPIO_NUM_22					        /*!< GPIO number used for I2C master clock */
#define ADS1219_SDA	             GPIO_NUM_21					        /*!< GPIO number used for I2C master data  */
#define ADS1219_RDY				 GPIO_NUM_19							/*!< Pin to monitoring the RDY pin from ADS1219 */
#define DS18B20_DQ 				 GPIO_NUM_4
#define HX711_SCK				 GPIO_NUM_18
#define HX711_DOUT				 GPIO_NUM_5
#define LJ12A3_OUT				 GPIO_NUM_33



extern bool LJ12A3_change_state_flag;



esp_err_t Acquisition_init( VariablesData_t* _variables_data, doorState_t* _door_data);
void Acquisition_set_callbacks(func_callback_t _variables_callback, func_callback_t _door_callback);
void Acquisition_start(long long sample_interval_in_seconds);
void Acquisition_set_sample_interval(long long sample_interval_in_seconds);
void Acquisition_test_sensors(SensorData_test_t* Sens_data);
bool Acquisition_door_state();
float Acquisition_food_weight();
void Acquisition_set_pressure_thresholds(pressureThresholds_t thresholds);
void Acquisition_set_pressure_thresholds_callbacks(func_callback_t _upper_pressure_threshold_callback, func_callback_t _lower_pressure_threshold_callback);
void Acquisition_check_pressure();
void Acquisition_continuously_acquiring_pressure_set_state(bool state);



#endif /* ACQUISITION_MANAGER_H_ */
