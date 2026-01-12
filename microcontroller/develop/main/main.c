#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "components/Cloud_Manager.h"
#include "components/Motorpump_Manager.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include <esp_timer.h>
#include <stdatomic.h>

// Include own components
#include "Acquisition_Manager.h"
#include "Cloud_Manager.h"
#include "Motorpump_Manager.h"
#include "File_Manager.h"
#include "Cloud_Manager.h"
#include "RTC_Manager.h"
#include "WiFi_Manager.h"
#include "Globals.h"
#include "portmacro.h"


static VariablesData_t variables_data;
static doorState_t door_data;
static motorpumpState_t motorpump_data;
static bool variables_file = false;
static bool door_file = false;
static bool weight_file = false;
static bool motorpump_file = false;

static VariablesData_t variables_data_buffer[40];
static doorState_t door_data_buffer[20];
static weightData_t weight_data_buffer[20];
static motorpumpState_t motorpump_data_buffer[20];
//static pressureThresholds_t pressure_thresholds;
static uint8_t variables_data_buffer_index = 0;
static uint8_t door_data_buffer_index = 0;
static uint8_t weight_data_buffer_index = 0;
static uint8_t motorpump_data_buffer_index = 0;

volatile bool http_client_released = false;					// Flag to know if the client was released
volatile bool wifi_net_changes_cloud_upload = true;			// Flag to know if the WiFi nets changes (by ESP32 AP mode) where upload to the cloud

#define LOST_WIFI_CONNECTION_BIT	BIT0					// Flag to know if the ESP32 is connected to a WiFi net
#define DOOR_EVENT_BIT 				BIT1					// Flag to know if there has been a door event
#define VARIABLES_EVENT_BIT 		BIT2					// Flag to know if there has been a sample event
#define POLLING_UPLOAD_BIT 			BIT3 					// Flag to know if have to look at the cloud for changes
#define RELEASE_HTTP_CLIENT_BIT 	BIT4					// Flag to know if have to release the http client
#define WIFI_CONNECTED_CALLBACK_BIT	BIT5					// Flag to know if the ESP32 has just been connected to WiFi
#define WIFI_UNABLE_CONNECTION_BIT	BIT6					// Flag to know if the ESP32 can´t connect to any WiFi net
#define STORE_DATA_FILE_BIT			BIT7					// Flag to know that is time to store data (from the buffer(s) in file(s)
#define WIFI_NETS_CHANGES_BIT		BIT8					// Flag to know if the client changed any WiFi net (ESP32 in AP mode)
#define UPPER_PRESS_THRESH_BIT		BIT9					// Flag to know if the pressure exceed the upper threshold
#define LOWER_PRESS_THRESH_BIT		BIT10					// Flag to know if the pressure exceed the lower threshold
#define MOTORPUMP_EVENT_BIT			BIT11					// Flag to know if there has been a motorpump event

static atomic_uint_fast32_t main_task_flags = 0;
static atomic_bool wifi_connected = true;
static atomic_bool main_loop_task_started = false;			// Flag to know if the main task has started
    
static current_settigns_t current_project_settings; 	// Store the current settings of the project (door state, motorpump state, food weight, sample interval, current/last connected wifi net)
static WiFi_SSID_PSSW_t Wifi_Net;						// Store the SSID and PASS of the current WiFi net (connected)
static WiFi_SSID_PSSW_t all_wifi_nets_file[5];			// Store all WiFi nets available			

static current_settigns_t settings_in_cloud;			// Store the current settings of the project that are in the cloud 
static WiFi_SSID_PSSW_t all_wifi_nets_cloud[5];		// Store all WiFi nets available that are in the cloud

static TaskHandle_t xMainTaskHandle = NULL;

void main_loop_task();
void check_http_client_released();
void check_http_client_no_released();
void check_pressure_thresholds();
void check_current_settings();
void check_sample_interval();
void check_wifi_nets(bool look_edtis);
void Check_update_data();
void polling_upload();
void polling_upload_handler();
void change_SI();
void variables_event_handler();
uint8_t upload_variables_data_buffer();
uint8_t upload_weight_data_buffer();
void door_event_handler();
uint8_t upload_door_data_buffer();
uint8_t upload_motorpump_data_buffer();
void check_upload_door_weight_motorpump_data();
void upper_pressure_threshold_hanlder();
void lower_pressure_threshold_hanlder();
void motorump_event_handler();
void wifi_connected_handler();
void wifi_unable_connection_hanlder();
void wifi_nets_changes_handler();
void store_data();
void store_variables_data();
void store_door_data();
void store_weight_data();
void store_motorpump_data();
void active_timer_store_data();
void stop_timer_store_data();

void variables_event_callback();
void door_event_callback();
void upper_pressure_threshold_callback();
void lower_pressure_threshold_callback();
void motorpump_event_callback();
void wifi_connected_callback();
void wifi_unable_connection_callback(); 
void wifi_disconnected_callback();
void wifi_changes_callback();


// /* Timer configuartion */
//static void IRAM_ATTR timer_polling_edits_callback(void* args);
//	
// Define the Timer to do polling (sample interval and WiFi nets)
//static esp_timer_handle_t timer_polling_edits = NULL;
//static const esp_timer_create_args_t ConfigTimer_polling_edits = {
//	.callback = timer_polling_edits_callback,
//	.arg = NULL,
//	.name = "timer_sample"
//};


// Define the Timer to free/release http client
static StaticTimer_t xTimerPollingEditsBuffer;
static TimerHandle_t timer_polling_edits;
static void timer_polling_edits_callback(TimerHandle_t xTimer);

// Define the Timer to free/release http client
//static StaticTimer_t xTimerHTTPClientBuffer;
//static TimerHandle_t Timer_HTTP_Client;
//static void timer_http_client_callback(TimerHandle_t xTimer);

// Define the Timer to free/release http client
static StaticTimer_t xTimerStoreDataFileBuffer;
static TimerHandle_t Timer_Store_Data_File;
static void timer_store_data_file_callback(TimerHandle_t xTimer);



