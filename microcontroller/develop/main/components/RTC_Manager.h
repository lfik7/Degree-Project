/*
 * RTC_Manager.h
 *
 *  Created on: 27/12/2025
 *      Author: Lotfi Dalal
 */

#ifndef RTC_MANAGER_H_
#define RTC_MANAGER_H_




#include <sys/param.h>
#include "File_Manager.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"

// Time sync (SNTP)
#include <time.h>
#include <sys/time.h>
#include "esp_sntp.h"


void RTCM_init();
esp_err_t RTCM_initialize_sntp(void);
esp_err_t RTCM_sync_time();
void RTCM_obtener_hora_actual(void);










#endif /* RTC_MANAGER_H_ */
