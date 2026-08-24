/*
 * Globlas.h
 *
 *  Created on: 31/12/2025
 *      Author: Lotfi Dalal
 */

#ifndef GLOBALS_H_
#define GLOBALS_H_



#include <stdbool.h>
#include <strings.h>
#include <unistd.h>



// ~~~~~~~~~~~~~~~~~~~~~~~~ Globals ~~~~~~~~~~~~~~~~~~~~~~~~
typedef void (*func_callback_t) (void);
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// ~~~~~~~~~~~~~~~~~~~~~~ Acquisition ~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
    uint32_t valores[6];	// {ads1219_h0, ads1219_h1, ads1219_h2, ads1219_h3, hx711_cha, hx711_chb}
    float temp;				// ds18b20
    bool det_obj;			// lj12a3-4-z
    long long timestamp;
}SensorData_test_t;

typedef struct {
    float temp;				// ds18b20
    float humidity;			// ads1210 ch3 (sen0193)
    float pressure;			// hx711 chb (md-ps002)
	float gas_oh;			// mix ads1219 ch0/1/2 (mq135/3/2)
	float gas_co2;			// mix ads1219 ch0/1/2 (mq135/3/2)
	float gas_nit;			// mix ads1219 ch0/1/2 (mq135/3/2)
    long long timestamp;
}VariablesData_t;

typedef struct{
	bool state;				// true: open, false: close
	long long timestamp;
}doorState_t;

typedef struct{
	float weight;
	long long timestamp;
}weightData_t;

typedef struct{
	bool state;				// true: active, false: stopped
	long long timestamp;
}motorpumpState_t;

typedef struct{
	float min;
	float max;
} pressureThresholds_t;
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


// ~~~~~~~~~~~~~~~~~~~~~~ File Manager ~~~~~~~~~~~~~~~~~~~~~~
typedef struct{
	char WiFi_SSID[32];
	uint16_t SAMP_INT;
	bool Door;
	bool Motorpump;
	float Weight;
	pressureThresholds_t pressure_thresholds;
}current_settigns_t;

typedef struct{
	char WiFi_SSID[32];
	char WiFi_PSSW[32];
}WiFi_SSID_PSSW_t;
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



#endif /* GLOBALS_H_ */