void app_main(void)
{
	xTaskCreate(main_loop_task, "Main loop task", 8192, NULL, 14, &xMainTaskHandle);
	
	
	timer_polling_edits = xTimerCreateStatic("Timer Polling Edits", pdMS_TO_TICKS(60000U), pdTRUE, (void *)0, timer_polling_edits_callback, &xTimerPollingEditsBuffer);
	Timer_Store_Data_File = xTimerCreateStatic("Timer Store Data", pdMS_TO_TICKS(/*1800000U*/600000U), pdTRUE, (void *)1, timer_store_data_file_callback, &xTimerStoreDataFileBuffer);
	
	
    // 1. Inicializar NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
    
    Acquisition_init(&variables_data, &door_data);
//    ESP_LOGI("Init", "Acquisition peripherals initialized!");
    Acquisition_set_callbacks(variables_event_callback, door_event_callback);
    Acquisition_set_pressure_thresholds_callbacks( &upper_pressure_threshold_callback, &lower_pressure_threshold_callback);
    
    Motorpump_init(&motorpump_data);
    Motorpump_set_callback(&motorpump_event_callback);
    
    FileM_init();
//    ESP_LOGI("Init", "File manager initialized!");
    
   	printf("\nListando archivos en la particion storage:\n");
    FileM_list_files("/littlefs");
    

    FileM_get_current_settings(&current_project_settings);
    
	FileM_get_all_wifi_nets(all_wifi_nets_file);
	
    if (!FileM_get_wifi_net(current_project_settings.WiFi_SSID, &Wifi_Net)) {
		ESP_LOGE("Init", "No se pudo cargar la red wifi! Utilizando otra!");
	}
	
	if (FileM_get_variables_file_size() > 0){
		ESP_LOGI("Init", "There is a variables file! Removing file...");
//		variables_file = true;
		FileM_remove_variables_file();
	}
	
	if (FileM_get_door_file_size() > 0){
		ESP_LOGI("Init", "There is a door file! Removing file...");
//		door_file = true;
		FileM_remove_door_file();
	}
	
	if (FileM_get_weight_file_size() > 0){
		ESP_LOGI("Init", "There is a weight file! Removing file...");
//		weight_file = true;
		FileM_remove_weight_file();
	}
	
	if (FileM_get_motorpump_file_size() > 0){
		ESP_LOGI("Init", "There is a motorpump file! Removing file...");
//		motorpump_file = true;
		FileM_remove_motorpump_file();
	}
	
	
	WiFi_set_sta_callbacks(wifi_connected_callback, wifi_disconnected_callback, wifi_unable_connection_callback);
	WiFi_set_ap_callbacks(wifi_changes_callback);
    WiFi_set_wifi_nets_available(all_wifi_nets_file, sizeof(all_wifi_nets_file)/sizeof(WiFi_SSID_PSSW_t));
	if (WiFi_init(Wifi_Net.WiFi_SSID, Wifi_Net.WiFi_PSSW) != ESP_OK){
		ESP_LOGE("Init","Can't initialize WiFi!");
//		return;
	} 
//	else{
//		WiFi_get_current_net_connected(&Wifi_Net);
//		if (strncmp(Wifi_Net.WiFi_SSID, current_project_settings.WiFi_SSID, sizeof(current_project_settings.WiFi_SSID)) != 0){
//			snprintf(current_project_settings.WiFi_SSID, sizeof(current_project_settings.WiFi_SSID), "%s", Wifi_Net.WiFi_SSID);
//		}
//		ESP_LOGI("Init", "WiFi initialized and connected!");
////		atomic_fetch_or(&main_task_flags, WIFI_CONNECTED_BIT);
////		atomic_store(&wifi_connected, true);
//	}
	
	if (!atomic_load(&wifi_connected)){
		WiFi_set_polling_time_try_connect(120000U);
		while (!atomic_load(&wifi_connected)){
			vTaskDelay(pdMS_TO_TICKS(10000U));
		}
	}
	
	WiFi_get_current_net_connected(&Wifi_Net);
	if (strncmp(Wifi_Net.WiFi_SSID, current_project_settings.WiFi_SSID, sizeof(current_project_settings.WiFi_SSID)) != 0){
		snprintf(current_project_settings.WiFi_SSID, sizeof(current_project_settings.WiFi_SSID), "%s", Wifi_Net.WiFi_SSID);
		FileM_store_new_settings(&current_project_settings);
	}
	ESP_LOGI("Init", "WiFi initialized and connected!");
		
	
	WiFi_set_polling_time_try_connect(300000U);
	
//	uint32_t bits_ask = atomic_exchange(&main_task_flags, 0);
//	if (atomic_load(&wifi_connected)){
	
    RTCM_init();
    RTCM_obtener_hora_actual();
//    ESP_LOGI("Init","Time synchronized!");
	    
    Cloud_init();
//	printf("Sample interval: %lld\n", samp_int);
	
	vTaskDelay(pdMS_TO_TICKS(2000));
	Check_update_data();
//    ESP_LOGI("Init","Data got from the cloud!");
	vTaskDelay(pdMS_TO_TICKS(2000));
//	}
    
    // Start acquisition system!
    Acquisition_set_pressure_thresholds(current_project_settings.pressure_thresholds);
    Acquisition_start(current_project_settings.SAMP_INT);
    
    ESP_LOGI("Init","System initialized!");
    

	ESP_LOGI("Init", "Sample interval setting in %ld", (long)current_project_settings.SAMP_INT*1000);
	
	atomic_store(&main_loop_task_started, true);
//	if ((current_project_settings.SAMP_INT > 60) && (atomic_load(&wifi_connected))) xTimerReset(Timer_HTTP_Client, 100);
//	if ((current_project_settings.SAMP_INT > 60)) xTimerReset(Timer_HTTP_Client, 100);
//	if (current_project_settings.SAMP_INT > 60){
//	if (timer_polling_edits == NULL){
//		esp_timer_create(&ConfigTimer_polling_edits, &timer_polling_edits);
//		esp_timer_start_periodic(timer_polling_edits, 60ULL * 1000000ULL); // 5 minutes
//	}
//	}
	
	xTimerStart(timer_polling_edits, 100);
	
	vTaskDelay(pdMS_TO_TICKS(100));
	
	uint32_t bits_ask = atomic_exchange(&main_task_flags, 0);
	if (bits_ask & DOOR_EVENT_BIT) xTaskNotify(xMainTaskHandle, 1, eIncrement);
	
	printf("Stack restante: %d bytes\n", uxTaskGetStackHighWaterMark(NULL) * 4);

}



