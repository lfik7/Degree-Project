/*
 * Firebase_Manager.c
 *
 *  Created on: 26/12/2025
 *      Author: Lotfi Dalal
 */

#include "Cloud_Manager.h"
#include "cJSON.h"
#include "esp_http_client.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include <stdio.h>
#include <string.h>
#include <time.h>





// Símbolos generados automáticamente por el compilador para el archivo embebido
extern const uint8_t root_ca_pem_start[] asm("_binary_root_ca_pem_start");
extern const uint8_t root_ca_pem_end[]   asm("_binary_root_ca_pem_end");

static const char *TAG_HTTPS = "HTTPS_CLIENT", *TAG_CLOUDM = "CLOUDM";


static const char* FIREBASE_URL = "https://proyecto-receptaculo-default-rtdb.firebaseio.com/";
static const char* FIREBASE_AUTH = "?auth=oDQJsD2ff832SPDK0GlDig2ujRC7nMTvKkDYhKKn";
static const char* full_url_template = "%s%s.json%s"; 
static char full_url[512];

static char cloud_answer[2048];

static char post_data_log[512];
static char patch_data[512];
static char put_data[512];
static esp_err_t _http_event_handler(esp_http_client_event_t *evt);

static esp_http_client_handle_t cloud_client = NULL;
static esp_err_t Cloud_get_data(const char* node_path);
static esp_err_t Cloud_get_data_careful(const char* node_path);
static esp_err_t Cloud_put_data(const char* node_path, const char* json_string);
static esp_err_t Cloud_put_data_careful(const char* node_path, const char* json_string);
static esp_err_t Cloud_post_log(const char* node_path, const char* json_string);
static esp_err_t Cloud_post_log_careful(const char* node_path, const char* json_string);
static esp_err_t Cloud_patch_data(const char* node_path, const char* json_string);
static esp_err_t Cloud_patch_data_careful(const char* node_path, const char* json_string);
//static esp_err_t Cloud_delete_node(const char* node_path);

void Cloud_init(){
    
	snprintf(full_url, sizeof(full_url), "%s.json%s", FIREBASE_URL, FIREBASE_AUTH);
	esp_http_client_config_t cloud_config ={
		.url = full_url,
        .event_handler = _http_event_handler,
        .cert_pem = (const char *)root_ca_pem_start,
        .keep_alive_enable = true,
        .keep_alive_idle = 120,		// Tiempo en segundos antes de enviar un paquete de prueba
        .keep_alive_interval = 5,	// Intervalo entre paquetes de prueba
     	.keep_alive_count = 3,		// Cuántas pruebas fallidas antes de cerrar
     	.user_data = cloud_answer,
     	.timeout_ms = 10000,
	};
	
	cloud_client = esp_http_client_init(&cloud_config);
}

void Cloud_release(){
	esp_http_client_cleanup(cloud_client);
}

