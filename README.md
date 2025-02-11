# Nodo IoT con ESP32 para Monitoreo de Temperatura y Humedad

Este proyecto implementa un nodo IoT utilizando un ESP32 para leer datos de un 
sensor DHT22, almacenar datos fallidos en NVS y enviarlos a un servidor HTTP. 
Está desarrollado con ESP-IDF y es configurable mediante `menuconfig`.

## Características Principales

- **Lectura de Sensor DHT22**: Obtiene temperatura y humedad.
- **Almacenamiento en NVS**: Guarda datos fallidos para reintentos posteriores.
- **Envío HTTP POST**: Envía datos a un servidor remoto.
- **Configuración Dinámica**: Parámetros ajustables mediante `menuconfig`.

## Estructura del Proyecto

```
nodoESP32Wifi/
├── components/
│   ├── dht22/
│   ├── http_client/
│   ├── nvs_storage/
│   ├── sensor_manager/
│   ├── tasks/
│   └── wifi_manager/
├── main/
│   ├── main.c
│   └── ...
├── CMakeLists.txt
└── README.md
```

## Requisitos

### Hardware

- **ESP32 DevKit**
- **Sensor DHT22**
- **Resistencia de 10kΩ** (si el módulo DHT22 no la incluye)
- **Cables de conexión**
- **Fuente de alimentación adecuada**

### Software
- **[ESP-IDF (v5.x)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html)  **
- **VS Code o terminal con soporte para ESP-IDF**

## Instalación y Configuración

1. **Clonar el Repositorio**

   ```bash
   git clone https://github.com/asmitmans/nodoESP32Wifi.git
   cd nodoESP32Wifi
   ```

2. **Configurar el Entorno ESP-IDF**

   Asegúrese de tener ESP-IDF instalado y configurado. 
   Siga las instrucciones oficiales: 
   [ESP-IDF Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
   
3. **Configurar el Proyecto**

   Ejecute `menuconfig` para establecer los parámetros necesarios:

   ```bash
   idf.py menuconfig
   ```

   - **Wi-Fi SSID y Contraseña**: Navegue a `Wi-Fi Configuration` y establezca su SSID y contraseña.
   - **URL del Servidor HTTP POST**: Vaya a `HTTP Configuration` y configure la URL de destino.
   - **Otros Parámetros**: Ajuste los reintentos y tiempos de espera según sea necesario.

4. **Compilar y Flashear**

   ```bash
   idf.py build
   idf.py flash
   ```

5. **Monitorear la Salida Serial**

   ```bash
   idf.py monitor
   ```

## Conexiones de Hardware

Conecte el sensor DHT22 al ESP32 según la siguiente tabla:

| Pin DHT22 | Pin ESP32 |
|-----------|-----------|
| VCC       | 3.3V      |
| GND       | GND       |
| DATA      | GPIO21    |


## API de Prueba

Para probar el envío de datos, puede utilizar una API de prueba como 
[JSONPlaceholder](https://jsonplaceholder.typicode.com/)
Asegúrese de configurar la URL de la API en `menuconfig` antes de compilar.

## Parámetros de Configuración en `menuconfig`

Es esencial configurar los siguientes parámetros antes de ejecutar el proyecto:

- **Wi-Fi SSID y Contraseña**: Para conectar el ESP32 a su red inalámbrica.
- **URL del Servidor HTTP POST**: Dirección del servidor que recibirá los datos.
- **Reintentos y Tiempos de Espera**: Opcionalmente, ajuste estos valores según sus necesidades.

## Consideraciones Adicionales

- **NVS (Non-Volatile Storage)**: El proyecto utiliza NVS para almacenar datos que no 
  pudieron enviarse, asegurando que no se pierdan lecturas en caso de fallos de 
  conexión.
- **Manejo de Errores**: Se implementan mecanismos para reintentar conexiones y envíos 
  en caso de fallos temporales.

---