void main_loop_task(){
	
	while(1){
		uint32_t count = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if (count > 0){
			
			uint32_t bits_ask = atomic_exchange(&main_task_flags, 0);
			
			if (bits_ask & VARIABLES_EVENT_BIT){
//				variables_event = false;
				variables_event_handler();
			}
			
			if (bits_ask & DOOR_EVENT_BIT){
//				door_event = false;
				door_event_handler();
			}
			
			if (bits_ask & UPPER_PRESS_THRESH_BIT){
				upper_pressure_threshold_hanlder();
			}
			
			if (bits_ask & LOWER_PRESS_THRESH_BIT) {
				lower_pressure_threshold_hanlder();
			}
			
			if (bits_ask & MOTORPUMP_EVENT_BIT) {
				motorump_event_handler();
			}
			
			if(bits_ask & STORE_DATA_FILE_BIT){
				// Call function to store data
				store_data();
			}
			
			if (bits_ask & POLLING_UPLOAD_BIT){
//				check_WF_SI = false;
				polling_upload_handler();
			}
			
			if (bits_ask & WIFI_CONNECTED_CALLBACK_BIT) {
//				wifi_connected_callback_called = false;
				wifi_connected_handler();
			}
			
			if (bits_ask & LOST_WIFI_CONNECTION_BIT){
				active_timer_store_data();
			}
			
			if (bits_ask & WIFI_UNABLE_CONNECTION_BIT){
//				wifi_unable_connection = false;
				wifi_unable_connection_hanlder();
			}
			
			if (bits_ask & WIFI_NETS_CHANGES_BIT) {
				wifi_nets_changes_handler();
			}
			
			if(bits_ask & RELEASE_HTTP_CLIENT_BIT){
//				release_http_client = false;
				check_http_client_no_released();
			}

	 		printf("Stack restante: %d bytes\n", uxTaskGetStackHighWaterMark(NULL) * 4);
		}
	}
}


void variables_event_callback(){
//	variables_event = true;
	atomic_fetch_or(&main_task_flags, VARIABLES_EVENT_BIT);
	xTaskNotify(xMainTaskHandle, 1, eIncrement);
}

void door_event_callback(){
//	door_event = true;
	atomic_fetch_or(&main_task_flags, DOOR_EVENT_BIT);
	if (atomic_load(&main_loop_task_started)) xTaskNotify(xMainTaskHandle, 1, eIncrement);
}


void upper_pressure_threshold_callback() {
	atomic_fetch_or(&main_task_flags, UPPER_PRESS_THRESH_BIT);
	xTaskNotify(xMainTaskHandle, 1, eIncrement);
}

void lower_pressure_threshold_callback() {
	atomic_fetch_or(&main_task_flags, LOWER_PRESS_THRESH_BIT);
	xTaskNotify(xMainTaskHandle, 1, eIncrement);
}

void motorpump_event_callback(){
	atomic_fetch_or(&main_task_flags, MOTORPUMP_EVENT_BIT);
	xTaskNotify(xMainTaskHandle, 1, eIncrement);
}


void timer_polling_edits_callback(TimerHandle_t xTimer){
//	check_WF_SI = true;
	atomic_fetch_or(&main_task_flags, POLLING_UPLOAD_BIT);
	xTaskNotify(xMainTaskHandle, 1, eIncrement);
}

//void timer_http_client_callback(TimerHandle_t xTimer){
////	release_http_client = true;
//	atomic_fetch_or(&main_task_flags, RELEASE_HTTP_CLIENT_BIT);
//	xTaskNotify(xMainTaskHandle, 1, eIncrement);
//}

void timer_store_data_file_callback(TimerHandle_t xTimer){
	atomic_fetch_or(&main_task_flags, STORE_DATA_FILE_BIT);
	xTaskNotify(xMainTaskHandle, 1, eIncrement);
}

void wifi_connected_callback(){
	atomic_store(&wifi_connected, true);
	if (atomic_load(&main_loop_task_started)) {
//		wifi_connected_callback_called = true;
		atomic_fetch_or(&main_task_flags, WIFI_CONNECTED_CALLBACK_BIT);
	 	xTaskNotify(xMainTaskHandle, 1, eIncrement);
	 }
}

void wifi_unable_connection_callback(){
	if (atomic_load(&main_loop_task_started)) {
//		wifi_unable_connection = true;
		atomic_fetch_or(&main_task_flags, WIFI_UNABLE_CONNECTION_BIT);
		xTaskNotify(xMainTaskHandle, 1, eIncrement);
	}
} 

void wifi_disconnected_callback(){
	if (atomic_load(&wifi_connected)) atomic_store(&wifi_connected,false);
	if (atomic_load(&main_loop_task_started)){
		atomic_fetch_or(&main_task_flags, LOST_WIFI_CONNECTION_BIT);
		xTaskNotify(xMainTaskHandle, 1, eIncrement);
	}
}


void wifi_changes_callback(){
	atomic_fetch_or(&main_task_flags, WIFI_NETS_CHANGES_BIT);
	xTaskNotify(xMainTaskHandle, 1, eIncrement);
}

void check_http_client_released(){
	if (http_client_released) {
		Cloud_init();
		http_client_released = false;
	}
//	if (release_http_client) release_http_client = false;
}

void check_http_client_no_released(){
	if (!http_client_released){
		Cloud_release();
		http_client_released = true;
		printf("HTTP client cleaned up!\n");
	}
}


void active_timer_store_data(){
	if (xTimerIsTimerActive(Timer_Store_Data_File) == pdFALSE) {
	    xTimerReset(Timer_Store_Data_File, 0);
	}
}

void stop_timer_store_data(){
	if (xTimerIsTimerActive(Timer_Store_Data_File) != pdFALSE) {
	    xTimerStop(Timer_Store_Data_File, 0);
	}
}

