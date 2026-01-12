/*
 * File_Manager.c
 *
 *  Created on: 26/12/2025
 *      Author: Lotfi Dalal
 */


#include "File_Manager.h"
#include "Globals.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include <stdio.h>



static const char* TAG_FileM = "FileM";

static FILE * variables_file = NULL;
static FILE * door_file = NULL;
static FILE * weight_file = NULL;
static FILE * motorpump_file = NULL;


current_settigns_t Cur_Set = {
	"Familia Vargas ETB 2.4G", 60, true, false, 0.0, .pressure_thresholds.min = 10, .pressure_thresholds.max = 40
//	"WiFi Net Test", 60, true, false, 0.0
};
//
//WiFi_SSID_PSSW_t W_ID_PSW[3] = {
//	{"moto g52 Lotfi","98100652ldv"},
//	{"WiFi Net Test", "Cualquiera123"},
//	{"Familia Vargas ETB 2.4G", "LMSD167294385"}
//};

//struct SensorData  mi_data;


static esp_err_t FileM_mount_file_system();
static size_t FileM_get_file_size(const char* path);
static esp_err_t FileM_Remove_File(const char* path);


static esp_err_t FileM_mount_file_system(){
    // Configuración y montaje
    ESP_LOGI(TAG_FileM,"Mounting file sistem...");
    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/littlefs",
        .partition_label = "storage",
        .format_if_mount_failed = true,
        .dont_mount = false
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_FileM, "Error mounting LittleFS");
    }
	
	return ret;
}


void FileM_init(){
	FileM_mount_file_system();
//	vTaskDelay(pdMS_TO_TICKS(2000));
//	ESP_LOGI("FileM","Guardando archivos csv");
//	FileM_store_new_settings(&Cur_Set);
////	FileM_store_wifi_net(W_ID_PSW, sizeof(W_ID_PSW)/sizeof(W_ID_PSW[0]), true);
//	vTaskDelay(pdMS_TO_TICKS(2000));
}


void FileM_store_new_settings(current_settigns_t* settings){
	FILE* f = fopen("/littlefs/Current_Settings.csv", "w");
    if (f == NULL) {
        ESP_LOGE(TAG_FileM, "Error opening file Current_Settings.csv");
        return;
    }
    fprintf(f, "# WiFi_SSID,SAMP_INT(s),Door,Motorpump,Weight,Press_Thresh_min,Press_Thresh_max\n");
    fprintf(f, "%s,%d,%d,%d,%.1f,%.1f,%.1f\n", settings->WiFi_SSID, settings->SAMP_INT, settings->Door, 
    		settings->Motorpump, settings->Weight, settings->pressure_thresholds.min, 
    		settings->pressure_thresholds.max);
    fclose(f);
}



void FileM_list_files(const char* base_path) {
    DIR *dir = opendir(base_path);
    if (!dir) return;

    struct dirent *openig;
    while ((openig = readdir(dir)) != NULL) {
        char full_path[300];
        snprintf(full_path, sizeof(full_path), "%s/%s", base_path, openig->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                // Es un directorio, entramos recursivamente
                if (strcmp(openig->d_name, ".") != 0 && strcmp(openig->d_name, "..") != 0) {
                    printf("[DIR]  %s\n", full_path);
                    FileM_list_files(full_path);
                }
            } else {
                // Es un archivo, imprimimos su tamaño
                printf("[FILE] %-30s | Tamanio: %ld bytes\n", full_path, st.st_size);
            }
        }
    }
    closedir(dir);
}




