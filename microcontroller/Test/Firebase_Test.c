
#include <string.h>
#include <stdio.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "nvs_flash.h"
#include "esp_netif.h"

// Time sync (SNTP)
#include <time.h>
#include <sys/time.h>
#include "esp_sntp.h"

// HTTPS
#include "esp_http_client.h"
#include "esp_tls.h"

//
#define WIFI_SSID      "Familia Vargas ETB 2.4G"
#define WIFI_PASS      "LMSD167294385"
//#define WIFI_SSID      "moto g52 Lotfi"
//#define WIFI_PASS      "98100652ldv"

// Símbolos generados automáticamente por el compilador para el archivo embebido
extern const uint8_t root_ca_pem_start[] asm("_binary_root_ca_pem_start");
extern const uint8_t root_ca_pem_end[]   asm("_binary_root_ca_pem_end");

static const char *TAG_WiFi = "wifi_sta";
static const char *TAG_NTP = "NTP_EXAMPLE";
static const char *TAG_HTTPS = "HTTPS_CLIENT";

static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048
#define FIREBASE_URL	"https://iot-esp32-test-c3fa7-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH	"?auth=larukypnRSv0gKjLw324YgQHvdgZo5ydC63dOlEf"

// Evento: manejo de conexión / desconexión / IP
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void wifi_init_sta(void);

void initialize_sntp(void);
void obtener_hora_actual(void);

esp_err_t _http_event_handler(esp_http_client_event_t *evt);
void firebase_get_data(const char* node_path);
void firebase_put_data(const char* node_path, const char* json_string);
void firebase_post_log(const char* node_path, const char* json_string);
void firebase_patch_data(const char* node_path, const char* json_string);
void firebase_delete_node(const char* node_path);


void app_main(void)
{
    wifi_event_group = xEventGroupCreate();
    wifi_init_sta();

    // Esperar conexión
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT,
                        pdFALSE, pdFALSE, portMAX_DELAY);

    ESP_LOGI(TAG_WiFi, "Wifi OK, ya puedes hacer peticiones.");
    
    initialize_sntp();
    obtener_hora_actual();
    ESP_LOGI(TAG_NTP, "Configuracion de hora terminada!");
    
//    const char *post_data = "{\"sensor_temp\": 35.5, \"estado\": \"conectado\"}";
    const char *fireb_node = "historial.json";
    firebase_get_data(fireb_node);
    
    while(true){
		ESP_LOGI("Loop", "Conectado!");
		vTaskDelay(pdMS_TO_TICKS(2000));
	}
}


// Evento: manejo de conexión / desconexión / IP
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        ESP_LOGI(TAG_WiFi, "Intentando conectar...");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG_WiFi, "Desconectado. Reintentando...");
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG_WiFi, "Conectado! IP asignada: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}


void wifi_init_sta(void)
{
    // 1. Inicializar NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // 2. Inicializar stack de red
    esp_netif_init();

    // 3. Manejo de eventos
    esp_event_loop_create_default();

    // Crear interfaz WiFi STA
    esp_netif_create_default_wifi_sta();

    // 4. Config WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // Registrar callback
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

    // Configurar modo estación
    esp_wifi_set_mode(WIFI_MODE_STA);

    // Configuración del SSID y contraseña
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    strcpy((char *)wifi_config.sta.ssid, WIFI_SSID);
    strcpy((char *)wifi_config.sta.password, WIFI_PASS);

    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);

    // 5. Iniciar WiFi
    esp_wifi_start();
}


void initialize_sntp(void) {
    ESP_LOGI(TAG_NTP, "Inicializando SNTP");
    
    // 1. Configurar el modo y el servidor
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    
    // 2. Inicializar el servicio
    esp_sntp_init();

    // 3. Esperar a que el tiempo se sincronice
    int retry = 0;
    const int retry_count = 15;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG_NTP, "Esperando respuesta de NTP... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    if (retry == retry_count) {
        ESP_LOGE(TAG_NTP, "No se pudo sincronizar el tiempo.");
    }
}


void obtener_hora_actual(void) {
    time_t now;
    struct tm timeinfo;
    
    time(&now); // Obtiene el tiempo actual del sistema (Unix timestamp)

    // Configurar zona horaria (Ejemplo: México/Bogotá -> -5 horas)
    // El formato es: Nombre-del-estándar,desplazamiento,nombre-de-verano...
    setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1);
    tzset();

    localtime_r(&now, &timeinfo);
    
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG_NTP, "La fecha/hora actual es: %s", strftime_buf);
}

