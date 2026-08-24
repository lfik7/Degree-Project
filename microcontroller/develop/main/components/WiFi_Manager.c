/*
 * WiFi_Manager.c
 *
 *  Created on: 26/12/2025
 *      Author: Lotfi Dalal
 */

#include "WiFi_Manager.h"
#include "Globals.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_wifi_types_generic.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "lwip/sockets.h"
#include "portmacro.h"
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>


static const char *TAG_WiFi_STA = "WIFI_STA", *TAG_WiFi_AP = "WIFI_AP", *TAG_TCP = "TCP", *TAG_TECP_PROCESS = "TCP_PROCESS";

//static char* AP_SSID = "Monitor_silo";
//static char* AP_PASS = "Mon147896325";

static EventGroupHandle_t wifi_sta_event_group, wifi_ap_event_group;

WiFi_SSID_PSSW_t wifi_nets_available[5];
static char WIFI_SSID[32], WIFI_PASS[32];
static int connections_try = 0;
static int wifi_nets_available_index = 0;
static uint pollin_time_try_connect = 300000U;
static QueueHandle_t tcp_process_queue;

typedef struct {
	int socket;
	char buffer[128];
} tcp_packet_t;

static atomic_bool client_connected = false;
static atomic_bool task_tcp_handler_created = false;
 
static wifi_config_t wifi_sta_config = {
    .sta = {
        .threshold.authmode = WIFI_AUTH_WPA2_PSK,
    },
};
static wifi_config_t wifi_ap_config = {
    .ap = {
		.ssid = "Monitor_silo",
		.ssid_len = strlen("Monitor_silo"),
		.channel = 1,
		.password = "Mon147896325",
		.max_connection = 1,
		.authmode = WIFI_AUTH_WPA2_PSK,
	},
};
static func_callback_t wifi_sta_connected_callback = NULL;
static func_callback_t wifi_sta_unable_connection_callback = NULL;
static func_callback_t wifi_sta_disconnected_callback = NULL;
static func_callback_t wifi_changes_callback = NULL;

static void wifi_init_sta_ap(void);
static void wifi_sta_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void wifi_sta_task_handler();
static void wifi_ap_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void wifi_ap_task_handler();
static void tcp_task_handler();
static void tcp_process_task_handler();



// Define the Timer to free/release http client
static StaticTimer_t xTimerWiFiSTAReconnection;
static TimerHandle_t Timer_WiFi_STA_Reconnection;
static void callback_timer_wifi_sta_reconnection(TimerHandle_t xTimer);



esp_err_t WiFi_init(const char* wifi_id, const char* wifi_pass)
{
	ESP_LOGI("WiFi_INIT", "Connected to net \"%s\" and password \"%s\"", wifi_id, wifi_pass);

	strcpy(WIFI_SSID, wifi_id);
	strcpy(WIFI_PASS, wifi_pass);
	
    wifi_sta_event_group = xEventGroupCreate();
    wifi_ap_event_group = xEventGroupCreate();
    	
 	Timer_WiFi_STA_Reconnection = xTimerCreateStatic("Timer HTTP client", pdMS_TO_TICKS(pollin_time_try_connect), pdTRUE, (void *)0, callback_timer_wifi_sta_reconnection, &xTimerWiFiSTAReconnection);
   	xTaskCreate(wifi_sta_task_handler, "STA WIFI TASK HANDLER", 4096, NULL, 10, NULL);
    
    wifi_init_sta_ap();
    
    xTaskCreate(wifi_ap_task_handler, "AP WIFI TASK HANDLER", 4096, NULL, 10, NULL);
	tcp_process_queue = xQueueCreate(10, sizeof(tcp_packet_t));

    // Esperar conexión
    EventBits_t bit_event;
    bit_event = xEventGroupWaitBits(wifi_sta_event_group, WIFI_WIFI_STA_CONNECTED_BIT | WIFI_WIFI_STA_FAIL_CONNECTION_BIT,
                        pdFALSE, pdFALSE, portMAX_DELAY);
                        
	if (bit_event & WIFI_WIFI_STA_FAIL_CONNECTION_BIT){
		ESP_LOGE(TAG_WiFi_STA, "WiFi unable to connection!");
		return ESP_FAIL;
	}

    ESP_LOGI(TAG_WiFi_STA, "WiFi OK, now you can browsing.");
    return ESP_OK;
}


