
# 🧠 Core Firmware Development - Degree Project

## 📝 Descripción

Este directorio contiene el código fuente definitivo y la arquitectura lógica del sistema embebido. El firmware resuelve la integración compleja de sensores, actuadores y protocolos de comunicación en tiempo real.

**Problema que resuelve:**

* **Gestión Multitarea:** Implementa un sistema operativo de tiempo real (**FreeRTOS**) para manejar simultáneamente el control de motores, la lectura de sensores y la conectividad.
* **Modularidad:** Separa la lógica de bajo nivel (drivers) de la lógica de aplicación mediante una estructura de componentes y librerías personalizadas.
* **Seguridad y Estabilidad:** Maneja certificados para conexiones seguras y una tabla de particiones optimizada para actualizaciones o almacenamiento de datos.

---

## ⚙️ Instalación y Configuración

Sigue estos pasos para replicar el entorno de desarrollo en **Espressif IDE** o VS Code con la extensión ESP-IDF.

1. **Clonar el repositorio:**
```bash
git clone https://github.com/lfik7/Degree-Project.git
cd Degree-Project/microcontroller/develop

```


2. **Configurar el Target:**
```bash
idf.py set-target esp32

```


3. **Menú de Configuración (Opcional):**
Si necesitas ajustar parámetros de Wi-Fi o pines desde el SDK:
```bash
idf.py menuconfig

```


4. **Compilar y Flashear:**
```bash
idf.py build flash monitor

```



---

## 🚀 Organización del Proyecto

El proyecto sigue la estructura estándar de **ESP-IDF**, optimizada para la escalabilidad:

### 📁 Componentes y Librerías (`/components` e `/instruments`)

* **Archivos Fuentes (`.c`):** Contienen la implementación lógica y algoritmos de control (ej. cálculos PID, procesamiento de señales).
* **Archivos de Cabecera (`.h`):** Definen las interfaces, estructuras de datos y constantes, permitiendo que otros módulos utilicen las funciones sin conocer la implementación interna.
* **`/instruments`:** Librerías especializadas diseñadas para este proyecto (sensores específicos, manejo de protocolos propietarios).

### 📁 Carpeta `main/certs`

Contiene los **Certificados SSL/TLS** (archivos `.pem` o `.crt`). Son esenciales para establecer conexiones seguras con servidores externos (MQTT, HTTPs o Nube), garantizando que los datos enviados desde el ESP32 no sean interceptados.

### 📄 Archivo `partitions.csv`

Define la distribución de la memoria Flash del ESP32.

* Determina el tamaño del área de código (App).
* Reserva espacio para **NVS** (almacenamiento de variables no volátiles).
* Configura particiones **OTA** (Over-The-Air) para actualizaciones inalámbricas.
* Asigna espacio para sistemas de archivos como **SPIFFS** o **LittleFS**.

---

## 🛠️ Tecnologías Utilizadas

* **Lenguajes:** C y C++ (Estándar de industria para sistemas críticos).
* **Framework:** **ESP-IDF (Espressif IoT Development Framework)**.
* **OS:** **FreeRTOS** (Kernel de tiempo real).
* **Seguridad:** MbedTLS para cifrado de datos y gestión de certificados.
* **Base de Datos/Storage:** **NVS (Non-Volatile Storage)** para persistencia de configuraciones locales.

---

## 📸 Ejemplo de Uso / Logs

Al iniciar, el sistema realiza una secuencia de inicialización que se puede observar en el monitor serial:

```text
I (512) SYSTEM: Initializing motor drivers...
I (645) SENSORS: IMU Calibration complete.
I (1022) WIFI: Connected to SSID: Degree_Project_AP
I (1540) CLOUD: SSL Certificate validated successfully.

```