// Función para manejar los eventos de la petición (opcional pero recomendada)
esp_err_t _http_event_handler(esp_http_client_event_t *evt) {

    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGI(TAG_HTTPS, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG_HTTPS, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG_HTTPS, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG_HTTPS, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG_HTTPS, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            printf("Respuesta: %.*s\n", evt->data_len, (char*)evt->data);
            // Clean the buffer in case of a new request
            if (output_len == 0 && evt->user_data) {
                // we are just starting to copy the output data into the use
                memset(evt->user_data, 0, MAX_HTTP_OUTPUT_BUFFER);
            }
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                int copy_len = 0;
                if (evt->user_data) {
                    // The last byte in evt->user_data is kept for the NULL character in case of out-of-bound access.
                    copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                    if (copy_len) {
                        memcpy(evt->user_data + output_len, evt->data, copy_len);
                    }
                } else {
                    int content_len = esp_http_client_get_content_length(evt->client);
                    if (output_buffer == NULL) {
                        // We initialize output_buffer with 0 because it is used by strlen() and similar functions therefore should be null terminated.
                        output_buffer = (char *) calloc(content_len + 1, sizeof(char));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG_HTTPS, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    copy_len = MIN(evt->data_len, (content_len - output_len));
                    if (copy_len) {
                        memcpy(output_buffer + output_len, evt->data, copy_len);
                    }
                }
                output_len += copy_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG_HTTPS, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
#if CONFIG_EXAMPLE_ENABLE_RESPONSE_BUFFER_DUMP
                ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
#endif
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG_HTTPS, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG_HTTPS, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG_HTTPS, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGI(TAG_HTTPS, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(evt->client, "From", "user@example.com");
            esp_http_client_set_header(evt->client, "Accept", "text/html");
            esp_http_client_set_redirection(evt->client);
            break;
        default: 
        break;
    }
    return ESP_OK;
}


void firebase_get_data(const char* node_path) {
    const char *fireb_url = FIREBASE_URL;
    const char *fireb_auth = FIREBASE_AUTH;
    uint16_t sizeFullString = strlen(fireb_url) + strlen(fireb_auth) + strlen(node_path) + 1;
	char full_url[sizeFullString];
	snprintf(full_url, sizeof(full_url), "%s%s%s", fireb_url, node_path, fireb_auth);
    esp_http_client_config_t config = {
        .url = full_url,
        .event_handler = _http_event_handler,
        .cert_pem = (const char *)root_ca_pem_start,
        .method = HTTP_METHOD_GET,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}


void firebase_put_data(const char* node_path, const char* json_string) {
    const char *fireb_url = FIREBASE_URL;
    const char *fireb_auth = FIREBASE_AUTH;
    uint16_t sizeFullString = strlen(fireb_url) + strlen(fireb_auth) + strlen(node_path) + 1;
	char full_url[sizeFullString];
	snprintf(full_url, sizeof(full_url), "%s%s%s", fireb_url, node_path, fireb_auth);
    esp_http_client_config_t config = {
        .url = full_url,
        .event_handler = _http_event_handler,
        .cert_pem = (const char *)root_ca_pem_start,
        .method = HTTP_METHOD_PUT,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_string, strlen(json_string));
    
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}


void firebase_post_log(const char* node_path, const char* json_string) {
    const char *fireb_url = FIREBASE_URL;
    const char *fireb_auth = FIREBASE_AUTH;
    uint16_t sizeFullString = strlen(fireb_url) + strlen(fireb_auth) + strlen(node_path) + 1;
	char full_url[sizeFullString];
	snprintf(full_url, sizeof(full_url), "%s%s%s", fireb_url, node_path, fireb_auth);
    esp_http_client_config_t config = {
        .url = full_url,
        .event_handler = _http_event_handler,
        .cert_pem = (const char *)root_ca_pem_start,
        .method = HTTP_METHOD_POST, // Generará un ID único en Firebase
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_string, strlen(json_string));
    
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}


void firebase_patch_data(const char* node_path, const char* json_string) {
    const char *fireb_url = FIREBASE_URL;
    const char *fireb_auth = FIREBASE_AUTH;
    uint16_t sizeFullString = strlen(fireb_url) + strlen(fireb_auth) + strlen(node_path) + 1;
	char full_url[sizeFullString];
	snprintf(full_url, sizeof(full_url), "%s%s%s", fireb_url, node_path, fireb_auth);
    esp_http_client_config_t config = {
        .url = full_url,
        .event_handler = _http_event_handler,
        .cert_pem = (const char *)root_ca_pem_start,
        .method = HTTP_METHOD_PATCH, // Indicamos actualización parcial
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    // Al igual que POST/PUT, necesitamos enviar encabezados y datos
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_string, strlen(json_string));

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG_HTTPS, "PATCH exitoso. Status = %d", esp_http_client_get_status_code(client));
    } else {
        ESP_LOGE(TAG_HTTPS, "Error en PATCH: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

void firebase_delete_node(const char* node_path) {
    const char *fireb_url = FIREBASE_URL;
    const char *fireb_auth = FIREBASE_AUTH;
    uint16_t sizeFullString = strlen(fireb_url) + strlen(fireb_auth) + strlen(node_path) + 1;
	char full_url[sizeFullString];
	snprintf(full_url, sizeof(full_url), "%s%s%s", fireb_url, node_path, fireb_auth);
    esp_http_client_config_t config = {
        .url = full_url,
        .event_handler = _http_event_handler,
        .cert_pem = (const char *)root_ca_pem_start,
        .method = HTTP_METHOD_DELETE,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}


