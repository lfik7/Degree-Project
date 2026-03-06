
# 🚀 Degree Project: Integrated Control & Monitoring System

## 📝 Descripción

Este proyecto de grado consiste en un sistema embebido integral diseñado para el **monitoreo remoto en tiempo real**. Resuelve la brecha entre la gestión de hardware industrial/robótico y la accesibilidad de una interfaz móvil moderna.

**Problemas que resuelve:**

* **Interconectividad:** Integra el control de bajo nivel (motores y sensores) con la nube (Firebase) y dispositivos móviles (Flutter).
* **Autonomía y Precisión:** Implementa algoritmos de control en el ESP32 para garantizar respuestas rápidas ante estímulos del entorno.
* **Telemetría Remota:** Permite la supervisión constante de variables críticas sin necesidad de presencia física en el sitio de operación.

---

## 📂 Estructura del Ecosistema

El proyecto está dividido en tres pilares fundamentales:

1. **[Hardware Circuits](https://www.google.com/search?q=./hardware-circuits):** Diseño electrónico, esquemáticos de potencia y archivos de fabricación de la PCB.
2. **[Microcontroller](https://www.google.com/search?q=./microcontroller):** Firmware desarrollado en ESP-IDF (C/C++) con arquitectura multi-tarea bajo FreeRTOS.
3. **[Mobile App](https://www.google.com/search?q=./mobile-app):** Aplicación multiplataforma en Flutter para el mando, control y visualización de datos.

---

## ⚙️ Instalación y Configuración

Para replicar el proyecto completo, sigue este orden de configuración:

### 1. Clonar el Repositorio

```bash
git clone https://github.com/lfik7/Degree-Project.git

```

### 2. Entorno de Hardware

* Consulta la carpeta `/hardware-circuits` para obtener la lista de materiales (BOM).
* Abre los archivos en **KiCad** si deseas modificar la placa base.

### 3. Entorno de Firmware (ESP32)

* Requiere **ESP-IDF v5.x**.

```bash
cd microcontroller/develop
idf.py build flash monitor

```

### 4. Entorno de Software (App)

* Requiere **Flutter SDK**.

```bash
cd mobile-app
flutter pub get
flutter run

```

---

## 🛠️ Stack Tecnológico

| Dominio | Tecnologías |
| --- | --- |
| **Hardware** | KiCad (PCB), Proteus (Simulación), ESP32 (MCU) |
| **Firmware** | C, C++, ESP-IDF, FreeRTOS |
| **Mobile App** | Dart, Flutter, GetX/Provider |
| **Backend/Cloud** | Firebase Auth, Firestore, Cloud Messaging |

---

## 🚀 Uso y Funcionamiento

El sistema funciona como un lazo cerrado de control:

1. El **Hardware** captura señales (encoders, sensores) y maneja los actuadores.
2. El **Firmware** procesa los datos y los sincroniza con la nube mediante protocolos seguros.
3. El **Usuario** visualiza el estado en la **App** y envía comandos de vuelta al hardware en milisegundos.

---

## 🤝 Contribuciones y Desarrollo

Este proyecto fue desarrollado como requisito para optar al título de Ingeniería. Si deseas continuar el desarrollo:

* Revisa las pruebas unitarias en `microcontroller/Test`.
* Configura tus propios certificados en `microcontroller/develop/main/certs`.
* Vincula tu propio proyecto de Firebase en la carpeta `mobile-app`.
