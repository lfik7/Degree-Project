
// Agregar el componente al proyecto: idf.py add-dependency "joltwallet/littlefs"


#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <strings.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <dirent.h>
#include <sys/stat.h>
#include "esp_littlefs.h"
#include "esp_log.h"

struct current_settigns{
	char WiFi_SSID[32];
	char WiFi_PSSW[32];
	uint16_t SAMP_INT;
}Cur_Set = {"Familia Vargas ETB 2.4G", "LMSD167294385", 60};

struct WiFi_SSID_PSSW{
	char WiFi_SSID[32];
	char WiFi_PSSW[32];
}W_ID_PSW[2] = {
	{"Familia Vargas ETB 2.4G", "LMSD167294385"},
	{"moto g52 Lotfi","98100652ldv"}
};

struct SensorData {
    uint32_t timestamp;
    float valores[7];
} mi_data;


esp_err_t Montar_LittleFS_FileSystem();
void guardar_csv();
void guardar_binario();
void listar_archivos_recursivo(const char *ruta_base) ;
uint8_t leer_csv(const char* ruta);
void cargar_configuracion_actual(const char* ruta, struct current_settigns*);
void cargar_configuracion_wifi(const char* ruta, struct WiFi_SSID_PSSW*, uint8_t desired_WiFi_option);
void leer_binario(const char* ruta);
size_t determinar_tamano_archivo(const char* ruta);
void cargar_lectura_sensores(const char* ruta, struct SensorData* data);


void app_main(void) {
	esp_err_t File_system;  
	File_system = Montar_LittleFS_FileSystem();
	if (File_system != ESP_OK) return;
	sleep(1);
	
//	printf("Guardando informacion!\n");
//	guardar_csv();
//	guardar_binario();
//	sleep(3);
	
	printf("Verificando espacio usado!\n");
	size_t total = 0, used = 0;
	esp_err_t ret = esp_littlefs_info("storage", &total, &used);
	if (ret == ESP_OK) {
	    printf("Particion 'storage': Total: %d bytes, Usado: %d bytes\n", total, used);
	}
	else{
		printf("Error adquiiendo informacion de la particion storage!\n");
	}

	listar_archivos_recursivo("/littlefs");
	
	sleep(2);
	
	printf("Leyendo archivos...\n");
	uint8_t lineas_archivoCSV = 0;
	lineas_archivoCSV = leer_csv("/littlefs/Current_Settings.csv");
	printf("El archivo tiene %d lineas\n", lineas_archivoCSV);
	sleep(1);
	lineas_archivoCSV = leer_csv("/littlefs/WiFi_SSID_PSSW.csv");
	printf("El archivo tiene %d lineas\n", lineas_archivoCSV);
	sleep(1);
	leer_binario("/littlefs/Sens_data.bin");
	
	sleep(2);
	printf("Cargando archivos...\n");
	cargar_configuracion_actual("/littlefs/Current_Settings.csv", &Cur_Set);
	sleep(1);
	struct WiFi_SSID_PSSW WiFi_ID_PSSW;
	cargar_configuracion_wifi("/littlefs/WiFi_SSID_PSSW.csv", &WiFi_ID_PSSW, 1);
	sleep(1);
	determinar_tamano_archivo("/littlefs/Sens_data.bin");
}


esp_err_t Montar_LittleFS_FileSystem(){
    // Configuración y montaje
    ESP_LOGI("LittleFS","Montando sistema de archivos...");
    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/littlefs",
        .partition_label = "storage",
        .format_if_mount_failed = true,
        .dont_mount = false
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE("LittleFS", "Error al montar LittleFS");
    }
	
	return ret;
}



void guardar_csv() {

    // Apertura del archivo en modo Append ("a")
    // Esto añade datos sin borrar lo anterior y sin cargar todo el archivo en RAM
    FILE* f = fopen("/littlefs/Current_Settings.csv", "w");
    if (f == NULL) {
        ESP_LOGE("LittleFS", "Error al abrir el archivo Current_Settings.csv");
        return;
    }
    // Escritura de la línea CSV
    // Ejemplo: timestamp, sensor1, sensor2, sensor3...
    fprintf(f, "# WiFi_SSID,WiFi_PSSW,SAMP_INT(s)\n");
    fprintf(f, "%s,%s,%d\n", Cur_Set.WiFi_SSID, Cur_Set.WiFi_PSSW, Cur_Set.SAMP_INT);
    // Cerrar para asegurar la escritura física
    fclose(f);
    
   f = fopen("/littlefs/WiFi_SSID_PSSW.csv", "w");
    if (f == NULL) {
        ESP_LOGE("LittleFS", "Error al abrir el archivo WiFi_SSID_PSSW.csv");
        return;
    }
    // Escritura de la línea CSV
    // Ejemplo: timestamp, sensor1, sensor2, sensor3...
    fprintf(f, "# SSID,PSSW\n");
    fprintf(f, "%s,%s\n", W_ID_PSW[0].WiFi_SSID, W_ID_PSW[0].WiFi_PSSW);
    fprintf(f, "%s,%s\n", W_ID_PSW[1].WiFi_SSID, W_ID_PSW[1].WiFi_PSSW);
    // Cerrar para asegurar la escritura física
    fclose(f);

//    // Desmontar (opcional, si ya no vas a usarlo más)
//    esp_vfs_littlefs_unregister("storage");
}



