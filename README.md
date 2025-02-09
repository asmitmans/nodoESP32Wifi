# Nodo IoT Base (ESP32)

Este proyecto implementa un **nodo IoT base** utilizando un **ESP32**, diseñado 
para leer datos de un sensor de temperatura y humedad (*DHT22*), enviar la 
información por **UART**, y controlar un **LED** según condiciones definidas. El 
firmware está desarrollado siguiendo buenas prácticas de programación en **ESP-IDF**.

---

## **Características Principales**

- **Lectura de Sensor (DHT22):** Temperatura y humedad ambiente.  
- **Comunicación UART:** Envío de datos a dispositivos externos.  
- **Control de LED:** Activación del LED si la temperatura supera los 30°C.  
- **Manejo Eficiente del Tiempo:** Basado en timers de FreeRTOS (sin bloqueos).

---

## **Requisitos**

- **Hardware:**  
  - ESP32 (módulo ESP-WROOM-32)  
  - Sensor DHT22 (GPIO 21)   
  - LEDs (GPIO 22)  
  - Resistencia pull-up para el DHT22 (4.7 kΩ recomendada)  
  - Adaptador USB-UART para la comunicación serie (si es necesario) (GPIO16 y 17) 

- **Software:**  
  - [ESP-IDF (v5.x)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html)  
  - VS Code o terminal con soporte para ESP-IDF  

---

## **Instalación y Configuración**

1. **Clonar el repositorio:**
   ```bash
   git clone https://github.com/usuario/nodo_base.git
   cd nodo_base
   ```

2. **Configurar el entorno de ESP-IDF:**
   ```bash
   . $IDF_PATH/export.sh
   ```

3. **Configurar opciones del proyecto (si es necesario):**
   ```bash
   idf.py menuconfig
   ```

4. **Compilar y flashear:**
   ```bash
   idf.py build
   idf.py flash
   idf.py monitor
   ```

---

## **Estructura del Proyecto**

```plaintext
nodo_base/
├── main/
│   ├── main.c               # Punto de entrada de la aplicación
│   ├── app_tasks.c          # Inicialización de tareas
│   └── app_tasks.h
└── components/
    ├── dht22/               # Lectura del sensor DHT22
    ├── uart_comm/           # Comunicación UART
    ├── led/                 # Control del LED
    └── sensor_manager/      # Lógica de negocio (lectura, procesamiento)
```

---

## **Funcionamiento del Firmware**

1. **Inicialización:**  
   - Configura el DHT22, UART y LED.  
   - Se inicia un timer que gestiona la lectura del sensor cada 10 segundos.

2. **Lectura de Datos:**  
   - Se obtiene la temperatura y la humedad del DHT22.  
   - Los datos se envían por UART a través de `uart_comm`.

3. **Control del LED:**  
   - Si la temperatura supera los **30°C**, se enciende el LED verde.  
   - En caso contrario, el LED permanece apagado.

4. **Manejo Eficiente del Tiempo:**  
   - Se utiliza un **timer de FreeRTOS** en lugar de `vTaskDelay` para evitar bloqueos innecesarios.

---

## **Configuración UART (para monitoreo)**

- **Baud Rate:** 115200  
- **Data Bits:** 8  
- **Paridad:** Ninguna  
- **Stop Bits:** 1  
- **Flow Control:** Desactivado  

---

## **Notas Técnicas Importantes**

- **Timers de FreeRTOS:** Se usa un timer para gestionar la lectura periódica del sensor 
  sin bloquear tareas.  
- **Modularización:** Cada funcionalidad está separada en componentes independientes para 
  facilitar el mantenimiento y la escalabilidad.  
- **Uso de `CMakeLists.txt`:** La configuración de dependencias se define a nivel de cada 
  componente, siguiendo buenas prácticas de ESP-IDF.

---

## **Futuras Mejoras (Opcionales)**

- Implementación de conectividad Wi-Fi para enviar datos a la nube.  
- Añadir manejo de errores avanzado y reconexión automática del sensor.  
- Expansión para soportar múltiples sensores o protocolos de comunicación.

---

## **Autores**

- **Desarrollador:** [asmitmans]  
- **Colaboración:** Proyecto desarrollado siguiendo buenas prácticas de firmware para 
  sistemas embebidos en ESP32.

---# nodoESP32Wifi
