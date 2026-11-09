# ESP32-CAM WiFi Repeater

**Board:** AI Thinker ESP32-CAM  
**Upload method:** Arduino IDE + ESP32-CAM-MB  
**Network mode:** Wi-Fi Station + SoftAP (AP+STA) with NAT/NAPT

This project connects the ESP32-CAM to an existing Wi-Fi network and shares that connection through its own Wi-Fi access point. Upstream and repeater credentials are configured from a lightweight browser page and stored in ESP32 NVS using the Preferences library.

## First-time setup

1. Install the Espressif ESP32 board package in Arduino IDE.
2. Select **AI Thinker ESP32-CAM** from Tools → Board.
3. Open ESP32-CAM-WiFi-Repeater.ino.
4. Connect the board through ESP32-CAM-MB, select its COM port, and upload.
5. On your phone, connect to the default Wi-Fi network:
   - **SSID:** Ibrahim_Extender
   - **Password:** Ibrahim123
6. Open http://192.168.4.1/ in your browser.
7. Use **البحث عن الشبكات** to scan nearby Wi-Fi networks, select one, enter its password, and save.
8. Change the repeater network name/password from the same page if desired.

> **Security:** The initial repeater password is Ibrahim123. Change it immediately from the setup page. Do not share the setup network password publicly.

## Configuration page

The page at http://192.168.4.1/ provides:

- Nearby Wi-Fi scan with signal strength.
- Upstream SSID and password configuration.
- Repeater SSID and password configuration.
- Connection status and upstream signal strength.
- Persistent settings saved in NVS, so credentials survive power loss.

The saved passwords are not displayed in the page. To change the upstream password, enter the new password. If you keep the same upstream SSID and leave its password field empty, the saved password is retained. The repeater password field can also be left empty to keep the current password.

If the repeater SSID/password is changed, reconnect your phone to the new repeater network. The setup page remains at http://192.168.4.1/.

## Hardware

- AI Thinker ESP32-CAM
- ESP32-CAM-MB programmer
- Stable 5 V power supply capable of handling Wi-Fi current peaks

## Arduino IDE requirements

- Espressif Arduino-ESP32 board package.
- A core version that exposes esp_netif_napt_enable() and the ESP-NETIF NAPT API.
- Built-in libraries: WiFi, WebServer, Preferences, and ESP-NETIF.

If compilation reports that esp_netif_napt_enable is unavailable, install a compatible Arduino-ESP32 core version.

## Network details and limitations

- Repeater default gateway: 192.168.4.1.
- The firmware uses NAT/NAPT; it is **not** a transparent Layer-2 bridge.
- The ESP32-CAM radio is half-duplex for AP+STA operation, so throughput and range are limited.
- The camera is not used by this firmware.
- The setup page is served over HTTP and is intended for use on the local repeater network. Avoid entering credentials while connected to an untrusted/public network.

## Troubleshooting

- **Repeater network is visible but internet does not work:** confirm the upstream credentials and check Serial Monitor at 115200 baud for STA connection and NAPT messages.
- **Cannot open the setup page:** connect to the repeater Wi-Fi and browse directly to http://192.168.4.1/.
- **Changed repeater credentials:** reconnect your phone using the new SSID/password.
- **Repeated disconnects or restarts:** use a stable 5 V supply and short USB cable.
- **Upload fails:** select AI Thinker ESP32-CAM, check the COM port, and ensure the ESP32-CAM-MB is seated correctly.

## Roadmap

- [x] Browser-based Wi-Fi configuration page
- [x] Nearby Wi-Fi scanning
- [x] Persistent configuration in NVS
- [x] Automatic upstream reconnect attempts
- [ ] Verify AP+STA and NAT on the target board/core version
- [ ] Add optional MQTT telemetry
- [ ] Add optional sensor inputs and relay outputs

## License

MIT. See LICENSE.
