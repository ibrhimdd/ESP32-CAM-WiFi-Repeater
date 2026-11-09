/*
 * ESP32-CAM WiFi Repeater with browser-based setup
 * Board: AI Thinker ESP32-CAM
 *
 * - Setup/configuration page at http://192.168.4.1/
 * - Stores upstream and repeater credentials in Preferences (NVS)
 * - Scans nearby Wi-Fi networks
 * - AP+STA with NAPT routing
 *
 * Requires an Arduino-ESP32 core exposing esp_netif_napt_enable().
 */

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <esp_netif.h>

namespace {
constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint32_t RECONNECT_INTERVAL_MS = 10000;
constexpr uint32_t CONNECT_TIMEOUT_MS = 18000;
constexpr uint8_t AP_CHANNEL = 1;
constexpr uint8_t AP_MAX_CLIENTS = 4;
constexpr char DEFAULT_AP_SSID[] = "Ibrahim_Extender";
constexpr char DEFAULT_AP_PASSWORD[] = "Ibrahim123"; // Change this in the setup page.
constexpr char AP_IP_TEXT[] = "192.168.4.1";

WebServer server(80);
Preferences preferences;

String upstreamSsid;
String upstreamPassword;
String repeaterSsid;
String repeaterPassword;
bool natEnabled = false;
bool configSaved = false;
uint32_t lastReconnectAttempt = 0;

String htmlEscape(const String& value) {
  String out;
  out.reserve(value.length() + 8);
  for (size_t i = 0; i < value.length(); ++i) {
    const char c = value[i];
    if (c == '&') out += F("&amp;");
    else if (c == '<') out += F("&lt;");
    else if (c == '>') out += F("&gt;");
    else if (c == '"') out += F("&quot;");
    else if (c == '\'') out += F("&#39;");
    else out += c;
  }
  return out;
}

void loadConfig() {
  preferences.begin("wifi-repeater", true);
  upstreamSsid = preferences.getString("sta_ssid", "");
  upstreamPassword = preferences.getString("sta_pass", "");
  repeaterSsid = preferences.getString("ap_ssid", DEFAULT_AP_SSID);
  repeaterPassword = preferences.getString("ap_pass", DEFAULT_AP_PASSWORD);
  preferences.end();

  if (repeaterSsid.isEmpty()) repeaterSsid = DEFAULT_AP_SSID;
  if (repeaterPassword.length() < 8) repeaterPassword = DEFAULT_AP_PASSWORD;
}

void saveConfig() {
  preferences.begin("wifi-repeater", false);
  preferences.putString("sta_ssid", upstreamSsid);
  preferences.putString("sta_pass", upstreamPassword);
  preferences.putString("ap_ssid", repeaterSsid);
  preferences.putString("ap_pass", repeaterPassword);
  preferences.end();
  configSaved = true;
}

bool enableNat() {
  esp_netif_t* apNetif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
  if (apNetif == nullptr) {
    Serial.println(F("[NAT] SoftAP interface not found."));
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
  WiFi.softAPdisconnect(true);
  WiFi.softAPConfig(
    IPAddress(192, 168, 4, 1),
    IPAddress(192, 168, 4, 1),
    IPAddress(255, 255, 255, 0)
  );
  const bool ok = WiFi.softAP(
    repeaterSsid.c_str(),
    repeaterPassword.c_str(),
    AP_CHANNEL,
    false,
    AP_MAX_CLIENTS
  );
  Serial.printf("[AP] %s: %s (%s)\n", ok ? "Started" : "Failed",
                repeaterSsid.c_str(), WiFi.softAPIP().toString().c_str());
}

bool connectUpstream() {
  if (upstreamSsid.isEmpty()) {
    Serial.println(F("[STA] No upstream network configured."));
    return false;
  }

  Serial.printf("[STA] Connecting to %s", upstreamSsid.c_str());
  WiFi.disconnect(false, false);
  WiFi.begin(upstreamSsid.c_str(), upstreamPassword.c_str());

  const uint32_t startedAt = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startedAt < CONNECT_TIMEOUT_MS) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[STA] Connection failed; retry will happen in background."));
    return false;
  }

  Serial.printf("[STA] Connected. IP: %s, RSSI: %d dBm\n",
                WiFi.localIP().toString().c_str(), WiFi.RSSI());
  natEnabled = enableNat();
  return natEnabled;
}