void variables_event_handler(){
	
	bool weight_change = false;
		
	if (variables_data.weight != current_project_settings.Weight){ 
		current_project_settings.Weight = variables_data.weight;
		FileM_store_new_settings(&current_project_settings);
		weight_change = true;
	}
	
	if (atomic_load(&wifi_connected)){
		check_http_client_released();
		
		printf("Subiendo muestra a la nube\n");
		
		if (variables_data_buffer_index == 0 && !variables_file){
			if (!Cloud_post_sensors_data(&variables_data)){
				ESP_LOGE("Variables_hand", "No se pudo subir los datos de las variables a la nube");
				variables_data_buffer[variables_data_buffer_index] = variables_data;
				variables_data_buffer_index ++;
			}
		} 
		else {
			if (variables_file){
				uint8_t data_size = 0;
				bool file_readed = false;
				variables_data_buffer[variables_data_buffer_index] = variables_data;
				variables_data_buffer_index ++;
				store_variables_data();
				FileM_open_variables_data_file();
				while (!file_readed){
					file_readed = FileM_get_variables_data(variables_data_buffer, &data_size);
					printf("Data cargada (data_size): %u\n", (uint)data_size);
					variables_data_buffer_index = data_size;
					if (upload_variables_data_buffer() < data_size){
						ESP_LOGE("Variables_hand","Error getting and upload variables file!");
						break;
					}
				}
				FileM_close_variables_data_file();
				FileM_remove_variables_file();
				variables_file = false;
				if (file_readed){
					ESP_LOGI("Variables_hand","variables file uploaded and removed!");
				}
			}
			else if (variables_data_buffer_index > 0) {
				variables_data_buffer[variables_data_buffer_index] = variables_data;
				variables_data_buffer_index ++;
				upload_variables_data_buffer();
			}
		}
		
		if (weight_change){
			if (!weight_file){
				if (weight_data_buffer_index == 0){
					if (!cloud_post_food_weight(variables_data.weight, variables_data.timestamp)){
						ESP_LOGE("Variables_hand", "No se pudo subir los datos del peso a la nube");
						weight_data_buffer[weight_data_buffer_index].weight = variables_data.weight;
						weight_data_buffer[weight_data_buffer_index].timestamp = variables_data.timestamp;
						weight_data_buffer_index ++;
						if (weight_data_buffer_index == 20) store_weight_data();
					}		
				} 
				else {
					weight_data_buffer[weight_data_buffer_index].weight = variables_data.weight;
					weight_data_buffer[weight_data_buffer_index].timestamp = variables_data.timestamp;
					weight_data_buffer_index ++;
					
					upload_weight_data_buffer();
				}
			}
			if (!Cloud_update_current_settings(&current_project_settings, "W")){
				ESP_LOGE("Variables_hand", "No se pudo actualizar los datos de configuración en la nube");
			}
		}
		
//		if (current_project_settings.SAMP_INT <= 60){
////			check_WF_SI = true;
//			atomic_fetch_or(&main_task_flags, POLLING_UPLOAD_BIT);
//			xTaskNotify(xMainTaskHandle, 1, eIncrement);
//		} 
//		else{
//			if (current_project_settings.SAMP_INT > 60) xTimerReset(Timer_HTTP_Client, 100);
//		}
		
	} 
	else {
		printf("WiFi not connected! Can't upload sensors data!\n");
		variables_data_buffer[variables_data_buffer_index] = variables_data;
		variables_data_buffer_index ++;
		
		if (weight_change){
			weight_data_buffer[weight_data_buffer_index].weight = variables_data.weight;
			weight_data_buffer[weight_data_buffer_index].timestamp = variables_data.timestamp;
			weight_data_buffer_index ++;
			if (weight_data_buffer_index == 20) store_weight_data();
		}
	}
}


uint8_t upload_variables_data_buffer(){
	ESP_LOGI("Upload variables","Upload variables data to the cloud!");
	uint8_t iter;
	for (iter = 0; iter < variables_data_buffer_index; iter ++){
		uint8_t tries = 0;
		do{
			if (Cloud_post_sensors_data(&variables_data_buffer[iter])){
				break;
			}
			ESP_LOGE("Upload variables", "No se pudo subir los datos de las variables a la nube");
			tries ++;
		}while (tries < 3);
		if (tries == 3) break;
	}
	if (iter < variables_data_buffer_index && iter > 0) {
		ESP_LOGI("Upload variables", "Corriendo datos del buffer...");
		for (uint8_t an_iter = 0; an_iter < variables_data_buffer_index - iter; an_iter ++) {
			variables_data_buffer[an_iter] = variables_data_buffer[an_iter + iter];
		}
		variables_data_buffer_index -= iter;
		ESP_LOGI("Upload variables", "Se aconseja guadar los datos del buffer (variables) en un archivo! (1)");
	} 
	else if (iter == variables_data_buffer_index){
		ESP_LOGI("Upload variables", "%u datos del buffer subidos correctamente!", (uint)variables_data_buffer_index);
		variables_data_buffer_index = 0;
	} 
	else if (iter == 0){
		if (variables_data_buffer_index == 40){
			store_variables_data();
		}
		else {
			ESP_LOGI("Upload variables", "Se aconseja guadar los datos del buffer (variables) en un archivo! (2)");
		}
	}
	return iter;
}

uint8_t upload_weight_data_buffer(){
	ESP_LOGI("Upload weight","Upload weight data to the cloud!");
	uint8_t iter;
	for (iter = 0; iter < weight_data_buffer_index; iter ++){
		uint8_t tries = 0;
		do{
			if (cloud_post_food_weight(weight_data_buffer[iter].weight, weight_data_buffer[iter].timestamp)){
				break;
			}
			ESP_LOGE("Upload weight", "No se pudo subir los datos del peso a la nube");
			tries ++;
		}while (tries < 3);
		if (tries == 3) break;
	}
	if (iter < weight_data_buffer_index && iter > 0) {
		ESP_LOGI("Upload weight", "Corriendo datos del buffer...");
		for (uint8_t an_iter = 0; an_iter < weight_data_buffer_index - iter; an_iter ++) {
			weight_data_buffer[an_iter] = weight_data_buffer[an_iter + iter];
		}
		weight_data_buffer_index -= iter;
		ESP_LOGI("Upload weight", "Se aconseja guadar los datos del buffer (peso) en un archivo! (1)");
	} 
	else if (iter == weight_data_buffer_index){
		ESP_LOGI("Upload weight", "%u datos del buffer subidos correctamente!", (uint) weight_data_buffer_index);
		weight_data_buffer_index = 0;
	} 
	else if (iter == 0){
		if (weight_data_buffer_index == 20) {
			store_weight_data();
		}
		else {
			ESP_LOGI("Upload weight", "Se aconseja guadar los datos del buffer (peso) en un archivo! (2)");
		}
	}
	return iter;
}