// Función para manejar los eventos de la petición (opcional pero recomendada)
static esp_err_t _http_event_handler(esp_http_client_event_t *evt) {

//    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
//            ESP_LOGI(TAG_HTTPS, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
//            ESP_LOGI(TAG_HTTPS, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
//            ESP_LOGI(TAG_HTTPS, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
//            ESP_LOGI(TAG_HTTPS, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
//            ESP_LOGI(TAG_HTTPS, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
//            printf("Respuesta: %.*s\n", evt->data_len, (char*)evt->data);
			if (output_len + evt->data_len < 512) { // 1024 o el tamaño de tu cloud_answer
                memcpy(cloud_answer + output_len, evt->data, evt->data_len);
                output_len += evt->data_len;
                cloud_answer[output_len] = '\0'; // Aseguramos que siempre sea un string válido
            }
//            // Clean the buffer in case of a new request
//            if (output_len == 0 && evt->user_data) {
//                // we are just starting to copy the output data into the use
//                memset(evt->user_data, 0, MAX_HTTP_OUTPUT_BUFFER);
//            }
//            /*
//             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
//             *  However, event handler can also be used in case chunked encoding is used.
//             */
//            if (!esp_http_client_is_chunked_response(evt->client)) {
//                // If user_data buffer is configured, copy the response into the buffer
//                int copy_len = 0;
//                if (evt->user_data) {
//                    // The last byte in evt->user_data is kept for the NULL character in case of out-of-bound access.
//                    copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
//                    if (copy_len) {
//                        memcpy(evt->user_data + output_len, evt->data, copy_len);
//                    }
//                } else {
//                    int content_len = esp_http_client_get_content_length(evt->client);
//                    if (output_buffer == NULL) {
//                        // We initialize output_buffer with 0 because it is used by strlen() and similar functions therefore should be null terminated.
//                        output_buffer = (char *) calloc(content_len + 1, sizeof(char));
//                        output_len = 0;
//                        if (output_buffer == NULL) {
//                            ESP_LOGE(TAG_HTTPS, "Failed to allocate memory for output buffer");
//                            return ESP_FAIL;
//                        }
//                    }
//                    copy_len = MIN(evt->data_len, (content_len - output_len));
//                    if (copy_len) {
//                        memcpy(output_buffer + output_len, evt->data, copy_len);
//                    }
//                }
//                output_len += copy_len;
//            }

            break;
        case HTTP_EVENT_ON_FINISH:
//            ESP_LOGI(TAG_HTTPS, "HTTP_EVENT_ON_FINISH");
//            if (output_buffer != NULL) {
//#if CONFIG_EXAMPLE_ENABLE_RESPONSE_BUFFER_DUMP
//                ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
//#endif
//                free(output_buffer);
//                output_buffer = NULL;
//            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
//            ESP_LOGI(TAG_HTTPS, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, 
    											&mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG_HTTPS, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG_HTTPS, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
//            if (output_buffer != NULL) {
//                free(output_buffer);
//                output_buffer = NULL;
//            }
//            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
//            ESP_LOGI(TAG_HTTPS, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(evt->client, "From", "user@example.com");
            esp_http_client_set_header(evt->client, "Accept", "text/html");
            esp_http_client_set_redirection(evt->client);
            break;
        default: 
        break;
    }
    return ESP_OK;
}


esp_err_t Cloud_get_data(const char* node_path) {
	if (cloud_client == NULL) return ESP_ERR_INVALID_STATE;
	
	memset(cloud_answer, 0, sizeof(cloud_answer));
	
	memset(full_url, 0, sizeof(full_url));	
	snprintf(full_url, sizeof(full_url), full_url_template, FIREBASE_URL, node_path, FIREBASE_AUTH);

	esp_http_client_set_post_field(cloud_client,NULL, 0);
    esp_http_client_set_url(cloud_client, full_url);
    esp_http_client_set_header(cloud_client, "Accept", "application/json");
    esp_http_client_set_method(cloud_client, HTTP_METHOD_GET);
    return esp_http_client_perform(cloud_client);
}


esp_err_t Cloud_get_data_careful(const char* node_path){
	
	if (Cloud_get_data(node_path) != ESP_OK){
		esp_http_client_cleanup(cloud_client);
		vTaskDelay(pdMS_TO_TICKS(1000));
		Cloud_init();
		if (Cloud_get_data(node_path) != ESP_OK) {
			ESP_LOGE(TAG_HTTPS,"No se pudo realizar el GET!");
			return ESP_FAIL;
		}
	}
	return ESP_OK;
}


esp_err_t Cloud_put_data(const char* node_path, const char* json_string) {
	if (cloud_client == NULL) return ESP_ERR_INVALID_STATE;
	
	memset(cloud_answer, 0, sizeof(cloud_answer));
	
	memset(full_url, 0, sizeof(full_url));
	snprintf(full_url, sizeof(full_url), full_url_template, FIREBASE_URL, node_path, FIREBASE_AUTH);
	
	esp_http_client_set_url(cloud_client, full_url);
    esp_http_client_set_header(cloud_client, "Content-Type", "application/json");
    esp_http_client_set_post_field(cloud_client, json_string, strlen(json_string));
    esp_http_client_set_method(cloud_client, HTTP_METHOD_PUT);
    
    return esp_http_client_perform(cloud_client);
}

static esp_err_t Cloud_put_data_careful(const char* node_path, const char* json_string) {
	if (Cloud_put_data(node_path, json_string) != ESP_OK){
		esp_http_client_cleanup(cloud_client);
		vTaskDelay(pdMS_TO_TICKS(1000));
		Cloud_init();
		if (Cloud_put_data(node_path, json_string) != ESP_OK) {
			ESP_LOGE(TAG_HTTPS,"No se pudo realizar el POST!");
			return ESP_FAIL;
		}
	}
	return ESP_OK;	
}


