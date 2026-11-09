/*
 * ESP32-CAM WiFi Repeater
 * Board: AI Thinker ESP32-CAM
 *
 * Connects to an upstream Wi-Fi network (STA) and provides a separate
 * Wi-Fi network (SoftAP). NAPT routes SoftAP client traffic upstream.
 *
 * Requires an Arduino-ESP32 core exposing esp_netif_napt_enable().
 */

#include <WiFi.h>
#include <esp_netif.h>
#include "include/secrets.h"

#ifndef UPSTREAM_WIFI_SSID
#error "Define UPSTREAM_WIFI_SSID in include/secrets.h"
#endif
#ifndef UPSTREAM_WIFI_PASSWORD
#error "Define UPSTREAM_WIFI_PASSWORD in include/secrets.h"
#endif
#ifndef REPEATER_WIFI_SSID
#error "Define REPEATER_WIFI_SSID in include/secrets.h"
#endif
#ifndef REPEATER_WIFI_PASSWORD
#error "Define REPEATER_WIFI_PASSWORD in include/secrets.h"
#endif

namespace {
constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint32_t STA_CONNECT_TIMEOUT_MS = 25000;
constexpr uint32_t RECONNECT_INTERVAL_MS = 10000;
constexpr uint8_t AP_CHANNEL = 1;
constexpr uint8_t AP_MAX_CLIENTS = 4;

uint32_t lastReconnectAttempt = 0;
bool natEnabled = false;

void printNetworkStatus() {
  Serial.println();
  Serial.println(F("===== WiFi Repeater Status ====="));
  Serial.printf("Upstream SSID: %s\n", UPSTREAM_WIFI_SSID);
  Serial.printf("STA connected: %s\n", WiFi.status() == WL_CONNECTED ? "yes" : "no");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("STA IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("STA RSSI: %d dBm\n", WiFi.RSSI());
  }
  Serial.printf("Repeater SSID: %s\n", REPEATER_WIFI_SSID);
  Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
  Serial.printf("NAPT: %s\n", natEnabled ? "enabled" : "disabled");
  Serial.println(F("==============================="));
}

bool enableNat() {
  // The default SoftAP netif key is WIFI_AP_DEF in Arduino-ESP32.
  esp_netif_t* apNetif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
  if (apNetif == nullptr) {
    Serial.println(F("[NAT] Could not find SoftAP network interface."));
    return false;
  }

  const esp_err_t result = esp_netif_napt_enable(apNetif);
  if (result != ESP_OK) {
    Serial.printf("[NAT] Enable failed: %s (%d)\n", esp_err_to_name(result), result);
    return false;
  }

  Serial.println(F("[NAT] NAPT enabled."));
  return true;
}

void startAccessPoint() {
  IPAddress apIp(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);

  if (!WiFi.softAPConfig(apIp, gateway, subnet)) {
    Serial.println(F("[AP] Failed to configure SoftAP IP."));
  }

  const bool started = WiFi.softAP(
    REPEATER_WIFI_SSID,
    REPEATER_WIFI_PASSWORD,
    AP_CHANNEL,
    false,
    AP_MAX_CLIENTS
  );

  if (started) {
    Serial.printf("[AP] Started: %s\n", REPEATER_WIFI_SSID);
    Serial.printf("[AP] Gateway: %s\n", WiFi.softAPIP().toString().c_str());
  } else {
    Serial.println(F("[AP] Failed to start SoftAP."));
  }
}

bool connectUpstream() {
  Serial.printf("[STA] Connecting to %s", UPSTREAM_WIFI_SSID);
  WiFi.begin(UPSTREAM_WIFI_SSID, UPSTREAM_WIFI_PASSWORD);

  const uint32_t startedAt = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startedAt < STA_CONNECT_TIMEOUT_MS) {
    delay(400);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[STA] Connection timed out. Will retry in background."));
    return false;
  }

  Serial.printf("[STA] Connected. IP: %s\n", WiFi.localIP().toString().c_str());
  return true;
}
} // namespace

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(500);
  Serial.println();
  Serial.println(F("\nESP32-CAM WiFi Repeater starting..."));

  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);

  // Start the AP first so local clients can connect while STA reconnects.
  startAccessPoint();

  const bool upstreamConnected = connectUpstream();
  if (upstreamConnected) {
    natEnabled = enableNat();
  }

  printNetworkStatus();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    natEnabled = false;
    const uint32_t now = millis();
    if (now - lastReconnectAttempt >= RECONNECT_INTERVAL_MS) {
      lastReconnectAttempt = now;
      Serial.println(F("[STA] Upstream disconnected; reconnecting..."));
      WiFi.disconnect(false, false);
      if (connectUpstream()) {
        natEnabled = enableNat();
        printNetworkStatus();
      }
    }
  }

  static uint32_t lastStatusPrint = 0;
  if (millis() - lastStatusPrint >= 30000) {
    lastStatusPrint = millis();
    printNetworkStatus();
  }

  delay(50);
}
