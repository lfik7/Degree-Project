
# 🛠️ Circuit Design & PCB Source Files - Degree Project

## 📝 Descripción

Esta carpeta contiene los archivos fuente oficiales del diseño electrónico del proyecto. A diferencia de la carpeta de imágenes, aquí se alojan los archivos de proyecto, librerías y hojas de ruta necesarios para la edición, simulación y exportación de archivos de fabricación (Gerbers).

La finalidad de este directorio es permitir la **reproducibilidad total** del hardware, facilitando que otros desarrolladores puedan auditar el diseño o realizar mejoras estructurales.

---

## 📂 Contenido y Uso de los Archivos

Los archivos en esta carpeta están destinados a ingenieros y técnicos:

* **Esquemáticos (`.sch` / `.kicad_sch`):** Contienen la lógica eléctrica. Se usan para verificar las conexiones de los pines, valores de resistencias/capacitores y el flujo de señales.
* **Diseño de PCB (`.pcb` / `.kicad_pcb`):** Contienen el diseño físico (rutas de cobre, capas, dimensiones). Se usan para ajustar el tamaño de la placa o mover componentes.
* **Librerías de Símbolos/Huellas:** Archivos necesarios para que el software reconozca componentes específicos que no vienen por defecto.
* **Archivos de Proyecto:** El archivo maestro que vincula todos los anteriores.

---

## 🛠️ Herramientas de Desarrollo

Para interactuar con estos archivos, se requieren las siguientes herramientas:

| Software | Versión Recomendada | Función |
| --- | --- | --- |
| **KiCad EDA** | 6.0 o Superior | Visualización, edición de esquemáticos y diseño de la placa. |
| **Proteus** | 8.10+ | En caso de que se requiera simulación dinámica del comportamiento del firmware con el hardware. |
| **Gerber Viewer** | Cualquiera | Para inspeccionar los archivos finales antes de enviarlos a una fábrica (ej: JLCPCB, PCBWay). |

---

## 🚀 Guía Rápida para Colaboradores

1. **Clonación:** Descarga o clona el repositorio completo para no perder los enlaces de las librerías locales.
2. **Apertura:** Inicia **KiCad** y selecciona "Open Project", buscando el archivo `.pro` o `.kicad_pro` en esta carpeta.
3. **Modificación:** Si realizas cambios en el esquemático, recuerda usar la herramienta *Update PCB from Schematic* para sincronizar el diseño físico.
4. **Exportación:** Para fabricar, dirígete a `File > Fabrication Outputs > Gerbers`.