esp_err_t Cloud_post_log(const char* node_path, const char* json_string) {
	if (cloud_client == NULL) return ESP_ERR_INVALID_STATE;
	
	memset(cloud_answer, 0, sizeof(cloud_answer));
	
	memset(full_url, 0, sizeof(full_url));
	snprintf(full_url, sizeof(full_url), full_url_template, FIREBASE_URL, node_path, FIREBASE_AUTH);
	
	esp_http_client_set_url(cloud_client, full_url);
    esp_http_client_set_header(cloud_client, "Content-Type", "application/json");
    esp_http_client_set_post_field(cloud_client, json_string, strlen(json_string));
    esp_http_client_set_method(cloud_client, HTTP_METHOD_POST);
    
    return esp_http_client_perform(cloud_client);
}

static esp_err_t Cloud_post_log_careful(const char* node_path, const char* json_string) {
	
	if (Cloud_post_log(node_path, json_string) != ESP_OK){
		esp_http_client_cleanup(cloud_client);
		vTaskDelay(pdMS_TO_TICKS(1000));
		Cloud_init();
		if (Cloud_post_log(node_path, json_string) != ESP_OK) {
			ESP_LOGE(TAG_HTTPS,"No se pudo realizar el POST!");
			return ESP_FAIL;
		}
	}
	return ESP_OK;
}


esp_err_t Cloud_patch_data(const char* node_path, const char* json_string) {
	if (cloud_client == NULL) return ESP_ERR_INVALID_STATE;
	
	memset(cloud_answer, 0, sizeof(cloud_answer));
	
	memset(full_url, 0, sizeof(full_url));
	snprintf(full_url, sizeof(full_url), full_url_template, FIREBASE_URL, node_path, FIREBASE_AUTH);
    
    // Al igual que POST/PUT, necesitamos enviar encabezados y datos
	esp_http_client_set_url(cloud_client, full_url);
    esp_http_client_set_header(cloud_client, "Content-Type", "application/json");
    esp_http_client_set_post_field(cloud_client, json_string, strlen(json_string));
    esp_http_client_set_method(cloud_client, HTTP_METHOD_PATCH);
    
    esp_err_t err = esp_http_client_perform(cloud_client);

    if (err != ESP_OK) {
        ESP_LOGE(TAG_HTTPS, "Error en PATCH: %s", esp_err_to_name(err));
    }

    return err;
}

static esp_err_t Cloud_patch_data_careful(const char* node_path, const char* json_string){
	
	if (Cloud_patch_data(node_path, json_string) != ESP_OK){
		esp_http_client_cleanup(cloud_client);
		vTaskDelay(pdMS_TO_TICKS(1000));
		Cloud_init();
		if (Cloud_patch_data(node_path, json_string) != ESP_OK) {
			ESP_LOGE(TAG_HTTPS,"No se pudo realizar el PATCH!");
			return ESP_FAIL;
		}
	}
	
	return ESP_OK;
	
}

//esp_err_t Cloud_delete_node(const char* node_path) {
//	if (cloud_client == NULL) return ESP_ERR_INVALID_STATE;
//	
//	memset(cloud_answer, 0, sizeof(cloud_answer));
//	
//	memset(full_url, 0, sizeof(full_url));
//	snprintf(full_url, sizeof(full_url), full_url_template, FIREBASE_URL, node_path, FIREBASE_AUTH);
//    
//
//	esp_http_client_set_post_field(cloud_client,NULL, 0);
//    esp_http_client_set_url(cloud_client, full_url);
//    esp_http_client_set_method(cloud_client, HTTP_METHOD_DELETE);
//    
//    return esp_http_client_perform(cloud_client);
//}


