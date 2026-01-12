/*
 * RTC_Manager.c
 *
 *  Created on: 27/12/2025
 *      Author: Lotfi Dalal
 */

#include "RTC_Manager.h"
#include "esp_err.h"
#include "esp_system.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "sdkconfig.h"




static const char *TAG_NTP = "RTCM";



void RTC_main_function(void)
{
    
    RTCM_initialize_sntp();
    RTCM_obtener_hora_actual();
    ESP_LOGI(TAG_NTP, "Configuracion de hora terminada!");

}

void RTCM_init(){
	RTCM_initialize_sntp();
	if(RTCM_sync_time() != ESP_OK){
		ESP_LOGI("RTCM", "Error in the time sync! Resetting system...");
		vTaskDelay(pdMS_TO_TICKS(1000));
		esp_restart();
	}
}

esp_err_t RTCM_initialize_sntp(void) {
    ESP_LOGI(TAG_NTP, "Initializing SNTP");
    
    // 1. Configurar el modo y el servidor
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    
    // 2. Inicializar el servicio
    esp_sntp_init();
    return ESP_OK;
}

esp_err_t RTCM_sync_time(){
    // 3. Esperar a que el tiempo se sincronice
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG_NTP, "Esperando respuesta de NTP... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    if (retry == retry_count) {
        ESP_LOGE(TAG_NTP, "No se pudo sincronizar el tiempo.");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}


void RTCM_obtener_hora_actual(void) {
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