// Evento: manejo de conexión / desconexión / IP
static void wifi_sta_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		xEventGroupSetBits(wifi_sta_event_group, WIFI_WIFI_STA_START_BIT);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		xEventGroupSetBits(wifi_sta_event_group, WIFI_WIFI_STA_RECONNECT_BIT);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG_WiFi_STA, "Connected to net: %s! IP: " IPSTR, WIFI_SSID, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_sta_event_group, WIFI_WIFI_STA_CONNECTED_BIT);
        
    }
}


void wifi_ap_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data){
	if (event_id == WIFI_EVENT_AP_STACONNECTED) {
	    wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
	    ESP_LOGI(TAG_WiFi_AP, "New client connected. MAC: " MACSTR, MAC2STR(event->mac));
	    xEventGroupSetBits(wifi_ap_event_group, WIFI_WIFI_AP_NEW_CONNECTION_BIT);
	} 
	else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
	    ESP_LOGI(TAG_WiFi_AP, "The client has disconnected!");
	    xEventGroupSetBits(wifi_ap_event_group, WIFI_WIFI_AP_LOST_CONNECTION_BIT); 
	}
}


void wifi_init_sta_ap(void)
{
    // Inicializar stack de red
    esp_netif_init();

    // Manejo de eventos
    esp_event_loop_create_default();

    // Crear interfaz WiFi STA
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    // Config WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // Registrar callback
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_sta_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_sta_event_handler, NULL);
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_ap_event_handler, NULL);

    // Configurar modo estación
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    // Configuración del SSID y contraseña
    strcpy((char *)wifi_sta_config.sta.ssid, WIFI_SSID);
    strcpy((char *)wifi_sta_config.sta.password, WIFI_PASS);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));

    // Iniciar WiFi
    ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_sta_task_handler(){
	EventBits_t bits;
	
	while (1){
		
		bits = xEventGroupWaitBits(wifi_sta_event_group, WIFI_WIFI_STA_RECONNECT_BIT | WIFI_WIFI_STA_CONNECTED_BIT | WIFI_WIFI_STA_START_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
		
		if (bits & WIFI_WIFI_STA_START_BIT){
			bool invalid_size = false;
			if ((strlen(WIFI_SSID) > 31) || (strlen(WIFI_PASS) > 31)){
				ESP_LOGE(TAG_WiFi_STA,"SSID or Password too long. Only 31 characters each!");
				invalid_size = true;
			}
			
			if ((strlen(WIFI_SSID) == 0) || strlen(WIFI_PASS) == 0){
				ESP_LOGE(TAG_WiFi_STA,"SSID or Password void!");
				invalid_size = true;
			}
			
			if (!invalid_size){
		        esp_wifi_connect();
		        ESP_LOGI(TAG_WiFi_STA, "Trying to connect...");
	        } else{
				connections_try = 5;
				xEventGroupSetBits(wifi_sta_event_group, WIFI_WIFI_STA_RECONNECT_BIT);
			}
        }
				
		if (bits & WIFI_WIFI_STA_RECONNECT_BIT){ // WIFI_EVENT_STA_DISCONNECTED
			if (xTimerIsTimerActive(Timer_WiFi_STA_Reconnection) == pdFALSE){
				xTimerReset(Timer_WiFi_STA_Reconnection, 100);
			}
			wifi_sta_disconnected_callback();
			if (connections_try < 3){
		    	ESP_LOGI(TAG_WiFi_STA, "Disconnected. Retrying...");
		    	esp_wifi_connect();
		    	connections_try ++; 
		    } else {
				connections_try = 0;
				if (wifi_nets_available_index < 3 && wifi_nets_available[wifi_nets_available_index].WiFi_SSID[0] != '\0'){
					esp_wifi_disconnect();
					snprintf(WIFI_SSID, sizeof(WIFI_SSID), "%s", wifi_nets_available[wifi_nets_available_index].WiFi_SSID);
					snprintf(WIFI_PASS, sizeof(WIFI_PASS), "%s", wifi_nets_available[wifi_nets_available_index].WiFi_PSSW);
				    strcpy((char *)wifi_sta_config.sta.ssid, WIFI_SSID);
				    strcpy((char *)wifi_sta_config.sta.password, WIFI_PASS);
				    esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config);
				    esp_wifi_connect();
				    ESP_LOGI(TAG_WiFi_STA, "Trying with net %s", wifi_nets_available[wifi_nets_available_index].WiFi_SSID);
				    wifi_nets_available_index ++;
				} else {
					ESP_LOGE( TAG_WiFi_STA, "Unable to connect to any available WiFi nets!");
					xEventGroupSetBits(wifi_sta_event_group, WIFI_WIFI_STA_FAIL_CONNECTION_BIT);
					wifi_sta_unable_connection_callback();
				}
			}
		}
		
		if (bits & WIFI_WIFI_STA_CONNECTED_BIT){ // IP_EVENT_STA_GOT_IP
			xTimerStop(Timer_WiFi_STA_Reconnection, 100);
			connections_try = 0;
		    wifi_nets_available_index = 0;
		    wifi_sta_connected_callback();
		}
		
	}
}