bool Cloud_get_current_settigns(current_settigns_t* settings){
    const char* node_path = "Current_Settings";

	if(Cloud_get_data_careful(node_path) != ESP_OK) return false; 
	
//	printf("Buffer cloud_answer: %s\n", cloud_answer);
    
    cJSON *json = cJSON_Parse(cloud_answer);
    if (json == NULL) {
        ESP_LOGE(TAG_CLOUDM,"Error al parsear el JSON\n");
        return false;
    }
    
    cJSON *items = cJSON_GetObjectItem(json, "WID"); // Red WiFi
    if (cJSON_IsString(items)){
		snprintf(settings->WiFi_SSID, sizeof(settings->WiFi_SSID), "%s",items->valuestring);
	}
    items = cJSON_GetObjectItem(json, "SI"); // Sample interval
    if (cJSON_IsNumber(items)){
		settings->SAMP_INT = items->valueint;
	}
    items = cJSON_GetObjectItem(json, "Do"); // Door
    if (cJSON_IsBool(items)){
		if (cJSON_IsTrue(items)){
			settings->Door = true;
		} else {
			settings->Door = false;
		}
	}
    items = cJSON_GetObjectItem(json, "Mo"); // Motoomba
    if (cJSON_IsBool(items)){
		if (cJSON_IsTrue(items)){
			settings->Motorpump = true;
		} else {
			settings->Motorpump = false;
		}
	}
    items = cJSON_GetObjectItem(json, "We"); // Weight
    if (cJSON_IsNumber(items)){
		settings->Weight = items->valuedouble;
	}
    items = cJSON_GetObjectItem(json, "PTmn"); // Weight
    if (cJSON_IsNumber(items)){
		settings->pressure_thresholds.min = items->valuedouble;
	}
    items = cJSON_GetObjectItem(json, "PTmx"); // Weight
    if (cJSON_IsNumber(items)){
		settings->pressure_thresholds.max = items->valuedouble;
	}
	
	cJSON_Delete(json); // Borra todo de una vez
	
	return true;
}


bool Cloud_get_wifi_nets(WiFi_SSID_PSSW_t* wifi_nets, uint8_t quantity_positions){
    const char* node_path = "WiFi_Nets/Nets";


	if(Cloud_get_data_careful(node_path) != ESP_OK) return false; 
	
	for(uint8_t iter = 0; iter < quantity_positions; iter++){
		memset(wifi_nets[iter].WiFi_SSID, 0, sizeof(wifi_nets[0].WiFi_SSID));
		memset(wifi_nets[iter].WiFi_PSSW, 0, sizeof(wifi_nets[0].WiFi_PSSW));
	}
	
	cJSON *root = cJSON_Parse(cloud_answer);
    if (!root) return false;

    uint8_t found_nets = 0;
    cJSON *node_net = NULL;
	
	// Iteramos sobre cada hijo del objeto principal (N1, N2...)
    cJSON_ArrayForEach(node_net, root) {
        if (found_nets >= quantity_positions) { 
			// Evitar desborde del arreglo
        	ESP_LOGI("CLOUD_WIFI_NETS","There are more than %u nets!", quantity_positions);
			break;
		} 

        // Extraemos ID
        cJSON *id_item = cJSON_GetObjectItem(node_net, "ID");
        if (cJSON_IsString(id_item)) {
            strncpy(wifi_nets[found_nets].WiFi_SSID, id_item->valuestring, sizeof(wifi_nets[found_nets].WiFi_SSID));
        }

        // Extraemos PS
        cJSON *ps_item = cJSON_GetObjectItem(node_net, "PS");
        if (cJSON_IsString(ps_item)) {
            strncpy(wifi_nets[found_nets].WiFi_PSSW, ps_item->valuestring, sizeof(wifi_nets[found_nets].WiFi_PSSW));
        }

        found_nets++;
    }

    cJSON_Delete(root);
    
    // Imprimir para verificar
//    for(int i = 0; i < found_nets; i++) {
//        printf("Red %d: SSID=%s, Pass=%s\n", i+1, wifi_nets[i].WiFi_SSID, wifi_nets[i].WiFi_PSSW);
//    }
	
	return true;
}

bool Cloud_get_wifi_nets_edits(bool * edited){
    const char* node_path = "WiFi_Nets/Edits/APP";

	if(Cloud_get_data_careful(node_path) != ESP_OK) return false; 
	
	if (strcmp("true\0", cloud_answer) == 0){
		*edited = true;
	} else if (strcmp("false\0", cloud_answer) == 0) {
		*edited = false;
	} else {
		ESP_LOGW("CLOUD_WIFI_NETS_EDITS","The answer is not 'true' nether 'false'!");
		return false;
	}
	
	
	return true;
}


