/*
 * WiFi_Manager.h
 *
 *  Created on: 26/12/2025
 *      Author: Lotfi Dalal
 */

#ifndef WIFI_MANAGER_H_
#define WIFI_MANAGER_H_




#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include <stdatomic.h>

#include "Globals.h"



#define WIFI_WIFI_STA_RECONNECT_BIT				BIT0
#define WIFI_WIFI_STA_CONNECTED_BIT 			BIT1
#define WIFI_WIFI_STA_FAIL_CONNECTION_BIT		BIT2
#define WIFI_WIFI_STA_START_BIT					BIT3

#define WIFI_WIFI_AP_NEW_CONNECTION_BIT			BIT0
#define WIFI_WIFI_AP_LOST_CONNECTION_BIT		BIT1


// Evento: manejo de conexión / desconexión / IP


esp_err_t WiFi_init(const char* wifi_id, const char* wifi_pass);
void WiFi_set_wifi_nets_available(WiFi_SSID_PSSW_t * all_wifi_nets, uint8_t quantity_positions);
void WiFi_get_current_net_connected(WiFi_SSID_PSSW_t* current_net_connected);
void WiFi_set_sta_callbacks(func_callback_t _wifi_connected_callback, func_callback_t _wifi_disconnected_callback, func_callback_t _wifi_unable_connection_callback);
void WiFi_set_polling_time_try_connect(uint period);
void WiFi_get_wifi_nets_available(WiFi_SSID_PSSW_t * all_wifi_nets, uint8_t quantity_positions);
void WiFi_set_ap_callbacks(func_callback_t _wifi_changes_callback);






#endif /* WIFI_MANAGER_H_ */