void wifi_ap_task_handler(){
	EventBits_t bits;
	
	while (1) {
		bits = xEventGroupWaitBits(wifi_ap_event_group, WIFI_WIFI_AP_NEW_CONNECTION_BIT | WIFI_WIFI_AP_LOST_CONNECTION_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
		
		if (bits & WIFI_WIFI_AP_NEW_CONNECTION_BIT){
			if (atomic_load(&task_tcp_handler_created)) {
				ESP_LOGE(TAG_TCP,"tcp_task_handler and tcp_process_task_handler are running!");
			} 
			else {
				atomic_store(&client_connected, true);
				xTaskCreate(tcp_task_handler, "TCP TASK HANDLER", 4096, NULL, 10, NULL);
				xTaskCreate(tcp_process_task_handler, "TCP_PROCESS_TASK_HANDLER", 4096, NULL, 10, NULL);
			}
    	}
    	
    	if ( bits & WIFI_WIFI_AP_LOST_CONNECTION_BIT) {
			atomic_store(&client_connected, false);
		}
	
	}
}

void tcp_task_handler(){
	atomic_store(&task_tcp_handler_created, true);
	ESP_LOGI(TAG_TCP,"Starting wifi_ap_task_handler...");
	static char tcp_rx_buffer[128];
	
	int listen_sock = -1;
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    // Crear el socket
    listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
     
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Configurar dirección y puerto (ej. 8080)
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(8080);
    bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
	

    // Escuchar y aceptar
    listen(listen_sock, 1);
    
    while (atomic_load(&client_connected)) {
        struct sockaddr_storage source_addr;
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);

        if (sock < 0) {
//            ESP_LOGI(TAG_TCP, "No se pudo aceptar la conexión");
            continue;
        }
        
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        // Bucle de recepción de información
        while (atomic_load(&client_connected)) {
            int len = recv(sock, tcp_rx_buffer, sizeof(tcp_rx_buffer) - 1, 0);

			if (len < 0) {
				if (errno == EAGAIN || errno == EWOULDBLOCK){
//					ESP_LOGI(TAG_TCP, "Timeout, trying again the recv...");
					continue;
				} 
				else {
			        ESP_LOGE(TAG_TCP, "Real error in recv");
			        break;
			    }
			} 
			else if (len == 0) {
			    ESP_LOGI(TAG_TCP, "Connection closed by client");
			    break;
			} 
			else {
				tcp_packet_t packet;
			    tcp_rx_buffer[len] = 0; 
			    packet.socket = sock;
			    printf("Command received: %s\n", tcp_rx_buffer);
			    strncpy(packet.buffer, tcp_rx_buffer, sizeof(packet.buffer) - 1);
			    xQueueSend(tcp_process_queue, &packet, portMAX_DELAY);
			    send(sock, "OK\r\n", 4, 0);
			    
			}
        }

		shutdown(sock, 0);
        close(sock);
	}
	// --- BLOQUE DE LIMPIEZA FINAL ---
    ESP_LOGI(TAG_TCP, "Cleaning up resources...");
    
    if (listen_sock != -1) {
        shutdown(listen_sock, 0);
        close(listen_sock); // LIBERA EL PUERTO 8080
    }
    
	xQueueSend(tcp_process_queue, "CLOSE", portMAX_DELAY);
	ESP_LOGI(TAG_TCP,"Task wifi_ap_task_handler deleted!");
	atomic_store(&task_tcp_handler_created, false);
	vTaskDelete(NULL);
}