bool Cloud_upload_wifi_nets(WiFi_SSID_PSSW_t* wifi_nets, uint8_t quantity_positions) {
    const char* node_path = "WiFi_Nets/Nets";
	
//	memset(put_data, 0, sizeof(put_data));

	int current_buff_len = 1;
	put_data[0] = '{';
	put_data[1] = '\0';
	bool first_field = true;
	int writen = 0;
	uint8_t iter;
	
	for(iter = 0; iter < quantity_positions; iter ++){
//		if (wifi_nets[iter].WiFi_SSID[0] == '\0') break;
		const char* separator = first_field ? "": ",";
		writen = snprintf(put_data + current_buff_len, sizeof(put_data) - current_buff_len, 
							"%s\"N%u\":{\"ID\":\"%s\",\"PS\":\"%s\"}", separator, iter + 1, 
							wifi_nets[iter].WiFi_SSID, wifi_nets[iter].WiFi_PSSW);
		
		current_buff_len += writen;
		first_field = false;
	}
	
	snprintf(put_data + current_buff_len, sizeof(put_data) - current_buff_len, "}");	
	
	if (Cloud_put_data_careful( node_path, put_data) != ESP_OK) return false;

	return true;
}


bool Cloud_update_wifi_nets_edits(bool edited, char* app_esp){
    const char* node_path = "WiFi_Nets/Edits";
    
    char json_string[50];
    
    snprintf(json_string, sizeof(json_string), "{\"%s\":%s}", app_esp, edited ? "true" : "false");
    
    if (Cloud_patch_data_careful(node_path, json_string) != ESP_OK){
		return false;
	}
	
	return true;
}


bool Cloud_get_sample_interval(long long* sample_inter){
	const char* node_path = "Sample_Inter";
	if(Cloud_get_data_careful(node_path) != ESP_OK) return false;
	
//	printf("cloud_answer: %s\n", cloud_answer);
	
	sscanf(cloud_answer, "%lld", sample_inter);
	 
	return true;
}



bool Cloud_post_sensors_data(VariablesData_t* sensors_data){
	struct tm timeinfo;
	localtime_r(&sensors_data->timestamp, &timeinfo);
	char node_path[50];
	
	
	snprintf(node_path, sizeof(node_path), "Data_log/%d/%d/%d", timeinfo.tm_year+1900, timeinfo.tm_mon+1, 
			timeinfo.tm_mday);
	
	
	snprintf(post_data_log, sizeof(post_data_log), 
			"{\"t\":%lld,\"T\":%.4f,\"H\":%.1f,\"P\":%.1f,\"C\":%.1f,\"O\":%.1f,\"N\":%.1f}",
			sensors_data->timestamp, sensors_data->temp, sensors_data->humidity, sensors_data->pressure, 
			sensors_data->gas_co2, sensors_data->gas_oh, sensors_data->gas_nit);

	if (Cloud_post_log_careful(node_path, post_data_log) != ESP_OK) return false;

	return true;
}




bool Cloud_post_door_state(doorState_t* door_data ){
	struct tm timeinfo;
	localtime_r(&door_data->timestamp, &timeinfo);
	char node_path[50];
	
	snprintf(node_path, sizeof(node_path), "Data_events/Door/%d/%d/%d", timeinfo.tm_year+1900, timeinfo.tm_mon+1, 
				timeinfo.tm_mday);
	
//	char post_data_log[300];
	snprintf(post_data_log, sizeof(post_data_log), "{\"t\":%lld,\"st\":%s}",
			door_data->timestamp, door_data->state?"true":"false");
	
	if (Cloud_post_log_careful(node_path, post_data_log) != ESP_OK) return false;

	
	return true;
}



bool Cloud_post_motorpump_state(bool state, long long timestamp){
	struct tm timeinfo;
	localtime_r(&timestamp, &timeinfo);
	char node_path[50];
	
	snprintf(node_path, sizeof(node_path), "Data_events/MoPu/%d/%d/%d", timeinfo.tm_year+1900, timeinfo.tm_mon+1, 
				timeinfo.tm_mday);
	
//	char post_data_log[300];
	snprintf(post_data_log, sizeof(post_data_log), "{\"t\":%lld,\"st\":%s}",
			timestamp, state?"true":"false");
	
	if (Cloud_post_log_careful(node_path, post_data_log) != ESP_OK) return false;

	
	return true;
}


