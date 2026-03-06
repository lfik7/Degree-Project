
# 🏗️ Hardware Development Hub - Degree Project

## 📝 Descripción

Esta carpeta contiene el ecosistema completo del diseño de hardware para el proyecto de grado. Su finalidad es centralizar toda la documentación técnica, archivos de diseño electrónico y validaciones de ingeniería necesarias para la construcción del dispositivo.

Aquí se gestiona desde el comportamiento lógico de los circuitos de potencia hasta la disposición física de los componentes en la placa, asegurando una transición fluida entre la teoría y la implementación real.

> [!WARNING]
> El desarrollo de Hardware sigue fase de prototipo, por lo que se debe tener en cuenta que tiene fallas. Hasta el momento se tienen dos fallas detectadas: el pin de entrada para el sensor LJ12A3-4Z utiliza un strapping pin del ESP32 por lo que se debe cambiar, adicionalmente, la alimentación del sensor MD-PS002 y de la galga extensiométrica es errónea, debe ser la misma que la alimentación analógica del HX711
> 

---

## 📂 Estructura y Resumen de Contenidos

### 1. [Images](https://www.google.com/search?q=./Images) - Documentación Visual

* **Uso:** Referencia rápida para inspección visual. Ideal para presentaciones, manuales de usuario y revisión de diseño sin necesidad de software especializado.
* **Herramientas:** Capturas de **KiCad 3D Viewer**, diagramas de flujo y renders de alta resolución.

### 2. [PCB](https://www.google.com/search?q=./PCB) - Archivos de Diseño Maestro

* **Uso:** Modificación del trazado de pistas, actualización de componentes y generación de archivos **Gerber** para la fabricación industrial. Es el archivo "vivo" del diseño.
* **Herramientas:** **KiCad EDA** (Recomendado v6.0 o superior) para abrir archivos `.kicad_pcb` y `.kicad_sch`.

### 3. [Schematics](https://www.google.com/search?q=./Schematics) - Simulaciones y Lógica de Potencia

* **Uso:** Validación del control de motores y la estabilidad de la fuente de alimentación. Permite realizar pruebas de estrés virtual y análisis de señales (PWM, rizado de voltaje).
* **Herramientas:** **Proteus Design Suite** (para simulación dinámica) y **LTspice** (para análisis de transitorios).

---

## 🛠️ Stack Tecnológico de Hardware

| Categoría | Herramienta | Función Principal |
| --- | --- | --- |
| **Diseño EDA** | KiCad | Creación de esquemáticos y diseño de PCB. |
| **Simulación** | Proteus / LTspice | Validación de circuitos de alimentación y motores. |
| **Visualización** | Gerber Viewer | Inspección de capas antes de manufactura. |
| **Mecánica** | Fusion 360 / FreeCAD | (Opcional) Para exportar el modelo 3D y diseñar carcasas. |

---

## 🚀 Instrucciones para Desarrolladores

1. **Para fabricar la placa:** Diríjase a la carpeta `/PCB`, abra el proyecto en KiCad y exporte los archivos de fabricación (Plots).
2. **Para entender el flujo de energía:** Revise las simulaciones en `/Schematics` para observar cómo se comporta el driver de los motores.
3. **Para auditoría rápida:** Consulte la carpeta `/Images` para ver los planos finales en formato PDF/PNG.