void tcp_process_task_handler(){
	
	static char tcp_tx_buffer[128];
	tcp_packet_t packet;
	WiFi_SSID_PSSW_t wifi_net;
	uint8_t max_positions_available = sizeof(wifi_nets_available)/sizeof(WiFi_SSID_PSSW_t);
	uint8_t iter;
	bool change_wifi_nets = false;
	
	while (atomic_load(&client_connected)) {
		
		if (xQueueReceive(tcp_process_queue, &packet, portMAX_DELAY)) {
			if (strcmp(packet.buffer, "CLOSE") == 0) {
				ESP_LOGI(TAG_TECP_PROCESS, "CLOSE command received!");
			}
			else if (packet.buffer[0] == 'w' || packet.buffer[0] == 'W') {
				switch (packet.buffer[1]) {
					case 'a':		// Add WiFi net
						
						printf("The user wants to add a WiFi net:\n");
						uint8_t count_wifi_nets = 0;
						for (count_wifi_nets = 0; count_wifi_nets < max_positions_available; count_wifi_nets ++) {
							if (wifi_nets_available[count_wifi_nets].WiFi_SSID[0] == '\0') break;
						}
						if (count_wifi_nets == max_positions_available){
							snprintf(tcp_tx_buffer, sizeof(tcp_tx_buffer),"wnf\n");
							send(packet.socket, tcp_tx_buffer, strlen(tcp_tx_buffer), 0);
							printf("\tNot positions available. There are 5 nets!\n");
						}
						else {
							sscanf(packet.buffer+2, "%31[^,],%31[^;\n]",wifi_net.WiFi_SSID, wifi_net.WiFi_PSSW);
							strncpy(wifi_nets_available[count_wifi_nets].WiFi_SSID, wifi_net.WiFi_SSID, sizeof(wifi_nets_available[count_wifi_nets].WiFi_SSID)); 
							strncpy(wifi_nets_available[count_wifi_nets].WiFi_PSSW, wifi_net.WiFi_PSSW, sizeof(wifi_nets_available[count_wifi_nets].WiFi_PSSW));
							printf("\tSSID: %s\n\tPASS: %s\n",wifi_nets_available[count_wifi_nets].WiFi_SSID, wifi_nets_available[count_wifi_nets].WiFi_PSSW);
							change_wifi_nets = true;
						}
						break;
					
					case 'c':		// Change WiFi net (only password)
						sscanf(packet.buffer+2, "%31[^,],%31[^;\n]",wifi_net.WiFi_SSID, wifi_net.WiFi_PSSW);
						printf("The user wants to change the password of a WiFi net:\n\tSSID: %s\n\tPASS: %s\n",wifi_net.WiFi_SSID, wifi_net.WiFi_PSSW);
						
						for (iter = 0; iter < max_positions_available; iter ++) {
							if (strcmp(wifi_net.WiFi_SSID, wifi_nets_available[iter].WiFi_SSID) == 0) {
								memset(wifi_nets_available[iter].WiFi_PSSW, 0, sizeof(wifi_nets_available[iter].WiFi_PSSW));
								strncpy(wifi_nets_available[iter].WiFi_PSSW, wifi_net.WiFi_PSSW, sizeof(wifi_nets_available[iter].WiFi_PSSW) - 1);
								break;
							}
						}
						if (iter == max_positions_available){
							snprintf(tcp_tx_buffer, sizeof(tcp_tx_buffer),"wnnf\n");
							send(packet.socket, tcp_tx_buffer, strlen(tcp_tx_buffer), 0);
							printf("\tWiFi net not found!\n");
						}
						else {
							printf("\tPassword successfully changed!\n");
							change_wifi_nets = true;
						}	
						break;
					
					case 'd':		// Delete WiFi net
						sscanf(packet.buffer+2, "%31[^,\n]",wifi_net.WiFi_SSID);
						printf("The user wants to delete a WiFi net:\n\tSSID: %s\n",wifi_net.WiFi_SSID);
						bool net_found = false;
						for (iter = 0; iter < max_positions_available; iter ++) {
							if (strcmp(wifi_net.WiFi_SSID, wifi_nets_available[iter].WiFi_SSID) == 0) {
								memset(wifi_nets_available[iter].WiFi_SSID, 0, sizeof(wifi_nets_available[iter].WiFi_SSID));
								memset(wifi_nets_available[iter].WiFi_PSSW, 0, sizeof(wifi_nets_available[iter].WiFi_PSSW));
								net_found = true;
								continue;
							}
							if (net_found){
								strncpy(wifi_nets_available[iter - 1].WiFi_SSID, wifi_nets_available[iter].WiFi_SSID, sizeof(wifi_nets_available[iter].WiFi_SSID) - 1);
								strncpy(wifi_nets_available[iter - 1].WiFi_PSSW, wifi_nets_available[iter].WiFi_PSSW, sizeof(wifi_nets_available[iter].WiFi_PSSW) - 1);
							}
						}
						
						if (net_found){
							memset(wifi_nets_available[iter - 1].WiFi_SSID, 0, sizeof(wifi_nets_available[iter - 1].WiFi_SSID));
							memset(wifi_nets_available[iter - 1].WiFi_PSSW, 0, sizeof(wifi_nets_available[iter - 1].WiFi_PSSW));
							printf("WiFi net deleted successfully!\n");
							change_wifi_nets = true;
						}
						else {
							printf("\tWiFi net not found!\n");
							snprintf(tcp_tx_buffer, sizeof(tcp_tx_buffer),"wnnf\n");
							send(packet.socket, tcp_tx_buffer, strlen(tcp_tx_buffer), 0);
						}
						break;
						
					case 'n':
						int net_number = -1;
						sscanf(packet.buffer+2, "%d", &net_number);
						net_number --;
						printf("The user wants to put a WiFi net in net %d:\n", net_number);
						sscanf(packet.buffer+3, "%31[^,],%31[^;\n]",wifi_net.WiFi_SSID, wifi_net.WiFi_PSSW);
						strncpy(wifi_nets_available[net_number].WiFi_SSID, wifi_net.WiFi_SSID, sizeof(wifi_nets_available[net_number].WiFi_SSID)); 
						strncpy(wifi_nets_available[net_number].WiFi_PSSW, wifi_net.WiFi_PSSW, sizeof(wifi_nets_available[net_number].WiFi_PSSW));
						printf("\tSSID: %s\n\tPASS: %s\n",wifi_nets_available[net_number].WiFi_SSID, wifi_nets_available[net_number].WiFi_PSSW);
						change_wifi_nets = true;
						break;
					
					case 's':		// Send all WiFi nets
						printf("The user wants to know the available wifi nets\n");
						
						for (iter = 0; iter < sizeof(wifi_nets_available)/sizeof(WiFi_SSID_PSSW_t); iter ++) {
							if (wifi_nets_available[iter].WiFi_SSID[0]!= '\0') {
								snprintf(tcp_tx_buffer, sizeof(tcp_tx_buffer),"wn%u%s,%s\n", iter+1, wifi_nets_available[iter].WiFi_SSID, wifi_nets_available[iter].WiFi_PSSW);
							} 
							else {
								snprintf(tcp_tx_buffer, sizeof(tcp_tx_buffer),"wn%u_,_\n", iter+1);
							}
							
							int ret_send = send(packet.socket, tcp_tx_buffer, strlen(tcp_tx_buffer), 0);
							printf("\ttcp_tx_buffer: %s\tret_send: %d\n\n",tcp_tx_buffer, ret_send);
							if (ret_send == -1) break;
							
							
							vTaskDelay(pdMS_TO_TICKS(50));
						}
						
						break;
					
					default:
						ESP_LOGE(TAG_TECP_PROCESS, "Unknown WiFi instruction!");
						break;
				}
				
				if (change_wifi_nets){
					// Do respective changes...
					change_wifi_nets = false;
					wifi_changes_callback();
				}
				
			}
			else {
				ESP_LOGE(TAG_TECP_PROCESS, "Unknown general instruction!");
			}
			
			memset(wifi_net.WiFi_SSID, 0, sizeof(wifi_net.WiFi_SSID));
			memset(wifi_net.WiFi_PSSW, 0, sizeof(wifi_net.WiFi_PSSW));
		}
		
	}
	
	ESP_LOGI(TAG_TECP_PROCESS, "tcp_process_task_handler deleted!");
	vTaskDelete(NULL);
	
}


