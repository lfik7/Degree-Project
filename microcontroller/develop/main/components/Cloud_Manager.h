/*
 * Firebase_Manager.h
 *
 *  Created on: 26/12/2025
 *      Author: Lotfi Dalal
 */

#ifndef CLOUD_MANAGER_H_
#define CLOUD_MANAGER_H_




#include <string.h>
#include <stdio.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_event.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "esp_netif.h"


// HTTPS
#include "esp_http_client.h"
#include "esp_tls.h"
#include "cJSON.h"

#include "Globals.h"


// Símbolos generados automáticamente por el compilador para el archivo embebido
extern const uint8_t root_ca_pem_start[] asm("_binary_root_ca_pem_start");
extern const uint8_t root_ca_pem_end[]   asm("_binary_root_ca_pem_end");



#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048


void Cloud_init();
void Cloud_release();
bool Cloud_get_current_settigns(current_settigns_t*);
bool Cloud_get_wifi_nets(WiFi_SSID_PSSW_t*, uint8_t quantity_positions);
bool Cloud_get_wifi_nets_edits(bool * edited);
bool Cloud_upload_wifi_nets(WiFi_SSID_PSSW_t*, uint8_t quantity_positions);
bool Cloud_update_wifi_nets_edits(bool edited, char* app_esp);
bool Cloud_get_sample_interval(long long*);
bool Cloud_post_sensors_data(VariablesData_t*);
bool Cloud_post_door_state(doorState_t*);
bool Cloud_post_motorpump_state(bool state, long long timestamp);
bool cloud_post_food_weight(float weight, long long timestamp);
bool Cloud_update_current_settings(current_settigns_t*, const char* fields);
bool Cloud_get_pressure_thresholds(pressureThresholds_t* thresholds);
bool Cloud_update_monitor_presence();








#endif /* CLOUD_MANAGER_H_ */
