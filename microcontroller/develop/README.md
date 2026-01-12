# Develop folder
Here is the full code (different files) of full system to build in the ESP32

## Current distribution of the sensors in the files
- The sensors SEN0193, MQ-2, MQ-3, and MQ-135 are in the file [I2C_DS1219_Test.c](https://github.com/lfik7/Degree-Project/blob/main/microcontroller/Test/I2C_ADS1219_Test.c)
- The sensors MD-PS002 and strain gauge are in the file [HX711_Test.c](https://github.com/lfik7/Degree-Project/blob/main/microcontroller/Test/HX711_Test.c)

### Notes:
- root_ca.pem file is for the HTTPS conection (Firebase_Test.c)
- partitions.csv file is for the flash memory distribution