void guardar_binario(){
	    
    // Apertura del archivo en modo Append ("a")
    // Esto añade datos sin borrar lo anterior y sin cargar todo el archivo en RAM
   	FILE* f = fopen("/littlefs/Sens_data.bin", "ab");
    if (f == NULL) {
        ESP_LOGE("LittleFS", "Error al abrir el archivo Sens_data.bin");
        return;
    }
	mi_data.timestamp = (uint32_t)time(NULL);
	mi_data.valores[0] = 25.5;
	mi_data.valores[1] = 3.87;
	mi_data.valores[2] = 18.9;
	mi_data.valores[3] = 0.5;
	mi_data.valores[4] = 2.98;
	mi_data.valores[5] = 105.9;
	mi_data.valores[6] = 0.008;
	fwrite(&mi_data,sizeof(struct SensorData), 1, f);
	fclose(f);
	
//    // Desmontar (opcional, si ya no vas a usarlo más)
//    esp_vfs_littlefs_unregister("storage");
}



void listar_archivos_recursivo(const char *ruta_base) {
    DIR *dir = opendir(ruta_base);
    if (!dir) return;

    struct dirent *entrada;
    while ((entrada = readdir(dir)) != NULL) {
        char ruta_completa[300];
        snprintf(ruta_completa, sizeof(ruta_completa), "%s/%s", ruta_base, entrada->d_name);

        struct stat st;
        if (stat(ruta_completa, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                // Es un directorio, entramos recursivamente
                if (strcmp(entrada->d_name, ".") != 0 && strcmp(entrada->d_name, "..") != 0) {
                    printf("[DIR]  %s\n", ruta_completa);
                    listar_archivos_recursivo(ruta_completa);
                }
            } else {
                // Es un archivo, imprimimos su tamaño
                printf("[FILE] %-30s | Tamanio: %ld bytes\n", ruta_completa, st.st_size);
            }
        }
    }
    closedir(dir);
}



uint8_t leer_csv(const char* ruta) {
    printf("\n--- Leyendo archivo: %s ---\n", ruta);
    FILE* f = fopen(ruta, "r");
    if (f == NULL) {
        ESP_LOGE("LittleFS", "Error al abrir el archivo para lectura");
        return 0;
    }

    char linea[128];
    uint8_t contador_lineas = 0;
    while (fgets(linea, sizeof(linea), f) != NULL) {
        // Imprimimos la línea leída
        printf("%s", linea);
        contador_lineas ++;
    }
    fclose(f);
    return contador_lineas;
}



void cargar_configuracion_actual(const char* ruta, struct current_settigns* Cur_set){
	printf("Cargando archivo %s:\n", ruta);
    FILE* f = fopen(ruta, "r");
    if (f == NULL) return;

    char linea[128];

    while (fgets(linea, sizeof(linea), f)) {
        // Ignorar comentarios o líneas vacías
        if (linea[0] == '#' || linea[0] == '\n') continue;

        // sscanf busca el formato: texto hasta la coma, saltar coma, texto hasta el final
        // %31[^,] lee hasta 31 caracteres que NO sean una coma
        int samp_int;
        if (sscanf(linea, "%31[^,],%31[^,],%d[^,\n]", Cur_set->WiFi_SSID, Cur_set->WiFi_PSSW, &samp_int) == 3) {
			Cur_set->SAMP_INT = samp_int;
            printf("\tSSID cargado: %s\n", Cur_set->WiFi_SSID);
            printf("\tPassword cargado: %s\n", Cur_set->WiFi_PSSW);
            printf("\tIntervalo de muestreo cargado (s): %d\n", Cur_set->SAMP_INT);
        }
        break;
    }
    
    fclose(f);
	
}