void FileM_get_current_settings(current_settigns_t* settigns){
	const char *path = "/littlefs/Current_Settings.csv";
	
//	ESP_LOGI(TAG_FileM,"Cargando archivo %s:\n", path);
    FILE* f = fopen(path, "r");
    if (f == NULL) return;

    char line[128];

    while (fgets(line, sizeof(line), f)) {
        // Ignorar comentarios o líneas vacías
        if (line[0] == '#' || line[0] == '\n') continue;

        // sscanf busca el formato: texto hasta la coma, saltar coma, texto hasta el final
        // %31[^,] lee hasta 31 caracteres que NO sean una coma
        int samp_int, door, motorpump;
        float weight, presTH_min, presTH_max;

        if (sscanf(line, "%31[^,],%d,%d,%d,%f,%f,%f", settigns->WiFi_SSID, &samp_int, &door, &motorpump, 
        	&weight, &presTH_min, &presTH_max) == 5) {
			settigns->SAMP_INT = samp_int;
			settigns->Door = (bool)door;
			settigns->Motorpump = (bool)motorpump;
			settigns->Weight = weight;
			settigns->pressure_thresholds.min = presTH_min;
			settigns->pressure_thresholds.max = presTH_max;
//            printf("\tSSID cargado: %s\n", Cur_set->WiFi_SSID);
//            printf("\tIntervalo de muestreo cargado (s): %d\n", Cur_set->SAMP_INT);
//            printf("\tEstado de la puerta: %s\n", (Cur_set->Door)?"abierta":"cerrada");
//            printf("\tEstado de la motorpump: %s\n", (Cur_set->Motorpump)?"activa":"inactiva");
//            printf("\tPeso del silo (kg): %.2f\n", Cur_set->Weight);
        }
        break;
    }
    
    fclose(f);
	
}



bool FileM_get_wifi_net(const char* wifi_net, WiFi_SSID_PSSW_t* WiFi_ID_PSSW) {
	const char* path = "/littlefs/WiFi_Nets.csv";
//    printf("Cargando archivo %s:\n", path);
    FILE* f = fopen(path, "r");
    if (f == NULL) return false;

    char line[128];
    bool found = false;
//    uint8_t contador_line = 0;

    while (fgets(line, sizeof(line), f)) {
		line[strcspn(line,"\r\n")] = 0;
        // Ignorar comentarios o líneas vacías
        if (line[0] == '#' || strlen(line) == 0) continue;

        // sscanf busca el formato: texto hasta la coma, saltar coma, texto hasta el final
        // %31[^,] lee hasta 31 caracteres que NO sean una coma
		char * ID = strtok(line, ",");
		char * PSW = strtok(NULL, ",");
	    if (ID != NULL && PSW != NULL) {
			if (strcmp(ID, wifi_net) == 0){
				strncpy(WiFi_ID_PSSW->WiFi_SSID, ID, 31);
				strncpy(WiFi_ID_PSSW->WiFi_PSSW, PSW, 31);
				WiFi_ID_PSSW->WiFi_SSID[31] = '\0';
				WiFi_ID_PSSW->WiFi_PSSW[31] = '\0';
				found = true;
				break;
			}
		}
    }
    
    
    fclose(f);
    
	if (!found) ESP_LOGE("Cargar WiFi","No se encontro la opcion deseada");
	
	return found;
}


void FileM_get_all_wifi_nets(WiFi_SSID_PSSW_t* wifi_nets){
	const char* path = "/littlefs/WiFi_Nets.csv";
//    printf("Cargando archivo %s:\n", path);
    FILE* f = fopen(path, "r");
    if (f == NULL) return;

    char line[128];
    uint8_t iter = 0;
    
    for(iter = 0; iter < 5; iter++){
		memset(wifi_nets[iter].WiFi_SSID, 0, sizeof(wifi_nets[0].WiFi_SSID));
		memset(wifi_nets[iter].WiFi_PSSW, 0, sizeof(wifi_nets[0].WiFi_PSSW));
	}
	
	iter = 0;
    while (fgets(line, sizeof(line), f)) {
		line[strcspn(line,"\r\n")] = 0;
        // Ignorar comentarios o líneas vacías
        if (line[0] == '#' || strlen(line) == 0) continue;

        // sscanf busca el formato: texto hasta la coma, saltar coma, texto hasta el final
        // %31[^,] lee hasta 31 caracteres que NO sean una coma
		char * ID = strtok(line, ",");
		char * PSW = strtok(NULL, ",");
	    if (ID != NULL && PSW != NULL) {
			strncpy(wifi_nets[iter].WiFi_SSID, ID, 31);
			strncpy(wifi_nets[iter].WiFi_PSSW, PSW, 31);
			wifi_nets[iter].WiFi_SSID[31] = '\0';
			wifi_nets[iter].WiFi_PSSW[31] = '\0';
//			printf("Red cargada:\n\tID: %s\n\tPassword: %s\n",wifi_nets[iter].WiFi_SSID, wifi_nets[iter].WiFi_PSSW);
			iter ++;
			if (iter == 5) {
				ESP_LOGI(TAG_FileM,"There are more than 5 wifi nets in the file (%s)", path);
				break;
			}
		}
    }
    
    
    fclose(f);
}



