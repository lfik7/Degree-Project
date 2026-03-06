# 🛰️ Hardware & Circuit Design - Degree Project

## 📝 Descripción

Esta carpeta constituye el núcleo del desarrollo físico del proyecto. Contiene la documentación visual y técnica de las placas de circuito impreso (PCB) y los diagramas esquemáticos. Su finalidad es servir como guía de referencia para el ensamblaje, la verificación de conexiones y la validación del diseño electrónico antes y después de la fabricación.

---

## 🖼️ Uso de las Imágenes

Las imágenes contenidas en esta sección tienen propósitos específicos para el ciclo de vida del desarrollo:

* **Esquemáticos (`.png` / `.jpg`):** Utilízalos para entender la lógica de conexión entre el microcontrolador, los sensores y la etapa de potencia sin necesidad de instalar el software de diseño.
* **Layout de la PCB:** Crucial para identificar la ubicación física de los componentes durante el proceso de soldadura (Soldering Guide).
* **Vistas 3D:** Empleadas para el diseño de carcasas o enclosures, permitiendo verificar alturas de componentes y espacios mecánicos.

---

## 🛠️ Herramientas de Desarrollo

Para visualizar los archivos fuente originales, realizar simulaciones o expandir el proyecto, se utilizaron o recomiendan las siguientes herramientas:

| Software | Función | Estado |
| --- | --- | --- |
| **KiCad EDA** | Diseño de esquemáticos y trazado de PCB (Layout). | Principal |
| **Proteus Design Suite** | Simulación de circuitos analógicos y digitales. | Validación |
| **EasyEDA** | Alternativa ligera para visualización rápida de archivos Gerber. | Visualización |
| **LTspice** | Simulación específica de la etapa de regulación de voltaje. | Análisis |

> [!TIP]
> Si deseas modificar el diseño, te recomendamos descargar los archivos `.kicad_pcb` (disponibles en la raíz de la carpeta de hardware) y abrirlos con la versión 6.0 o superior de KiCad.

---

## 🚀 Cómo continuar

1. **Revisión de Errores:** Compara los esquemáticos con el manual de usuario de los componentes principales.
2. **Generación de Gerbers:** Si deseas mandar a fabricar la placa, utiliza el software KiCad para exportar los archivos necesarios para la manufactura industrial.
