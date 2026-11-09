# ESP32-CAM WiFi Repeater

**Board:** AI Thinker ESP32-CAM  
**Upload method:** Arduino IDE + ESP32-CAM-MB  
**Network mode:** Wi-Fi Station + SoftAP (AP+STA) with NAT/NAPT

This project turns an AI Thinker ESP32-CAM into a small Wi-Fi repeater. The ESP32 connects to an existing Wi-Fi network as a station and creates a second Wi-Fi network for nearby devices. NAT is enabled on the SoftAP interface so connected clients can reach the internet through the upstream connection.

## Current project status

This repository is the initial project scaffold. Hardware-specific sensor integrations and MQTT/relay automation are planned as separate modules; they are not claimed as tested in this initial revision.

## Hardware

- AI Thinker ESP32-CAM
- ESP32-CAM-MB programmer
- Stable 5 V power supply (use a supply capable of handling Wi-Fi current peaks)

## Network settings

The project is configured for:

- Repeater SSID: `Ibrahim_Extender`
- Upstream SSID: configured in `include/secrets.h`
- Repeater password: configured in `include/secrets.h`

**Do not commit real Wi-Fi passwords or other secrets.** Copy `include/secrets.example.h` to `include/secrets.h` and fill in your local values. The real secrets file is ignored by Git.

## Arduino IDE setup

1. Install the ESP32 board package by Espressif Systems.
2. Select **AI Thinker ESP32-CAM** from Tools → Board.
3. Use the ESP32-CAM-MB to upload the sketch.
4. Copy `include/secrets.example.h` to `include/secrets.h` and enter the upstream Wi-Fi credentials and repeater password.
5. Open `ESP32-CAM-WiFi-Repeater.ino`, select the correct port, and upload.
6. Open Serial Monitor at **115200 baud**.

Use a recent Arduino-ESP32 core that provides the ESP-NETIF NAPT API. If your installed core does not expose `esp_netif_napt_enable`, update the ESP32 board package to a compatible release.

## Default repeater network

- SSID: `Ibrahim_Extender`
- Gateway: `192.168.4.1` (default SoftAP address)
- Security: WPA2-PSK

The upstream router assigns the ESP32 its station address. The repeater's SoftAP uses its own subnet; clients receive addresses from the ESP32 DHCP server.

## Important limitations

- This is a NAT repeater, not a transparent Layer-2 bridge.
- Throughput and range are limited by the ESP32-CAM radio, antenna, power supply, and Wi-Fi conditions.
- The ESP32-CAM camera is not used by the initial repeater firmware.
- Test with a low-risk network first. Do not expose the repeater password publicly.

## Repository layout

```
ESP32-CAM-WiFi-Repeater/
├── ESP32-CAM-WiFi-Repeater.ino
├── include/
│   └── secrets.example.h
├── .gitignore
└── README.md
```

## Troubleshooting

- **AP appears but there is no internet:** confirm the upstream SSID/password, verify the ESP32 obtained a station IP, and confirm NAPT was enabled successfully in Serial Monitor.
- **Repeated disconnects/restarts:** use a stable 5 V supply and short USB cable.
- **Upload fails:** select AI Thinker ESP32-CAM, check the COM port, and ensure the ESP32-CAM-MB is seated correctly.
- **NAPT API compile error:** update the Espressif ESP32 board package; NAPT API availability differs between Arduino-ESP32 versions.

## Roadmap

- [ ] Verify AP+STA and NAT on the target board/core version
- [ ] Add a lightweight status/configuration page
- [ ] Add optional MQTT telemetry
- [ ] Add optional sensor inputs and relay outputs
- [ ] Add persistent configuration and reconnect diagnostics

## License

MIT. See [LICENSE](LICENSE).
