# 🔬 Microcontroller Component Tests - Degree Project

## 📝 Descripción

Esta carpeta contiene los módulos de prueba individuales desarrollados durante la fase de prototipado. El objetivo principal es **aislar y validar el funcionamiento de cada componente de hardware** (sensores, drivers de motores, comunicación, protocolos, sistema de archivos) antes de su integración en el firmware principal.

**Problema que resuelve:**

* **Depuración acelerada:** Permite identificar fallos de hardware o conexiones sin la complejidad del código completo.
* **Base de Librerías:** Estos archivos sirvieron como "Proof of Concept" (PoC) para estructurar las librerías personalizadas del proyecto.
* **Validación de Periféricos:** Pruebas específicas para protocolos I2C, SPI, PWM y UART en el ESP32.

---

## ⚙️ Instalación y Configuración

Para ejecutar estos tests, necesitas tener configurado el entorno oficial de Espressif.

1. **Clonar el repositorio:**
```bash
git clone https://github.com/lfik7/Degree-Project.git
cd Degree-Project/microcontroller/Test

```


2. **Configurar el entorno (Espressif IDF):**
  * Asegúrate de tener instalado **ESP-IDF v5.x**.
  * En la terminal de ESP-IDF, navega a la subcarpeta del test que desees probar (ej: `Motor_Test`).


3. **Compilar y Flashear:**
```bash
idf.py set-target esp32
idf.py build
idf.py -p [PUERTO_COM] flash monitor

```



---

## 🚀 Uso y Ejemplos

Cada archivo representa un test independiente. Por lo general, el flujo de uso es:

* **Entrada:** Conectar el componente al pinout definido en el archivo de cada test.
* **Salida:** Monitoreo de logs a través de la consola serial (UART).

**Ejemplo de flujo de prueba:**

> Al ejecutar el test de motores, la consola mostrará el ciclo de trabajo (Duty Cycle) del PWM y la respuesta de los encoders en tiempo real, permitiendo calibrar los parámetros PID iniciales.

---

## 🛠️ Tecnologías Utilizadas

Este módulo de pruebas se apoya en el stack oficial de desarrollo profesional para ESP32:

* **Lenguajes:** C / C++ (Optimizado para sistemas embebidos).
* **Framework:** **ESP-IDF (Espressif IoT Development Framework)**, utilizando el sistema de construcción CMake.
* **RTOS:** FreeRTOS (Para la gestión de tareas y temporizadores en los tests).
* **Protocolos Simulados:**
* **MCPWM** Para control de velocidad de motores.
* **I2C:** Para lectura de sensores inerciales o pantallas.
* **GPIO/Interrupts:** Para lectura de encoders y botones de pánico.



---

## 📂 Estructura de la Carpeta

* `/Test_Sensor_X`: Código base para la lectura de datos crudos.
* `/Test_Motor_Control`: Script de validación de puente H y señales PWM.
* `/Test_Communication`: Pruebas de conectividad (Wi-Fi/Bluetooth si aplica).

### Current distribution of the sensors in the files
- The sensors SEN0193, MQ-2, MQ-3, and MQ-135 are in the file [I2C_DS1219_Test.c](https://github.com/lfik7/Degree-Project/blob/main/microcontroller/Test/I2C_ADS1219_Test.c)
- The sensors MD-PS002 and strain gauge are in the file [HX711_Test.c](https://github.com/lfik7/Degree-Project/blob/main/microcontroller/Test/HX711_Test.c)

> [!NOTE]
> root_ca.pem file is for the HTTPS conection (Firebase_Test.c)
> 