void door_event_handler(){
	
	current_project_settings.Door = door_data.state;
	FileM_store_new_settings(&current_project_settings);
	
	if (atomic_load(&wifi_connected)){
		
		check_http_client_released();
		
		if (door_data_buffer_index == 0){ 
			if (!Cloud_post_door_state(&door_data)){
				ESP_LOGE("Door_hand", "No se pudo subir los datos de la puerta a la nube");
				door_data_buffer[door_data_buffer_index] = door_data;
				door_data_buffer_index ++;
				if (door_data_buffer_index == 20) store_door_data();
			}
		}
		else {
			
			door_data_buffer[door_data_buffer_index] = door_data;
			door_data_buffer_index ++;
			upload_door_data_buffer();
		}
			
		Cloud_update_current_settings(&current_project_settings, "D");
		 
//		if (current_project_settings.SAMP_INT > 60) xTimerReset(Timer_HTTP_Client, 100);
	} 
	else {
		printf("WiFi not connected! Can't upload door data!\n");
		door_data_buffer[door_data_buffer_index] = door_data;
		door_data_buffer_index ++;
		if (door_data_buffer_index == 20) store_door_data();
	}
		
}


uint8_t upload_door_data_buffer(){
	ESP_LOGI("Upload door","Upload door data to the cloud!");
	uint8_t iter;
	for (iter = 0; iter < door_data_buffer_index; iter ++){
		uint8_t tries = 0;
		do{
			if (Cloud_post_door_state(&door_data_buffer[iter])){
				break;
			}
			ESP_LOGE("Door_hand", "No se pudo subir los datos de la puerta a la nube");
			tries ++;
		}while (tries < 3);
		if (tries == 3) break;
	}
	if (iter < door_data_buffer_index && iter > 0) {
		ESP_LOGI("Door_hand", "Corriendo datos del buffer...");
		for (uint8_t an_iter = 0; an_iter < door_data_buffer_index - iter; an_iter ++) {
			door_data_buffer[an_iter] = door_data_buffer[an_iter + iter];
		}
		door_data_buffer_index -= iter;
		ESP_LOGI("Door_hand", "Se aconseja guadar los datos del buffer (puerta) en un archivo! (1)");
	} 
	else if (iter == door_data_buffer_index){
		ESP_LOGI("Door_hand", "%u datos del buffer subidos correctamente!", (uint)door_data_buffer_index);
		door_data_buffer_index = 0;
	} 
	else if (iter == 0){
		if (door_data_buffer_index == 20){
			store_door_data();
		}
		else {
			ESP_LOGI("Door_hand", "Se aconseja guadar los datos del buffer (puerta) en un archivo! (2)");
		}
	}
	return iter;
}


void motorump_event_handler() {
	
	if (motorpump_data.state) {
		Acquisition_continuously_acquiring_pressure_set_state(true);		// start
	} 
	else {
		Acquisition_continuously_acquiring_pressure_set_state(false);	// start
	}
	
	current_project_settings.Motorpump = motorpump_data.state;
	FileM_store_new_settings(&current_project_settings);
	
	if (atomic_load(&wifi_connected)){
		
		check_http_client_released();
		
		if (motorpump_data_buffer_index == 0){ 
			if (!Cloud_post_motorpump_state(motorpump_data.state, motorpump_data.timestamp)){
				ESP_LOGE("Motorpump_hand", "No se pudo subir los datos de la motobomba a la nube");
				motorpump_data_buffer[motorpump_data_buffer_index] = motorpump_data;
				motorpump_data_buffer_index ++;
				if (motorpump_data_buffer_index == 20) store_motorpump_data();
			}
		}
		else {
			
			motorpump_data_buffer[motorpump_data_buffer_index] = motorpump_data;
			motorpump_data_buffer_index ++;
			upload_motorpump_data_buffer();
		}
			
		Cloud_update_current_settings(&current_project_settings, "M");
		 
//		if (current_project_settings.SAMP_INT > 60) xTimerReset(Timer_HTTP_Client, 100);
	} 
	else {
		printf("WiFi not connected! Can't upload motorpump data!\n");
		motorpump_data_buffer[motorpump_data_buffer_index] = motorpump_data;
		motorpump_data_buffer_index ++;
		if (motorpump_data_buffer_index == 20) store_motorpump_data();
	}
		
}



uint8_t upload_motorpump_data_buffer(){
	ESP_LOGI("Upload motorpump","Upload motorpump data to the cloud!");
	uint8_t iter;
	for (iter = 0; iter < motorpump_data_buffer_index; iter ++){
		uint8_t tries = 0;
		do{
			if (Cloud_post_motorpump_state(motorpump_data_buffer[iter].state, motorpump_data_buffer[iter].timestamp)){ 
				break;
			}
			ESP_LOGE("Motorpump_upl", "No se pudo subir los datos del motor a la nube");
			tries ++;
		}while (tries < 3);
		if (tries == 3) break;
	}
	if (iter < motorpump_data_buffer_index && iter > 0) {
		ESP_LOGI("Motorpump_upl", "Corriendo datos del buffer...");
		for (uint8_t an_iter = 0; an_iter < motorpump_data_buffer_index - iter; an_iter ++) {
			motorpump_data_buffer[an_iter] = motorpump_data_buffer[an_iter + iter];
		}
		motorpump_data_buffer_index -= iter;
		ESP_LOGI("Motorpump_upl", "Se aconseja guadar los datos del buffer (puerta) en un archivo! (1)");
	} 
	else if (iter == motorpump_data_buffer_index){
		ESP_LOGI("Motorpump_upl", "%u datos del buffer subidos correctamente!", (uint)motorpump_data_buffer_index);
		motorpump_data_buffer_index = 0;
	} 
	else if (iter == 0){
		if ( motorpump_data_buffer_index == 20) {
			store_motorpump_data();
		}
		else {
			ESP_LOGI("Motorpump_upl", "Se aconseja guadar los datos del buffer (puerta) en un archivo! (2)");
		}
	}
	return iter;
	
}


