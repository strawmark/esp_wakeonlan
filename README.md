
# ESP32 Wake-On-LAN Web Server

This project is designed to run a web server on a Wi-Fi-enabled ESP32 module, allowing you to send Wake-On-LAN signals to devices connected to the same network.

## Key Features

### Wake-On-LAN Packet Generation

The core of the project is sending UDP packets to wake up devices that support power-on via "magic packets" on the same physical LAN.

### REST API

|        Method | Endpoint                 | Description                                                 |
| ------------: | ------------------------ | ----------------------------------------------------------- |
|       **GET** | `/api/devices`           | Get the full list of devices                       |
|      **POST** | `/api/devices`           | AAdd a new device                                  |
|       **GET** | `/api/devices/{id}`      | Details of a single device                          |
| **PUT/PATCH** | `/api/devices/{id}`      | Update name and/or MAC address                               |
|    **DELETE** | `/api/devices/{id}`      | Remove a device                                         |
|      **POST** | `/api/devices/{id}/wake` | Trigger the wake signal for the device                              |
|       **GET** | `/api/wake/status`       | Real-time status of the wake signal transmission |

### Real-Time Communication

The server uses **Server-Sent Events** (SSE) to:
- Update the device list on the user interface.
- Report the transmission status of the wake signal.

### Persistent Storage

By using a `devices.json` file, you can preload a list of devices. Using the REST API allows you to modify this list, ensuring changes persist across ESP32 reboots.

### Web Interface

The user interface is a single-page website featuring a grid of available devices and a status indicator for wake signal transmission.

By using the "url" and "device-name" parameters in the URL, you can display a button to return to the requested URL and filter the device list. For example, using **Nginx**, you can have the web server respond if a service is unreachable because a PC is powered off.

Add these lines to your server configuration:

```txt title:"your-site.conf"
error_page 502 = @trigger_wol; 

location / {
    proxy_pass http://SERVER_IP:PORT;
}

location @trigger_wol {
    return 302 http://WAKEONLAN_URL/?url=$scheme://$host$request_uri&device-name=DEVICE_NAME;
}
```

Configure the Wake On LAN server location by adding:

```txt title:"wol_site.conf"
proxy_buffering off;

location / {
    proxy_pass http://WAKEONLAN_LOCAL_IP:80;
}
```

## Required Libraries

- **AsyncTCP** (3.4.8) by ESP32Async
- **ESP Async WebServer** (3.8.1) by ESP32Async
- **WakeOnLan** (1.1.7) by a7dm0
- **ArduinoJson** (7.4.3) by Benoit Blanchon
- **LittleFS** (built-in to Arduino ESP32)
- **WiFi** (built-in to Arduino ESP32)
- **WiFiUDP** (built-in to Arduino ESP32)

## Board manager e plugin

- **esp32** (2.0.17) by Espressif Systems
- [LittleFS](https://github.com/earlephilhower/arduino-littlefs-upload/releases)

## Setup

1. Install the LittleFS plugin for ArduinoIDE.
2. Edit `config.h` to set:
    - NET_SSID: Your Wi-Fi network name.
    - NET_PASSWORD: Your Wi-Fi network password.
    - WAKEONLAN_PORT: Port for wake packets (default: 9).
3. Configure your devices in `data/devices.json` using `devices.json.example` as a template.
4. From ArduinoIDE, compile and upload the esp_wakeonlan.ino sketch, then upload the contents of the data folder using the LittleFS plugin.
5. After startup, the ESP32 will obtain an IP address on your local network. Access the web server via a browser using its local IP.