void cargar_configuracion_wifi(const char* ruta, struct WiFi_SSID_PSSW* WiFi_ID_PSSW, uint8_t desired_WiFi_option) {
    printf("Cargando archivo %s:\n", ruta);
    FILE* f = fopen(ruta, "r");
    if (f == NULL) return;

    char linea[128];
    uint8_t contador_linea = 0;

    while (fgets(linea, sizeof(linea), f)) {
        // Ignorar comentarios o líneas vacías
        if (linea[0] == '#' || linea[0] == '\n') continue;

        // sscanf busca el formato: texto hasta la coma, saltar coma, texto hasta el final
        // %31[^,] lee hasta 31 caracteres que NO sean una coma
        if(contador_linea == desired_WiFi_option){
	        if (sscanf(linea, "%31[^,],%31[^\n]", WiFi_ID_PSSW->WiFi_SSID, WiFi_ID_PSSW->WiFi_PSSW) == 2) {
	            printf("\tSSID cargado: %s\n", WiFi_ID_PSSW->WiFi_SSID);
	            printf("\tPassword cargado: %s\n", WiFi_ID_PSSW->WiFi_PSSW);
	        }	
	        break;
		}
		contador_linea ++;
    }
    
    if(contador_linea < desired_WiFi_option){
		ESP_LOGE("Cargar WiFi","Solo hay %d opciones. No se encontro la %d opcion deseada", contador_linea, desired_WiFi_option);
	}
    
    fclose(f);
}


void leer_binario(const char* ruta) {
    printf("\n--- Leyendo datos binarios: %s ---\n", ruta);
    FILE* f = fopen(ruta, "rb"); // "rb" para lectura binaria
    if (f == NULL) {
        ESP_LOGE("LittleFS", "Error al abrir binario");
        return;
    }

    struct SensorData temp_data;
    int registro = 1;

    // Leemos mientras haya estructuras completas en el archivo
    while (fread(&temp_data, sizeof(struct SensorData), 1, f) == 1) {
        printf("Registro %d:\n", registro++);
        printf("  Timestamp: %lu\n", (long)temp_data.timestamp);
        printf("  \tValores: \n\t\t S1: %.2f \n\t\t S2: %.2f \n\t\t S3: %.2f \n\t\t S4: %.2f  \n\t\t S5: %.2f  \n\t\t S6: %.2f \n\t\t S7: %.2f\n", 
                temp_data.valores[0], temp_data.valores[1], temp_data.valores[2], temp_data.valores[3], temp_data.valores[4], temp_data.valores[5], temp_data.valores[6]);
    }
    fclose(f);
    printf("\n");
}


size_t determinar_tamano_archivo(const char* ruta){
	
	struct stat st;
	if (stat(ruta, &st) == 0) {
	    if (S_ISDIR(st.st_mode)) {
	        // Es un directorio, entramos recursivamente
	       ESP_LOGE("Det Tam Arch","Es un directorio!");
	       return 0;
	    } else {
	        // Es un archivo, imprimimos su tamaño
	        printf("[FILE] %-30s | Tamanio: %ld bytes\n", ruta, st.st_size);
	    }
	}
	return st.st_size;
}


void cargar_lectura_sensores(const char* ruta, struct SensorData* data){
    printf("\n--- Leyendo datos binarios: %s ---\n", ruta);
    FILE* f = fopen(ruta, "rb"); // "rb" para lectura binaria
    if (f == NULL) {
        ESP_LOGE("LittleFS", "Error al abrir binario");
        return;
    }

    struct SensorData temp_data;
    int registro = 1;

    // Leemos mientras haya estructuras completas en el archivo
    while (fread(&temp_data, sizeof(struct SensorData), 1, f) == 1) {
        printf("Registro %d:\n", registro++);
        printf("  Timestamp: %lu\n", (long)temp_data.timestamp);
        printf("  \tValores: \n\t\t S1: %.2f \n\t\t S2: %.2f \n\t\t S3: %.2f \n\t\t S4: %.2f  \n\t\t S5: %.2f  \n\t\t S6: %.2f \n\t\t S7: %.2f\n", 
                temp_data.valores[0], temp_data.valores[1], temp_data.valores[2], temp_data.valores[3], temp_data.valores[4], temp_data.valores[5], temp_data.valores[6]);
        data[registro-1].timestamp = temp_data.timestamp;
        data[registro-1].valores[0] = temp_data.valores[0];
        data[registro-1].valores[1] = temp_data.valores[1];
        data[registro-1].valores[2] = temp_data.valores[2];
        data[registro-1].valores[3] = temp_data.valores[3];
        data[registro-1].valores[4] = temp_data.valores[4];
        data[registro-1].valores[5] = temp_data.valores[5];
        data[registro-1].valores[6] = temp_data.valores[6];
    }
    fclose(f);
    printf("\n");
	
}