void check_upload_door_weight_motorpump_data(){
	ESP_LOGI("ChkUpdDWM","Checking door, weight, and/or motorpump data (buffers and files)...");
	uint8_t data_size = 0;
	bool file_readed = false;
	
	if (door_file || door_data_buffer_index > 0){
		if (door_file) {
			if (door_data_buffer_index > 0) store_door_data();
			FileM_open_door_data_file();
			while (!file_readed){
				file_readed = FileM_get_door_data(door_data_buffer, &data_size);
				door_data_buffer_index = data_size;
				if (upload_door_data_buffer() < data_size){
					ESP_LOGE("ChkUpdDWM","Error getting and upload door file!");
					break;
				}
			}
			FileM_close_door_data_file();
			FileM_remove_door_file();
			door_file = false;
			if (file_readed){
				ESP_LOGI("ChkUpdDWM","Door file uploaded and removed!");
			}
		} 
		else if (door_data_buffer_index > 0){
			upload_door_data_buffer();
		}
	}
	
	if (weight_file || weight_data_buffer_index > 0){
		if (weight_file) {
			if (weight_data_buffer_index > 0) store_weight_data();
			FileM_open_weight_data_file();
			while (!file_readed){
				file_readed = FileM_get_weight_data(weight_data_buffer, &data_size);
				weight_data_buffer_index = data_size;
				if (upload_weight_data_buffer() < data_size){
					ESP_LOGE("ChkUpdDWM","Error getting and upload weight file!");
					break;
				}
			}
			FileM_close_weight_data_file();
			FileM_remove_weight_file();
			weight_file = false;
			if (file_readed){
				ESP_LOGI("ChkUpdDWM","Weight file uploaded and removed!");
			}
		} 
		else if (weight_data_buffer_index > 0){
			upload_weight_data_buffer();
		}
	}
	
	if (motorpump_file || motorpump_data_buffer_index > 0){
		if (motorpump_file) {
			if (motorpump_data_buffer_index > 0) store_motorpump_data();
			FileM_open_motorpump_data_file();
			while (!file_readed){
				file_readed = FileM_get_motorpump_data(motorpump_data_buffer, &data_size);
				motorpump_data_buffer_index = data_size;
				if (upload_motorpump_data_buffer() < data_size){
					ESP_LOGE("ChkUpdDWM","Error getting and upload motorpump file!");
					break;
				}
			}
			FileM_close_motorpump_data_file();
			FileM_remove_motorpump_file();
			motorpump_file = false;
			if (file_readed){
				ESP_LOGI("ChkUpdDWM","Motorpump file uploaded and removed!");
			}
		} 
		else if (motorpump_data_buffer_index > 0){
			upload_motorpump_data_buffer();
		}
	}
}



void upper_pressure_threshold_hanlder() {
	printf("The pressure is upper than threshold. Please turn on the motorpump!\n");
	Motorpump_turn_on();
}

void lower_pressure_threshold_hanlder() {
	printf("The pressure is lower than threshold. Please turn off the motorpump!\n");
	Motorpump_turn_off();
}


void wifi_connected_handler(){
	stop_timer_store_data();
	WiFi_get_current_net_connected(&Wifi_Net);
	if (strncmp(Wifi_Net.WiFi_SSID, current_project_settings.WiFi_SSID, sizeof(current_project_settings.WiFi_SSID)) != 0){
		snprintf(current_project_settings.WiFi_SSID, sizeof(current_project_settings.WiFi_SSID), "%s", Wifi_Net.WiFi_SSID);
	}
	check_http_client_released();
	Check_update_data();
	check_upload_door_weight_motorpump_data();
//	if (current_project_settings.SAMP_INT > 60) xTimerReset(Timer_HTTP_Client, 100);
	// Here can go the logic to upload the data of the sensors (which could not be upload before)
}



void wifi_unable_connection_hanlder(){
	atomic_store(&wifi_connected,false);
	check_http_client_no_released();
//	if (xTimerIsTimerActive(Timer_HTTP_Client) != pdFALSE) {
//	    xTimerStop(Timer_HTTP_Client, 0);
//	}
}

void wifi_nets_changes_handler() {
	wifi_net_changes_cloud_upload = false;
	
	uint8_t wifi_nets_positions = sizeof(all_wifi_nets_file)/sizeof(WiFi_SSID_PSSW_t);
	
	WiFi_get_wifi_nets_available(all_wifi_nets_file, wifi_nets_positions);
	FileM_store_wifi_net(all_wifi_nets_file, wifi_nets_positions, true);
	
	for (uint8_t iter = 0; iter < wifi_nets_positions; iter ++) {
		all_wifi_nets_cloud[iter] = all_wifi_nets_file[iter];
	}
	
	if (atomic_load(&wifi_connected)) {
		
		check_http_client_released();
		
		if (Cloud_upload_wifi_nets(all_wifi_nets_cloud, wifi_nets_positions)) {
			wifi_net_changes_cloud_upload = true;
			Cloud_update_wifi_nets_edits(true, "ESP");
		}
		else {
			ESP_LOGE("WiFi_NETS_CHANGES", "Could not upload the WiFi nets changes!");
		}
		
		
//		if (current_project_settings.SAMP_INT > 60) xTimerReset(Timer_HTTP_Client, 100);
	}
}


void polling_upload_handler(){
	if (atomic_load(&wifi_connected)){
		check_http_client_released();
		polling_upload();
//		if (current_project_settings.SAMP_INT > 60) xTimerReset(Timer_HTTP_Client, 100);
	}
}