void WiFi_set_wifi_nets_available(WiFi_SSID_PSSW_t * all_wifi_nets, uint8_t quantity_positions){
	
	for (uint8_t iter = 0; iter < quantity_positions; iter ++){
		memset(wifi_nets_available[iter].WiFi_SSID, 0, sizeof(wifi_nets_available[iter].WiFi_SSID));
		memset(wifi_nets_available[iter].WiFi_PSSW, 0, sizeof(wifi_nets_available[iter].WiFi_PSSW));
		strncpy(wifi_nets_available[iter].WiFi_SSID, all_wifi_nets[iter].WiFi_SSID, sizeof(wifi_nets_available[iter].WiFi_SSID) - 1);
		strncpy(wifi_nets_available[iter].WiFi_PSSW, all_wifi_nets[iter].WiFi_PSSW, sizeof(wifi_nets_available[iter].WiFi_PSSW) - 1);
	}
	
}


void WiFi_set_sta_callbacks(func_callback_t _wifi_sta_connected_callback, func_callback_t _wifi_disconnected_callback, func_callback_t _wifi_sta_unable_connection_callback){
	wifi_sta_connected_callback = _wifi_sta_connected_callback;
	wifi_sta_unable_connection_callback = _wifi_sta_unable_connection_callback;
	wifi_sta_disconnected_callback = _wifi_disconnected_callback;
}