void FileM_store_wifi_net(WiFi_SSID_PSSW_t* wifi_nets, uint8_t quantity_positions, bool new_file){
	
	FILE* f = fopen("/littlefs/WiFi_Nets.csv", new_file?"w":"a");
    if (f == NULL) {
        ESP_LOGE(TAG_FileM, "Error al abrir el archivo Current_Settings.csv");
        return;
    }
    if(new_file) fprintf(f, "# SSID,PSSW\n");

    for(uint8_t iter = 0; iter < quantity_positions; iter++){
		if (wifi_nets[iter].WiFi_SSID[0] == '\0') break;
		fprintf(f, "%s,%s\n", wifi_nets[iter].WiFi_SSID, wifi_nets[iter].WiFi_PSSW);
		ESP_LOGI(TAG_FileM, "Adding WiFi net: %s", wifi_nets[iter].WiFi_SSID);
	}

    
    fclose(f);
	
}



size_t FileM_get_file_size(const char* path){
	
	struct stat st;
	if (stat(path, &st) == 0) {
	    if (S_ISDIR(st.st_mode)) {
	        // Es un directorio, entramos recursivamente
	       ESP_LOGE(TAG_FileM,"Error getting the size of the file. Is a folder!");
	       return 0;
	    } else {
	        // Es un archivo, imprimimos su tamaño
	        printf("[FILE] %-30s | Tamanio: %ld bytes\n", path, st.st_size);
	    }
	}
	return st.st_size;
}


void FileM_store_variables_data(VariablesData_t* data, uint8_t data_size){
	FILE* f = fopen("/littlefs/variables_file.bin","ab");
	
	if (f == NULL){
		ESP_LOGE(TAG_FileM, "Error opening variables_file.bin to store!");
	}
	
	fwrite(data, sizeof(VariablesData_t), data_size, f);
	fclose(f);
}

size_t FileM_get_variables_file_size(){
	size_t file_size = 0;
	
	char* file = "/littlefs/variables_file.bin";
	
	file_size = FileM_get_file_size(file);
	
	return file_size;
}

bool FileM_open_variables_data_file(){
	if (variables_file == NULL) { 
		variables_file = fopen("/littlefs/variables_file.bin","rb");
	}
	
	if (variables_file == NULL) {
		ESP_LOGE(TAG_FileM, "Error opening variables_file.bin to store!");
		return false;
	}
	
	return true;
}

bool FileM_get_variables_data(VariablesData_t* data, uint8_t* data_size){
    printf("\n--- Leyendo datos binarios de las variables ---\n");

	if (variables_file == NULL){
		ESP_LOGE(TAG_FileM, "Error getting data from the variables_file! File is closed!");
		return false;
	}
	
    // Leemos mientras haya estructuras completas en el archivo
    size_t data_size_readed = fread(data, sizeof(VariablesData_t), 40, variables_file);
    *data_size = (uint) data_size_readed;
    if (data_size_readed == 40){ 			// If data_size_readed is equal to 40, it means there is more data
    	return false;  
    } 
//    while (fread(&temp_data, sizeof(VariablesData_t), 1, variables_file) == 1) {
//        printf("Registro %d:\n", registro++);
//        printf("  Timestamp: %lu\n", (long)temp_data.timestamp);
//        printf("  \tValores: \n\t\t S1: %.2f \n\t\t S2: %.2f \n\t\t S3: %.2f \n\t\t S4: %.2f  \n\t\t S5: %.2f  \n\t\t S6: %.2f \n\t\t S7: %.2f\n", 
//                temp_data.valores[0], temp_data.valores[1], temp_data.valores[2], temp_data.valores[3], temp_data.valores[4], temp_data.valores[5], temp_data.valores[6]);

//    }
//    printf("\n");
	return true;
}