void polling_upload(){
//	long long samp_int = 0;
	const char* TAG_Function = "Polling_Upload";


	check_sample_interval();
	
	check_pressure_thresholds();
	
	check_wifi_nets(true);
	
	if (!Cloud_update_monitor_presence()) {
		ESP_LOGE(TAG_Function, "Could not upload his presence!");
	}
	
}


void change_SI(){
	
//	if (current_project_settings.SAMP_INT > 60){
//		if (timer_polling_edits == NULL){
//			esp_timer_create(&ConfigTimer_polling_edits, &timer_polling_edits);
//			esp_timer_start_periodic(timer_polling_edits, 60ULL * 1000000ULL); // 5 minutes
//		}
//	} else{
//		if (timer_polling_edits != NULL){
//			esp_timer_stop(timer_polling_edits);
//			esp_timer_delete(timer_polling_edits);
//			timer_polling_edits = NULL;
//		}
//	}
	
	Acquisition_set_sample_interval(current_project_settings.SAMP_INT);
}



void check_pressure_thresholds() {
	const char* TAG_Function = "CHECK_PRESSURE_TRESHOLDS";
	pressureThresholds_t current_pressure_thresholds;
	
	if (!Cloud_get_pressure_thresholds(&current_pressure_thresholds)){
		ESP_LOGE(TAG_Function, "Could not download the pressure thresholds!");
		return;
	}
		
	if (current_project_settings.pressure_thresholds.min != current_pressure_thresholds.min || current_project_settings.pressure_thresholds.max != current_pressure_thresholds.max) {
		current_project_settings.pressure_thresholds = current_pressure_thresholds;
		settings_in_cloud.pressure_thresholds = current_pressure_thresholds;
		
		Acquisition_set_pressure_thresholds(current_project_settings.pressure_thresholds);
		Acquisition_check_pressure();
		
		ESP_LOGI(TAG_Function, "Pressure thresholds changed!");
		
		ESP_LOGI(TAG_Function, "Updating changes in settings (file and cloud)...");
		FileM_store_new_settings(&current_project_settings);
		if (!Cloud_update_current_settings(&settings_in_cloud, "P")){
			ESP_LOGE(TAG_Function,"Can´t upload the current settings to the cloud!");
		}
	}
}


void check_current_settings(){
	const char* TAG_Function = "CHECK_CURRENT_SETTINGS";
	
	bool change_settings = false/*, change_wifiNets_file = false*/;
	char fields[5] = {'\0', '\0', '\0', '\0', '\0'};
	int field_pointer = 0;
	bool motorpump = false;
	float weight = 0.0;
	doorState_t door_data_sens = {false, 0};
	
	
    if (!Cloud_get_current_settigns(&settings_in_cloud)){
		ESP_LOGE(TAG_Function, "Can't download the settings!");
		return;
	}
	
	
	door_data_sens.state = Acquisition_door_state();
	weight = Acquisition_food_weight();
	time_t timestamp;
	time(&timestamp);
	door_data_sens.timestamp = timestamp;
	
//	ESP_LOGI("CHECKING_UPDATE", "Checking door...");
	if (door_data_sens.state != current_project_settings.Door){
		current_project_settings.Door = door_data_sens.state;
		change_settings = true;
	}
	if (door_data_sens.state != settings_in_cloud.Door){
		settings_in_cloud.Door = door_data_sens.state;
		fields[field_pointer] = 'D';
		field_pointer ++;
		change_settings = true;
		Cloud_post_door_state(&door_data_sens);
	}
	
//	ESP_LOGI("CHECKING_UPDATE", "Checking weight...");
	if (weight != current_project_settings.Weight){
		current_project_settings.Weight = weight;
		change_settings = true;
	}
	if (weight != settings_in_cloud.Weight){
		settings_in_cloud.Weight = weight;
		fields[field_pointer] = 'w';
		field_pointer ++;
		change_settings = true;
		cloud_post_food_weight(weight, (long long)timestamp);
	}
	
//	ESP_LOGI("CHECKING_UPDATE", "Checking motorpump...");
	if (motorpump != current_project_settings.Motorpump){
		current_project_settings.Motorpump = motorpump;
		change_settings = true;
	}
	if (motorpump != settings_in_cloud.Motorpump){
		settings_in_cloud.Motorpump = motorpump;
		fields[field_pointer] = 'M';
		field_pointer ++;
		change_settings = true;
		Cloud_post_motorpump_state(motorpump, (long long) timestamp);
	}
	
//	ESP_LOGI("CHECKING_UPDATE", "Checking WiFi net connected...");
	if (strcmp(settings_in_cloud.WiFi_SSID, current_project_settings.WiFi_SSID) != 0){
		snprintf(settings_in_cloud.WiFi_SSID, sizeof(settings_in_cloud.WiFi_SSID), "%s", 
				current_project_settings.WiFi_SSID);
		change_settings = true;
		fields[field_pointer] = 'W';
	}
	
	if (change_settings){
		ESP_LOGI(TAG_Function, "Updating changes in settings (file and cloud)...");
		FileM_store_new_settings(&current_project_settings);
		if (!Cloud_update_current_settings(&settings_in_cloud, fields)){
			ESP_LOGE(TAG_Function,"Can´t upload the current settings to the cloud!");
		}
	}
}


void check_sample_interval(){
	const char* TAG_Function = "CHECK_SAMPLE_INTERVAL";
	long long samp_int = 0;
	bool change_settings = false;
	
	if(!Cloud_get_sample_interval(&samp_int)){
		ESP_LOGE("Init", "Can't download the sample interval!");
		return;
	}
	
////	ESP_LOGI("CHECKING_UPDATE", "Checking sample interval...");
	if (samp_int != current_project_settings.SAMP_INT){
		change_settings = true;
		current_project_settings.SAMP_INT = (int)samp_int;
		change_SI();
	}
	if (samp_int != settings_in_cloud.SAMP_INT){
		change_settings = true;
		settings_in_cloud.SAMP_INT = (int)samp_int;
	}
	
	if (change_settings){
		ESP_LOGI(TAG_Function, "Updating changes in settings (file and cloud)...");
		FileM_store_new_settings(&current_project_settings);
		if (!Cloud_update_current_settings(&settings_in_cloud, "S")){
			ESP_LOGE(TAG_Function,"Can´t upload the current settings to the cloud!");
		}
	}
}


