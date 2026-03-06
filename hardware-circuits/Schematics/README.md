

# ⚡ Power Supply & Motor Control Simulations - Degree Project

## 📝 Descripción

Esta carpeta alberga los archivos de simulación y esquemáticos lógicos enfocados en las dos etapas críticas de potencia del proyecto:

1. **Etapa de Alimentación:** Circuitos diseñados para la regulación, filtrado y protección de voltaje, asegurando un suministro estable para los componentes sensibles.
2. **Etapa de Manejo de Motores:** Diseño de controladores (drivers) encargados de gestionar la potencia del actuador del sistema.

La finalidad de este directorio es proporcionar un entorno de pruebas virtual donde se han validado corrientes, picos de voltaje y respuesta térmica antes de proceder al diseño de la PCB final.

---

## ⚙️ Uso de las Simulaciones

Los archivos y capturas aquí contenidos son útiles para:

* **Análisis de Señal:** Observar el comportamiento de la modulación (PWM) enviada a los motores a través de osciloscopios virtuales.
* **Cálculo de Consumo:** Validar que los reguladores de voltaje soportan la carga máxima requerida por los motores sin caídas de tensión.
* **Pruebas de Estrés:** Simular fallos o variaciones en la entrada de energía para verificar la robustez de los circuitos de protección.
* **Referencia de Depuración:** Si el hardware físico falla, estos esquemáticos sirven como "punto de oro" para comparar voltajes teóricos vs. reales.

---

## 🛠️ Herramientas de Desarrollo

Para ejecutar o modificar estas simulaciones, se utilizaron los siguientes softwares:

| Software | Función | Archivos Relacionados |
| --- | --- | --- |
| **Proteus Design Suite** | Simulación interactiva de circuitos y microcontroladores. | `.pdsprj` |
| **LTspice** | Análisis profundo de transitorios y ruido en la etapa de potencia. | `.asc` |
| **KiCad (Schematic Editor)** | Dibujo técnico y exportación de netlists para la PCB. | `.kicad_sch` |
| **EveryCircuit / Multisim** | (Opcional) Verificación rápida de leyes de Kirchhoff y flujos de corriente. | Capturas de pantalla |

---

## ⚠️ Notas Técnicas

* **Configuración de Motores:** Las simulaciones están configuradas para motores de [12V] con una carga nominal de [600mA].
* **Reguladores:** Se ha prestado especial atención a la disipación de calor en los componentes de potencia.
