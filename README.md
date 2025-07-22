# ESP32 Wake-on-LAN Web Server

This project starts a web server hosting a simple webpage that allows users to send a Wake-on-LAN signal to devices connected to the same Wi-Fi network.
- The `config.h` file contains hardcoded values for SSID, password, and the MAC addresses of the devices to wake. These settings must be configured before flashing the code to the ESP32 board.
- The `webpages.h` file defines the constant string representing the HTML content served by the web server. The original HTML source is located in the `web/` directory.
- To ensure external accessibility, set a static lease for the ESP32 on your router and configure a port forwarding rule from a WAN port to port 80 on the ESP32.