void FileM_close_variables_data_file(){
	fclose(variables_file);
	variables_file = NULL;
}

void FileM_remove_variables_file(){
	char* path = "/littlefs/variables_file.bin";
	FileM_Remove_File(path);
}



void FileM_store_door_data(doorState_t* data, uint8_t data_size){
	FILE* f = fopen("/littlefs/door_file.bin","ab");
	
	if (f == NULL){
		ESP_LOGE(TAG_FileM, "Error opening door_file.bin to store!");
	}
	
	fwrite(data, sizeof(doorState_t), data_size, f);
	fclose(f);	
}

size_t FileM_get_door_file_size(){
	size_t file_size = 0;
	
	char* file = "/littlefs/door_file.bin";
	
	file_size = FileM_get_file_size(file);
	
	return file_size;
}

bool FileM_open_door_data_file(){
	if (door_file == NULL) {
		door_file = fopen("/littlefs/door_file.bin","rb");
	}
	
	if (door_file == NULL){
		ESP_LOGE(TAG_FileM, "Error opening door_file.bin to store!");
		return false;
	}
	
	return true;
}

bool FileM_get_door_data(doorState_t* data, uint8_t* data_size){
    printf("\n--- Leyendo datos binarios de la puerta ---\n");

	if (door_file == NULL){
		ESP_LOGE(TAG_FileM, "Error getting data from the door_file! File is closed!");
		return false;
	}
	
    // Leemos mientras haya estructuras completas en el archivo
    size_t data_size_readed = fread(data, sizeof(doorState_t), 20, door_file);
    *data_size = (uint) data_size_readed;
    if (data_size_readed == 20){ 			// If data_size_readed is equal to 20, it means there is more data
    	return false;  
    } 
//    while (fread(&temp_data, sizeof(VariablesData_t), 1, variables_file) == 1) {
//        printf("Registro %d:\n", registro++);
//        printf("  Timestamp: %lu\n", (long)temp_data.timestamp);
//        printf("  \tValores: \n\t\t S1: %.2f \n\t\t S2: %.2f \n\t\t S3: %.2f \n\t\t S4: %.2f  \n\t\t S5: %.2f  \n\t\t S6: %.2f \n\t\t S7: %.2f\n", 
//                temp_data.valores[0], temp_data.valores[1], temp_data.valores[2], temp_data.valores[3], temp_data.valores[4], temp_data.valores[5], temp_data.valores[6]);

//    }
//    printf("\n");
	return true;
}

void FileM_close_door_data_file(){
	fclose(door_file);
	door_file = NULL;
}

void FileM_remove_door_file(){
	char* path = "/littlefs/door_file.bin";
	FileM_Remove_File(path);
}



void FileM_store_weight_data(weightData_t* data, uint8_t data_size){
	FILE* f = fopen("/littlefs/weight_file.bin","ab");
	
	if (f == NULL){
		ESP_LOGE(TAG_FileM, "Error opening weight_file.bin to store!");
	}
	
	fwrite(data, sizeof(weightData_t), data_size, f);
	fclose(f);	
}

size_t FileM_get_weight_file_size(){
	size_t file_size = 0;
	
	char* file = "/littlefs/weight_file.bin";
	
	file_size = FileM_get_file_size(file);
	
	return file_size;
}

bool FileM_open_weight_data_file(){
	if (weight_file == NULL) {
		weight_file = fopen("/littlefs/weight_file.bin","rb");
	}
	
	if (weight_file == NULL){
		ESP_LOGE(TAG_FileM, "Error opening weight_file.bin to store!");
		return false;
	}
	
	return true;
}

