
# 🧠 Firmware Management Hub - Degree Project

## 📝 Descripción

Este repositorio centraliza toda la lógica de control embebida en el microcontrolador ESP32. El software aquí desarrollado gestiona la interacción entre los actuadores, la adquisición de datos de los sensores y la comunicación externa.

**Problema que resuelve:**

* **Integración Sistémica:** Unifica componentes de hardware individuales en un sistema autónomo funcional.
* **Control de Tiempo Real:** Garantiza que las tareas críticas (como el control de motores y la seguridad) se ejecuten sin retardos gracias a la arquitectura de **FreeRTOS**.
* **Escalabilidad:** Proporciona un entorno estructurado donde se pueden añadir nuevas funcionalidades sin comprometer la estabilidad del núcleo del firmware.

---

## ⚙️ Instalación y Configuración

Para trabajar con este firmware, se recomienda el uso del entorno oficial de **Espressif**.

1. **Clonación del repositorio:**
```bash
git clone https://github.com/lfik7/Degree-Project.git
cd Degree-Project/microcontroller

```


2. **Configuración del Entorno:**
    * Instale **Espressif IDE** o la extensión de **ESP-IDF** para VS Code.
    * Configure las variables de entorno ejecutando el script `export.sh` (Linux/macOS) o `export.bat` (Windows) de su instalación de ESP-IDF.


3. **Compilación de un módulo:**
Navegue a la carpeta de interés (ej: `develop`) y ejecute:
```bash
idf.py build

```



---

## 📂 Estructura de la Carpeta

### 1. [Test](https://www.google.com/search?q=./Test) - Validación Unitaria

* **Uso:** Ideal para realizar pruebas aisladas de periféricos (encoders, pantallas, sensores) y calibrar parámetros antes de la integración final.
* **Herramientas:** **Espressif IDE** para compilación y **Serial Monitor** para depuración de señales.

### 2. [Develop](https://www.google.com/search?q=./develop) - Firmware Principal

* **Uso:** Contiene el código de producción, las librerías personalizadas (`instruments`), la gestión de certificados de seguridad y la tabla de particiones de memoria.
* **Herramientas:** **ESP-IDF v5.x** (Framework principal), **CMake** (Sistema de construcción) y **MbedTLS** (Seguridad).

---

## 🚀 Uso y Visualización

El firmware está diseñado para reportar su estado constantemente a través del puerto serie.

**Ejemplo de flujo de ejecución:**
Al encender el dispositivo, el firmware inicia el stack de red y los drivers de potencia. Se pueden visualizar logs de este tipo:

```text
I (100) boot: ESP-IDF v5.1-dirty 2nd stage bootloader
I (120) cpu_start: Starting FreeRTOS scheduler...
I (250) MOTOR_CTRL: PID Controller initialized with Kp=1.2, Ki=0.5
I (500) WIFI_STA: Connected to AP successfully.

```

---

## 🛠️ Tecnologías Utilizadas

* **Lenguaje:** C y C++ (Optimizado para bajo nivel).
* **Framework:** **ESP-IDF** (Entorno de desarrollo nativo de Espressif).
* **Sistema Operativo:** **FreeRTOS** (Multitarea y gestión de colas/semáforos).
* **Bases de Datos / Persistencia:** **NVS (Non-Volatile Storage)** para guardar estados y configuraciones de red entre reinicios.
