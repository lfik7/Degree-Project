/*
 * File_Manager.h
 *
 *  Created on: 26/12/2025
 *      Author: Lotfi Dalal
 */

#ifndef FILE_MANAGER_H_
#define FILE_MANAGER_H_



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

#include "Globals.h"





void FileM_init();
void FileM_store_new_settings(current_settigns_t*);
void FileM_get_current_settings(current_settigns_t*);
void FileM_store_wifi_net(WiFi_SSID_PSSW_t*, uint8_t quantity_positions, bool new_file);
bool FileM_get_wifi_net(const char* red_wifi, WiFi_SSID_PSSW_t*);
void FileM_get_all_wifi_nets(WiFi_SSID_PSSW_t*);
void FileM_store_variables_data(VariablesData_t* data, uint8_t data_size);
size_t FileM_get_variables_file_size();
bool FileM_open_variables_data_file();
bool FileM_get_variables_data(VariablesData_t* data, uint8_t* data_size);
void FileM_close_variables_data_file();
void FileM_remove_variables_file();
void FileM_store_door_data(doorState_t* data, uint8_t data_size);
size_t FileM_get_door_file_size();
bool FileM_open_door_data_file();
bool FileM_get_door_data(doorState_t* data, uint8_t* data_size);
void FileM_close_door_data_file();
void FileM_remove_door_file();
void FileM_store_weight_data(weightData_t* data, uint8_t data_size);
size_t FileM_get_weight_file_size();
bool FileM_open_weight_data_file();
bool FileM_get_weight_data(weightData_t* data, uint8_t* data_size);
void FileM_close_weight_data_file();
void FileM_remove_weight_file();
void FileM_store_motorpump_data(motorpumpState_t* data, uint8_t data_size);
size_t FileM_get_motorpump_file_size();
bool FileM_open_motorpump_data_file();
bool FileM_get_motorpump_data(motorpumpState_t* data, uint8_t* data_size);
void FileM_close_motorpump_data_file();
void FileM_remove_motorpump_file();
void FileM_list_files(const char* base_path);








#endif /* FILE_MANAGER_H_ */
