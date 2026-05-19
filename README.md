# 🖥️ AtheniumOS - Sistema Embebido para ESP32

Sistema interactivo para ESP32 con pantalla táctil ILI9341, sensor DHT11, lector de tarjetas SD y un juego tipo Snake incluido.

## 📋 Características principales

- **Menú interactivo** con botones táctiles
- **Sensor DHT11** - Monitoreo de temperatura y humedad
- **Lector SD** - Navegación de archivos y notas de texto
- **Juego Snake** - Juego arcade completamente funcional
- **Interfaz visual** - Tema "Midnight Emerald" con retroalimentación auditiva y visual

## 🛠️ Hardware requerido

| Componente       | Pin GPIO | Descripción                          |
|-----------------|----------|--------------------------------------|
| ESP32           | -        | Microcontrolador principal           |
| ILI9341         | 15, 2, 4 | Pantalla TFT 320x240 (CS, DC, RST)  |
| XPT2046         | 21, 22   | Pantalla táctil (CS, IRQ)           |
| Módulo SD       | 5        | Lector de tarjetas microSD           |
| DHT11           | 16       | Sensor de temperatura y humedad      |
| Buzzer pasivo   | 27       | Salida de audio                      |
| LED             | 17       | Indicador de estado                  |

## 🎮 Funcionalidades

### 📁 Archivos SD
- Navegación de directorios
- Visualización de archivos
- Lector de notas (`notes.txt`)

### 🌡️ Sensor DHT11
- Medición de temperatura (°C)
- Medición de humedad (%)
- Actualización en tiempo real

### 🐍 Juego Snake
- Controles táctiles (D-pad virtual)
- Sistema de puntuación
- Efectos visuales y de sonido
- Mensaje de "Game Over" personalizado

### ℹ️ Información del sistema
- Detalles del hardware
- Estado de periféricos
- Configuración de pines




