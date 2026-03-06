
# 📱 Mobile Application - Degree Project

## 📝 Descripción

Esta carpeta contiene el código fuente de la aplicación móvil diseñada para el control y monitoreo en tiempo real del sistema. La app actúa como el puente entre el usuario y el ESP32, permitiendo una gestión intuitiva del hardware.

**Problema que resuelve:**

* **Visualización de Datos:** Transforma las señales crudas de los sensores en gráficas e indicadores comprensibles.
* **Experiencia de Usuario (UX):** Proporciona una interfaz moderna que abstrae la complejidad técnica del sistema embebido.

---

## ⚙️ Instalación y Configuración

Para ejecutar este proyecto, necesitas tener instalado el SDK de Flutter y las herramientas de Android/iOS.

1. **Clonación del repositorio:**
```bash
git clone https://github.com/lfik7/Degree-Project.git
cd Degree-Project/mobile-app

```


2. **Instalación de dependencias:**
```bash
flutter pub get

```


3. **Verificación de dispositivos:**
```bash
flutter devices

```


4. **Ejecución en modo Debug:**
```bash
flutter run

```



---

## 📂 Organización del Proyecto (Clean Architecture)

El proyecto está estructurado para facilitar el mantenimiento y la escalabilidad:

| Carpeta | Propósito | Ejemplo de Contenido |
| --- | --- | --- |
| **`/control`** | Lógica de Negocio | Controladores (GetX/Bloc) que manejan la lógica del motor. |
| **`/core`** | Utilidades y Servicios | Configuración de red, clientes MQTT/HTTP y manejo de errores. |
| **`/globals`** | Constantes y Estilos | Definiciones de colores, temas y rutas globales de la app. |
| **`/ui`** | Capa de Presentación | Widgets, pantallas (Screens) y componentes de la interfaz. |

---

## 🚀 Uso y Capturas

La aplicación se conecta al hardware y permite al usuario operar el sistema mediante:

* **Joystick/Botones Virtuales:** Para el movimiento de motores.
* **Dashboard de Telemetría:** Lectura de batería, temperatura y estados.

**Ejemplo de código (Inyección de dependencias):**

> En la carpeta `/control`, se gestionan los estados para que la UI se actualice automáticamente cuando llega un nuevo dato del ESP32.

---

## 🛠️ Tecnologías y Herramientas

* **Lenguaje:** **Dart**.
* **Framework:** **Flutter**.
* **Gestión de Estado:** (Ej: GetX, Provider o BLoC).
* **Protocolos de Comunicación:** HTTP / WebSockets / MQTT (según implementación).
* **Herramientas de Desarrollo:**
* **VS Code / Android Studio:** Para el desarrollo del código.
* **Flutter DevTools:** Para inspeccionar el rendimiento y el layout.
* **Postman:** Para probar las APIs o endpoints de comunicación.



---

## 🧪 Pruebas y Continuidad

Para continuar el desarrollo o realizar pruebas de integración:

1. **Mock Data:** En la carpeta `/core`, puedes simular datos de entrada para probar la UI sin el hardware físico.
2. **Widgets Tests:** Ejecuta `flutter test` para validar que los componentes de la UI respondan correctamente.

---

## 🔥 Configuración de Firebase

La aplicación utiliza **Firebase** como infraestructura de backend para servicios de tiempo real, autenticación y notificaciones. Para configurar el entorno de desarrollo, sigue estos pasos:

### 1. Requisitos Previos

* Tener instalado el **Firebase CLI** (`npm install -g firebase-tools`).
* Haber iniciado sesión con `firebase login`.
* Tener instalado el comando de configuración de FlutterFire:
```bash
dart pub global activate flutterfire_cli

```



### 2. Vinculación del Proyecto

Para conectar la app con tu proyecto de Firebase, ejecuta el asistente de configuración desde la raíz de `mobile-app`:

```bash
flutterfire configure

```

Esto generará automáticamente el archivo `lib/firebase_options.dart` con las credenciales necesarias para Android e iOS.

### 3. Servicios Utilizados

* **Firebase Auth:** Gestión de usuarios (Login/Registro) para el acceso seguro al control del hardware.
* **Cloud Firestore / Realtime Database:** Almacenamiento y sincronización en tiempo real de la telemetría enviada por el ESP32.
* **Firebase Cloud Messaging (FCM):** Envío de alertas críticas al dispositivo móvil (ej: "Batería baja" o "Obstrucción detectada").
* **Crashlytics:** Monitoreo de errores en la aplicación para mejorar la estabilidad en producción.

### 4. Inicialización en el Código

Asegúrate de que el método `main()` en `lib/main.dart` incluya la inicialización:

```dart
void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await Firebase.initializeApp(
    options: DefaultFirebaseOptions.currentPlatform,
  );
  runApp(MyApp());
}

```

> [!IMPORTANT]
> Los archivos de configuración específicos de la plataforma (`google-services.json` para Android y `GoogleService-Info.plist` para iOS) no se deben subir al repositorio público por razones de seguridad si el proyecto es de código abierto.