String pageHtml(const String& notice = "") {
  String page;
  page.reserve(6500);
  page += F("<!doctype html><html lang='ar' dir='rtl'><head><meta charset='utf-8'>");
  page += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  page += F("<title>Ibrahim WiFi Repeater</title><style>");
  page += F("*{box-sizing:border-box}body{margin:0;background:#edf6f9;color:#17323a;font-family:Arial,Tahoma,sans-serif;padding:18px}");
  page += F(".wrap{max-width:620px;margin:20px auto}.hero{background:#075565;color:white;padding:22px;border-radius:18px;margin-bottom:16px}");
  page += F(".card{background:white;border:1px solid #d6e5e8;border-radius:16px;padding:18px;margin:14px 0;box-shadow:0 8px 24px #17323a0d}");
  page += F("h1{font-size:23px;margin:0 0 8px}h2{font-size:18px;margin:0 0 14px}p{line-height:1.7;margin:6px 0}");
  page += F("label{display:block;font-weight:bold;margin:13px 0 6px}input,select{width:100%;padding:13px;border:1px solid #b8cdd2;border-radius:10px;font-size:16px;background:white;color:#17323a}");
  page += F("button{border:0;border-radius:10px;background:#007c83;color:white;padding:13px 16px;font-size:15px;font-weight:bold;cursor:pointer;margin-top:12px;width:100%}");
  page += F(".secondary{background:#e1f1f0;color:#075565}.status{padding:12px;border-radius:10px;background:#e7f5ed;color:#17643b;margin-top:10px}.warn{background:#fff3df;color:#79520b}");
  page += F(".muted{font-size:13px;color:#60777d}.row{display:flex;gap:8px}.row button{flex:1}.network{padding:10px;border-bottom:1px solid #edf1f2;cursor:pointer}");
  page += F("</style></head><body><main class='wrap'><header class='hero'><h1>Ibrahim WiFi Repeater</h1><p>إعداد المقوي وإدارة اتصال الإنترنت</p></header>");

  if (notice.length()) {
    page += F("<div class='status'>");
    page += htmlEscape(notice);
    page += F("</div>");
  }

  page += F("<section class='card'><h2>حالة الاتصال</h2><p>شبكة المقوي: <b>");
  page += htmlEscape(repeaterSsid);
  page += F("</b></p><p>عنوان صفحة الإعدادات: <b>http://");
  page += AP_IP_TEXT;
  page += F("</b></p><p>اتصال الإنترنت: <b>");
  page += (WiFi.status() == WL_CONNECTED ? F("متصل") : F("غير متصل"));
  page += F("</b></p><p class='muted'>");
  if (WiFi.status() == WL_CONNECTED) {
    page += F("الشبكة المصدر: ");
    page += htmlEscape(upstreamSsid);
    page += F(" — قوة الإشارة: ");
    page += String(WiFi.RSSI());
    page += F(" dBm");
  } else {
    page += F("أدخل بيانات شبكة الإنترنت بالأسفل للاتصال بها.");
  }
  page += F("</p></section>");

  page += F("<form method='POST' action='/save'><section class='card'><h2>شبكة الإنترنت الأساسية</h2>");
  page += F("<label for='ssid'>اسم الشبكة (SSID)</label><input id='ssid' name='ssid' maxlength='32' required value='");
  page += htmlEscape(upstreamSsid);
  page += F("' placeholder='اسم شبكة الراوتر'>");
  page += F("<div class='row'><button class='secondary' type='button' onclick='scan()'>البحث عن الشبكات</button></div><div id='networks'></div>");
  page += F("<label for='pass'>كلمة مرور الشبكة</label><input id='pass' name='pass' type='password' maxlength='63' placeholder='اتركها فارغة إذا لم تتغير'>");
  page += F("<p class='muted'>لأسباب أمنية لا نعرض كلمة المرور المحفوظة.</p></section>");

  page += F("<section class='card'><h2>شبكة المقوي</h2><label for='apssid'>اسم شبكة المقوي</label><input id='apssid' name='apssid' maxlength='32' required value='");
  page += htmlEscape(repeaterSsid);
  page += F("'><label for='appass'>كلمة مرور شبكة المقوي</label><input id='appass' name='appass' type='password' minlength='8' maxlength='63' placeholder='8 أحرف على الأقل — اتركها كما هي إذا لم ترد تغييرها'>");
  page += F("<p class='muted'>كلمة المرور الحالية لا تظهر في الصفحة. أدخل كلمة جديدة فقط عند الرغبة في تغييرها.</p>");
  page += F("<button type='submit'>حفظ الإعدادات والاتصال</button></section></form>");
  page += F("<section class='card'><h2>ملاحظات</h2><p class='muted'>بعد تغيير اسم أو كلمة مرور شبكة المقوي، قد ينقطع هاتفك عنها. اتصل بالشبكة الجديدة ثم افتح 192.168.4.1 مرة أخرى.</p>");
  page += F("<p class='muted'>لو لم تنجح بيانات الراوتر، ستظل شبكة المقوي متاحة لتعديلها.</p></section>");
  page += F("</main><script>async function scan(){const box=document.getElementById('networks');box.innerHTML='جارٍ البحث...';try{const r=await fetch('/scan');const a=await r.json();box.innerHTML='';if(!a.length){box.textContent='لم يتم العثور على شبكات.';return;}a.forEach(n=>{const d=document.createElement('div');d.className='network';d.textContent=n.ssid+' ('+n.rssi+' dBm)'+(n.secure?' 🔒':'');d.onclick=()=>document.getElementById('ssid').value=n.ssid;box.appendChild(d);});}catch(e){box.textContent='تعذر البحث. حاول مرة أخرى.';}}</script>");
  page += F("</body></html>");
  return page;
}