void check_wifi_nets(bool look_edits){
	const char* TAG_Function = "CHECK_WIFI_NETS";
	bool change_wifiNets_file = false;
	bool wifi_nets_edited = false;
	uint8_t quantity_wifi_nets = sizeof(all_wifi_nets_file)/sizeof(WiFi_SSID_PSSW_t);
		
	if (!wifi_net_changes_cloud_upload){ 
		if (Cloud_upload_wifi_nets(all_wifi_nets_cloud, quantity_wifi_nets)) {
			wifi_net_changes_cloud_upload = true;
			Cloud_update_wifi_nets_edits(true, "ESP");
		}
		else {
			ESP_LOGE("WiFi_NETS_CHANGES", "Could not upload the WiFi nets changes!");
		}
		return;
	}

	if (look_edits) {
		if (!Cloud_get_wifi_nets_edits(&wifi_nets_edited)){
			ESP_LOGE(TAG_Function, "Can't download the wifi nets edits!");
		}
	}
	
	
	if (look_edits && !wifi_nets_edited) return;
	
    if (!Cloud_get_wifi_nets(all_wifi_nets_cloud, quantity_wifi_nets)){
		ESP_LOGE("Init", "Can't download the wifi nets!");
		return;
	}
	
	for(uint8_t iter = 0; iter < quantity_wifi_nets; iter++){ // hay que evitar que borre/modifique la red a la que está conectado!!!
		if (strcmp(all_wifi_nets_file[iter].WiFi_SSID, all_wifi_nets_cloud[iter].WiFi_SSID) != 0){
			snprintf(all_wifi_nets_file[iter].WiFi_SSID, sizeof(all_wifi_nets_file[iter].WiFi_SSID), "%s", all_wifi_nets_cloud[iter].WiFi_SSID);
			change_wifiNets_file = true;
		}
		if (strcmp(all_wifi_nets_file[iter].WiFi_PSSW, all_wifi_nets_cloud[iter].WiFi_PSSW) != 0){
			snprintf(all_wifi_nets_file[iter].WiFi_PSSW, sizeof(all_wifi_nets_file[iter].WiFi_PSSW), "%s", all_wifi_nets_cloud[iter].WiFi_PSSW);
			change_wifiNets_file = true;
		}
	}
	
	if (change_wifiNets_file){
		ESP_LOGI("CHECKING_UPDATE", "Updating file WiFi_Nets.csv...");
		FileM_store_wifi_net(all_wifi_nets_file, quantity_wifi_nets, true);
		WiFi_set_wifi_nets_available(all_wifi_nets_file, quantity_wifi_nets);
	} 
	else{
		ESP_LOGI(TAG_Function, "There are not changes in the nets!");
	}
	if (wifi_nets_edited) {
		if (!Cloud_update_wifi_nets_edits(false, "APP")){
			ESP_LOGE(TAG_Function, "could not update the wifi nets edits!");
		}
	}
}


void Check_update_data(){
	const char* TAG_Function = "CHECKING_UPDATE"; 
	
	ESP_LOGI(TAG_Function, "Checking and update data from file and cloud...");
	
	check_current_settings();
	
	check_sample_interval();
	
	check_pressure_thresholds();
	
	check_wifi_nets(false);
	
		
	if (!Cloud_update_monitor_presence()) {
		ESP_LOGE(TAG_Function, "Could not upload his presence!");
	}
	
	ESP_LOGI(TAG_Function, "Done!");
	
}



void store_variables_data(){
	ESP_LOGI("Store variables","Storing variables data in file");
	size_t file_size = 0;
	file_size = (FileM_get_variables_file_size())/sizeof(VariablesData_t);
	printf("variables_data_buffer_index: %u\n", (uint)variables_data_buffer_index);
	if (variables_data_buffer_index > 0 && file_size < 748000){
		FileM_store_variables_data(variables_data_buffer, variables_data_buffer_index);
		variables_data_buffer_index = 0;
		variables_file = true;
	}
	file_size = (FileM_get_variables_file_size())/sizeof(VariablesData_t);
	printf("Variables file size: %u\n", (uint)file_size);
}

void store_door_data(){
	ESP_LOGI("Store door","Storing door data in file");
	size_t file_size = 0;
	file_size = (FileM_get_door_file_size())/sizeof(doorState_t);
	printf("door_data_buffer_index: %u\n", (uint)door_data_buffer_index);
	if (door_data_buffer_index > 0 && file_size < 300){
		FileM_store_door_data(door_data_buffer, door_data_buffer_index);
		door_data_buffer_index = 0;
		door_file = true;
	}
	file_size = (FileM_get_door_file_size())/sizeof(doorState_t);
	printf("Door file size: %u\n", (uint)file_size);
}

void store_weight_data(){
	ESP_LOGI("Store weight","Storing weight data in file");
	size_t file_size = 0;
	file_size = (FileM_get_weight_file_size())/sizeof(weightData_t);
	if (weight_data_buffer_index > 0 && file_size < 150){
		FileM_store_weight_data(weight_data_buffer, weight_data_buffer_index);
		weight_data_buffer_index = 0;
		weight_file = true;
	}
	printf("Weight file size: %u\n", (uint)file_size);
}

void store_motorpump_data(){
	ESP_LOGI("Store motorpump","Storing motorpump data in file");
	size_t file_size = 0;
	file_size = (FileM_get_motorpump_file_size())/sizeof(motorpumpState_t);
	if (motorpump_data_buffer_index > 0 && file_size < 300){
		FileM_store_motorpump_data(motorpump_data_buffer, motorpump_data_buffer_index);
		motorpump_data_buffer_index = 0;
		motorpump_file = true;
	}	
	printf("Motorpump file size: %u\n", (uint)file_size);
}

void store_data(){
	
	store_door_data();
	
	store_weight_data();

	store_motorpump_data();
	
	store_variables_data();
	
}