bool cloud_post_food_weight(float weight, long long timestamp){
	struct tm timeinfo;
	localtime_r(&timestamp, &timeinfo);
	char node_path[50];
	
	snprintf(node_path, sizeof(node_path), "Data_events/Weig/%d/%d/%d", timeinfo.tm_year+1900, 
				timeinfo.tm_mon+1, timeinfo.tm_mday);
	
//	char post_data_log[300];
	snprintf(post_data_log, sizeof(post_data_log), "{\"t\":%lld,\"vl\":%.1f}",
			timestamp, weight);
	
	if (Cloud_post_log_careful(node_path, post_data_log) != ESP_OK) return false;

	
	return true;
}



bool Cloud_update_current_settings(current_settigns_t* settings, const char* fields){
	
	const char *node_path = "Current_Settings";
	
	int current_buff_len = 1;
	patch_data[0] = '{';
	patch_data[1] = '\0';
	
	int quantity_flieds = strlen(fields);
	bool first_field = true;
	int writen = 0;
	
	
	for(uint8_t iter = 0; iter < quantity_flieds; iter ++){
		const char* separator = first_field ? "": ",";
		writen = 0;
		switch (fields[iter]) {
			case 'W':	// WiFi
				writen = snprintf(patch_data + current_buff_len, sizeof(patch_data) - current_buff_len, 
									"%s\"WID\":\"%s\"", separator, settings->WiFi_SSID);
				break;
			case 'S':	// Sample interval
				writen = snprintf(patch_data + current_buff_len, sizeof(patch_data) - current_buff_len, 
									"%s\"SI\":%ld", separator, (long)settings->SAMP_INT);
				break;
			case 'D':	// Door
				writen = snprintf(patch_data + current_buff_len, sizeof(patch_data) - current_buff_len, 
									"%s\"Do\":%s", separator, settings->Door?"true":"false");
				break;
			case 'M':	// Motorpump
				writen = snprintf(patch_data + current_buff_len, sizeof(patch_data) - current_buff_len, 
									"%s\"Mo\":%s", separator, settings->Motorpump?"true":"false");
				break;
			case 'w':	// weight
				writen = snprintf(patch_data + current_buff_len, sizeof(patch_data) - current_buff_len, 
									"%s\"We\":%.1f", separator, settings->Weight);
				break;
			case 'P':
				writen = snprintf(patch_data + current_buff_len, sizeof(patch_data) - current_buff_len, 
									"%s\"PTmn\":%.1f,\"PTmx\":%.1f", separator, settings->pressure_thresholds.min, 
									settings->pressure_thresholds.max);
		}
		if(writen > 0){
			current_buff_len += writen;
			first_field = false;
		}
	}
	
//	current_buff_len = strlen(patch_data);
	snprintf(patch_data + current_buff_len, sizeof(patch_data) - current_buff_len, "}");	
	
	
	if (Cloud_patch_data_careful(node_path, patch_data) != ESP_OK) return false;
	
	return true;
}



bool Cloud_get_pressure_thresholds(pressureThresholds_t* thresholds){
    const char* node_path = "Alarm_thresholds/Pres";

	if(Cloud_get_data_careful(node_path) != ESP_OK) return false;
	
//	printf("Buffer cloud_answer: %s\n", cloud_answer);
    
    cJSON *json = cJSON_Parse(cloud_answer);
    if (json == NULL) {
        ESP_LOGE(TAG_CLOUDM,"Error al parsear el JSON\n");
        return false;
    }
    
    cJSON *items = cJSON_GetObjectItem(json, "mn"); // Red WiFi
    if (cJSON_IsNumber(items)){
		thresholds->min = items->valuedouble;
	}
    items = cJSON_GetObjectItem(json, "mx"); // Sample interval
    if (cJSON_IsNumber(items)){
		thresholds->max = items->valuedouble;
	}
	
	cJSON_Delete(json); // Borra todo de una vez
	
	return true;
}


bool Cloud_update_monitor_presence(){
	
    const char* node_path = "Monitor_presence";
    
    time_t now;
    
    time(&now); // Obtiene el tiempo actual del sistema (Unix timestamp)
    
    char json_string[50];
    
    snprintf(json_string, sizeof(json_string), "{\"t\":%lld}", (long long)now);
    
    if (Cloud_patch_data_careful(node_path, json_string) != ESP_OK){
		return false;
	}
	
	return true;
}