bool FileM_get_weight_data(weightData_t* data, uint8_t* data_size){
    printf("\n--- Leyendo datos binarios del peso ---\n");
	
	if (weight_file == NULL){
		ESP_LOGE(TAG_FileM, "Error getting data from the weight_file! File is closed!");
		return false;
	}
    
    // Leemos mientras haya estructuras completas en el archivo
    size_t data_size_readed = fread(data, sizeof(weightData_t), 20, weight_file);
    *data_size = (uint) data_size_readed;
    if (data_size_readed == 20){ 			// If data_size_readed is equal to 20, it means there is more data
    	return false;  
    } 
//    while (fread(&temp_data, sizeof(VariablesData_t), 1, variables_file) == 1) {
//        printf("Registro %d:\n", registro++);
//        printf("  Timestamp: %lu\n", (long)temp_data.timestamp);
//        printf("  \tValores: \n\t\t S1: %.2f \n\t\t S2: %.2f \n\t\t S3: %.2f \n\t\t S4: %.2f  \n\t\t S5: %.2f  \n\t\t S6: %.2f \n\t\t S7: %.2f\n", 
//                temp_data.valores[0], temp_data.valores[1], temp_data.valores[2], temp_data.valores[3], temp_data.valores[4], temp_data.valores[5], temp_data.valores[6]);

//    }
//    printf("\n");
	return true;
}

void FileM_close_weight_data_file(){
	fclose(weight_file);
	weight_file = NULL;
}

void FileM_remove_weight_file(){
	char* path = "/littlefs/weight_file.bin";
	FileM_Remove_File(path);
}



void FileM_store_motorpump_data(motorpumpState_t* data, uint8_t data_size){
	FILE* f = fopen("/littlefs/motorpump_file.bin","ab");
	
	if (f == NULL){
		ESP_LOGE(TAG_FileM, "Error opening motorpump_file.bin to store!");
	}
	
	fwrite(data, sizeof(motorpumpState_t), data_size, f);
	fclose(f);	
}

size_t FileM_get_motorpump_file_size(){
	size_t file_size = 0;
	
	char* file = "/littlefs/motorpump_file.bin";
	
	file_size = FileM_get_file_size(file);
	
	return file_size;
}

bool FileM_open_motorpump_data_file(){
	if (motorpump_file == NULL) {
		motorpump_file = fopen("/littlefs/motorpump_file.bin","rb");
	}
	
	if (motorpump_file == NULL){
		ESP_LOGE(TAG_FileM, "Error opening motorpump_file.bin to store!");
		return false;
	}
	
	return true;
}

bool FileM_get_motorpump_data(motorpumpState_t* data, uint8_t* data_size){
    printf("\n--- Leyendo datos binarios de la motobomba ---\n");
	
	if (motorpump_file == NULL){
		ESP_LOGE(TAG_FileM, "Error getting data from the motorpump_file! File is closed!");
		return false;
	}
	
    // Leemos mientras haya estructuras completas en el archivo
    size_t data_size_readed = fread(data, sizeof(motorpumpState_t), 20, motorpump_file);
    *data_size = (uint) data_size_readed;
    if (data_size_readed == 20){ 			// If data_size_readed is equal to 20, it means there is more data
    	return false;  
    } 
//    while (fread(&temp_data, sizeof(VariablesData_t), 1, variables_file) == 1) {
//        printf("Registro %d:\n", registro++);
//        printf("  Timestamp: %lu\n", (long)temp_data.timestamp);
//        printf("  \tValores: \n\t\t S1: %.2f \n\t\t S2: %.2f \n\t\t S3: %.2f \n\t\t S4: %.2f  \n\t\t S5: %.2f  \n\t\t S6: %.2f \n\t\t S7: %.2f\n", 
//                temp_data.valores[0], temp_data.valores[1], temp_data.valores[2], temp_data.valores[3], temp_data.valores[4], temp_data.valores[5], temp_data.valores[6]);

//    }
//    printf("\n");
	return true;
}

void FileM_close_motorpump_data_file(){
	fclose(motorpump_file);
	motorpump_file = NULL;
}

void FileM_remove_motorpump_file(){
	char* path = "/littlefs/motorpump_file.bin";
	FileM_Remove_File(path);
}



esp_err_t FileM_Remove_File(const char* path){
	
	if(remove(path) == 0){
		ESP_LOGI(TAG_FileM, "Archivo %s eliminado con exito!", path);
		return ESP_OK;
	}else{
		ESP_LOGE(TAG_FileM, "Error al eliminar archivo %s !", path);
		return ESP_FAIL;
	}
}