void handleRoot() {
  server.send(200, "text/html; charset=utf-8", pageHtml());
}

void handleScan() {
  const int count = WiFi.scanNetworks(false, true);
  String json = "[";
  bool first = true;
  for (int i = 0; i < count; ++i) {
    const String ssid = WiFi.SSID(i);
    if (ssid.isEmpty()) continue;
    if (!first) json += ',';
    first = false;
    json += "{\"ssid\":\"";
    String escaped = ssid;
    escaped.replace("\\", "\\\\");
    escaped.replace("\"", "\\\"");
    json += escaped;
    json += "\",\"rssi\":";
    json += String(WiFi.RSSI(i));
    json += ",\"secure\":";
    json += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "false" : "true");
    json += "}";
  }
  json += "]";
  WiFi.scanDelete();
  server.send(200, "application/json; charset=utf-8", json);
}

void handleSave() {
  String newStaSsid = server.arg("ssid");
  String newStaPassword = server.arg("pass");
  String newApSsid = server.arg("apssid");
  String newApPassword = server.arg("appass");

  newStaSsid.trim();
  newApSsid.trim();

  if (newStaSsid.isEmpty() || newStaSsid.length() > 32 ||
      newApSsid.isEmpty() || newApSsid.length() > 32) {
    server.send(400, "text/html; charset=utf-8", pageHtml("اسم الشبكة غير صحيح."));
    return;
  }

  if (newApPassword.isEmpty()) {
    newApPassword = repeaterPassword;
  } else if (newApPassword.length() < 8 || newApPassword.length() > 63) {
    server.send(400, "text/html; charset=utf-8", pageHtml("كلمة مرور شبكة المقوي يجب أن تكون من 8 إلى 63 حرفًا."));
    return;
  }

  if (newStaPassword.isEmpty() && newStaSsid != upstreamSsid) {
    server.send(400, "text/html; charset=utf-8", pageHtml("أدخل كلمة مرور شبكة الإنترنت الجديدة."));
    return;
  }
  if (newStaPassword.isEmpty()) newStaPassword = upstreamPassword;

  upstreamSsid = newStaSsid;
  upstreamPassword = newStaPassword;
  repeaterSsid = newApSsid;
  const bool apCredentialsChanged = repeaterPassword != newApPassword;
  repeaterPassword = newApPassword;
  saveConfig();

  server.send(200, "text/html; charset=utf-8",
              pageHtml("تم حفظ الإعدادات. جارٍ إعادة تشغيل اتصال الشبكة..."));
  delay(900);

  if (apCredentialsChanged || WiFi.softAPSSID() != repeaterSsid) {
    startAccessPoint();
  }
  natEnabled = false;
  connectUpstream();
}

void handleNotFound() {
  server.sendHeader("Location", String("http://") + AP_IP_TEXT + "/", true);
  server.send(302, "text/plain", "");
}

void startWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/scan", HTTP_GET, handleScan);
  server.on("/save", HTTP_POST, handleSave);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println(F("[WEB] Setup page: http://192.168.4.1/"));
}

void printStatus() {
  Serial.printf("[STATUS] AP=%s, STA=%s, NAT=%s\n",
                repeaterSsid.c_str(),
                WiFi.status() == WL_CONNECTED ? "connected" : "disconnected",
                natEnabled ? "enabled" : "disabled");
}
} // namespace

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(500);
  Serial.println(F("\nESP32-CAM WiFi Repeater starting..."));

  loadConfig();
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);

  startAccessPoint();
  startWebServer();

  if (!upstreamSsid.isEmpty()) connectUpstream();
  else Serial.println(F("[SETUP] Connect to the repeater and open http://192.168.4.1/"));
  printStatus();
}

void loop() {
  server.handleClient();

  if (WiFi.status() != WL_CONNECTED &&
      !upstreamSsid.isEmpty() &&
      millis() - lastReconnectAttempt >= RECONNECT_INTERVAL_MS) {
    lastReconnectAttempt = millis();
    Serial.println(F("[STA] Reconnecting to saved upstream network..."));
    connectUpstream();
  }

  static uint32_t lastStatusPrint = 0;
  if (millis() - lastStatusPrint >= 30000) {
    lastStatusPrint = millis();
    printStatus();
  }
  delay(2);
}