void WiFi_get_wifi_nets_available(WiFi_SSID_PSSW_t * all_wifi_nets, uint8_t quantity_positions) {
	
	for (uint8_t iter = 0; iter < quantity_positions; iter ++){
		strncpy(all_wifi_nets[iter].WiFi_SSID, wifi_nets_available[iter].WiFi_SSID, sizeof(all_wifi_nets[iter].WiFi_SSID) - 1);
		strncpy(all_wifi_nets[iter].WiFi_PSSW, wifi_nets_available[iter].WiFi_PSSW, sizeof(all_wifi_nets[iter].WiFi_PSSW) - 1);
		printf("WiFi net available %d:\n\tID: %s\n\tPS: %s\n", (int)iter+1, wifi_nets_available[iter].WiFi_SSID, wifi_nets_available[iter].WiFi_PSSW);
	}
}


void WiFi_set_ap_callbacks(func_callback_t _wifi_changes_callback) {
	wifi_changes_callback = _wifi_changes_callback;
}

void WiFi_get_current_net_connected(WiFi_SSID_PSSW_t* current_net_connected){
	
	snprintf(current_net_connected->WiFi_SSID, sizeof(current_net_connected->WiFi_PSSW), "%s", WIFI_SSID);
	snprintf(current_net_connected->WiFi_PSSW, sizeof(current_net_connected->WiFi_PSSW), "%s", WIFI_PASS);
	
}


void callback_timer_wifi_sta_reconnection(TimerHandle_t xTimer){
	connections_try = 3;
    wifi_nets_available_index = 0;
    xEventGroupSetBits(wifi_sta_event_group, WIFI_WIFI_STA_RECONNECT_BIT);
}



void WiFi_set_polling_time_try_connect(uint period){
	bool active = false;
	if (xTimerIsTimerActive(Timer_WiFi_STA_Reconnection)) active = true;
	xTimerChangePeriod(Timer_WiFi_STA_Reconnection, pdMS_TO_TICKS(period), 100);
	if(!active) xTimerStop(Timer_WiFi_STA_Reconnection, 100);
}



