/*
  ESP32 IR Obstacle Monitor
  FC-51 on GPIO21 + Wi-Fi web dashboard (raw WiFiServer, no external libraries)

  Wiring:  FC-51 VCC -> 3V3,  GND -> GND,  OUT -> GPIO21
  Routes:  /                 dashboard page
           /api/status       JSON status
           /api/sensor/on    enable sensor input
           /api/sensor/off   disable sensor input
*/

#include <WiFi.h>

// ---------- Configuration ----------
const char* ssid     = "realme P3";
const char* password = "mathi@011";

const int SENSOR_PIN = 21;                    // FC-51 OUT (LOW = obstacle)
const int LED_PIN    = 2;                     // Onboard LED mirrors detection
const unsigned long DEBOUNCE_MS       = 40;   // Signal must be stable this long
const unsigned long CLIENT_TIMEOUT_MS = 400;  // Max wait for a browser request

WiFiServer server(80);

// ---------- Sensor state ----------
bool sensorEnabled = true;
bool obstacle = false;                        // Debounced result
bool lastRaw = false;
unsigned long lastChangeMs = 0;
unsigned long detectionCount = 0;
unsigned long lastDetectionMs = 0;
bool hasDetection = false;

// ---------- Web page ----------
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="theme-color" content="#0a0f1e">
<title>IR Obstacle Monitor</title>
<style>
:root{--bg:#0a0f1e;--card:rgba(255,255,255,.05);--line:rgba(255,255,255,.09);--text:#e8ecf6;--muted:#8f9ab3;--c:#6b7590;--glow:rgba(107,117,144,.18)}
body[data-state=clear]{--c:#2fd884;--glow:rgba(47,216,132,.28)}
body[data-state=alert]{--c:#ff5a5f;--glow:rgba(255,90,95,.38)}
body[data-state=off]{--c:#6b7590;--glow:rgba(107,117,144,.18)}
*{box-sizing:border-box;margin:0}
body{min-height:100vh;display:flex;justify-content:center;padding:24px 16px 40px;color:var(--text);
 font-family:system-ui,-apple-system,"Segoe UI",Roboto,Helvetica,Arial,sans-serif;
 background:radial-gradient(900px 520px at 50% -10%,var(--glow),transparent 70%),var(--bg)}
.app{width:100%;max-width:440px}
header{display:flex;justify-content:space-between;align-items:center;margin-bottom:22px}
h1{font-size:1.15rem;font-weight:650}
.sub{font-size:.78rem;color:var(--muted);margin-top:3px}
.pill{display:flex;align-items:center;gap:7px;font-size:.75rem;color:var(--muted);padding:6px 11px;border:1px solid var(--line);border-radius:99px;background:var(--card)}
.dot{width:8px;height:8px;border-radius:50%;background:#6b7590}
.pill.on .dot{background:#2fd884;box-shadow:0 0 8px #2fd884}
.pill.err .dot{background:#ff5a5f}
.card{background:var(--card);border:1px solid var(--line);border-radius:20px;padding:22px;backdrop-filter:blur(8px)}
.hero{text-align:center;padding:34px 22px 28px}
.orb{position:relative;width:168px;height:168px;margin:0 auto 24px;border-radius:50%;display:grid;place-items:center;color:var(--c);
 background:radial-gradient(circle at 50% 38%,rgba(255,255,255,.08),rgba(255,255,255,.02));
 border:2px solid var(--c);box-shadow:0 0 40px var(--glow),inset 0 0 30px var(--glow);transition:border-color .35s,color .35s,box-shadow .35s}
.orb svg{width:62px;height:62px;stroke:currentColor;fill:none;stroke-width:1.7;stroke-linecap:round;stroke-linejoin:round}
.orb::before,.orb::after{content:"";position:absolute;inset:-2px;border-radius:50%;border:2px solid var(--c);opacity:0}
body[data-state=clear] .orb::before{animation:ping 3s ease-out infinite}
body[data-state=alert] .orb::before{animation:ping 1s ease-out infinite}
body[data-state=alert] .orb::after{animation:ping 1s ease-out .5s infinite}
body[data-state=alert] .orb{animation:pop .35s ease-out}
@keyframes ping{0%{transform:scale(1);opacity:.55}100%{transform:scale(1.55);opacity:0}}
@keyframes pop{0%{transform:scale(.94)}60%{transform:scale(1.05)}100%{transform:scale(1)}}
.status{font-size:1.6rem;font-weight:700;color:var(--c);transition:color .35s}
.hint{margin-top:6px;font-size:.85rem;color:var(--muted);min-height:1.2em}
.ctl{display:flex;justify-content:space-between;align-items:center;gap:16px;margin-top:14px}
.ctl h2{font-size:.98rem;font-weight:600}
.ctl p{font-size:.8rem;color:var(--muted);margin-top:3px;line-height:1.35}
.switch{position:relative;flex:none;width:56px;height:32px}
.switch input{position:absolute;inset:0;width:100%;height:100%;margin:0;opacity:0;cursor:pointer;z-index:1}
.track{position:absolute;inset:0;border-radius:99px;background:#2a3249;transition:background .25s}
.track::after{content:"";position:absolute;top:4px;left:4px;width:24px;height:24px;border-radius:50%;background:#fff;box-shadow:0 2px 6px rgba(0,0,0,.4);transition:transform .25s}
.switch input:checked+.track{background:#2fd884}
.switch input:checked+.track::after{transform:translateX(24px)}
.switch input:focus-visible+.track{outline:2px solid #fff;outline-offset:3px}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-top:14px}
.stat{padding:16px 18px}
.stat span{display:block;font-size:.7rem;text-transform:uppercase;letter-spacing:.08em;color:var(--muted)}
.stat b{display:block;margin-top:6px;font-size:1.3rem;font-weight:650;font-variant-numeric:tabular-nums;white-space:nowrap}
.stat:last-child{grid-column:1/-1}
footer{margin-top:20px;text-align:center;font-size:.72rem;color:var(--muted)}
@media(prefers-reduced-motion:reduce){*{animation:none!important;transition:none!important}}
</style></head>
<body data-state="off">
<div class="app">
  <header>
    <div><h1>IR Obstacle Monitor</h1><div class="sub">ESP32 &middot; FC-51 &middot; GPIO21</div></div>
    <div class="pill" id="conn"><i class="dot"></i><span id="connTxt">Connecting</span></div>
  </header>

  <section class="card hero">
    <div class="orb">
      <svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="2"/><path d="M4.93 19.07a10 10 0 0 1 0-14.14"/><path d="M7.76 16.24a6 6 0 0 1 0-8.48"/><path d="M16.24 7.76a6 6 0 0 1 0 8.48"/><path d="M19.07 4.93a10 10 0 0 1 0 14.14"/></svg>
    </div>
    <div class="status" id="status">Connecting&hellip;</div>
    <div class="hint" id="hint"></div>
  </section>

  <section class="card ctl">
    <div>
      <h2>Sensor input</h2>
      <p>Turn off to ignore the FC-51 and stop counting detections.</p>
    </div>
    <label class="switch"><input type="checkbox" id="toggle" aria-label="Enable sensor input"><span class="track"></span></label>
  </section>

  <section class="grid">
    <div class="card stat"><span>Detections</span><b id="count">0</b></div>
    <div class="card stat"><span>Last detected</span><b id="last">&mdash;</b></div>
    <div class="card stat"><span>Uptime</span><b id="up">&mdash;</b></div>
  </section>

  <footer>Live &middot; refreshes every 0.5 s</footer>
</div>

<script>
const $=i=>document.getElementById(i);
const TITLES={clear:'Path clear',alert:'Obstacle detected',off:'Sensor off'};
const HINTS={clear:'Nothing in front of the sensor',alert:'Something is close to the sensor',off:'Sensor input is turned off'};
let inflight=false,fails=0,busy=false;

function dur(s){
  if(s<0)return'\u2014';
  if(s<60)return s+'s';
  const m=Math.floor(s/60);
  if(m<60)return m+'m '+(s%60)+'s';
  return Math.floor(m/60)+'h '+(m%60)+'m';
}
function conn(ok){
  $('conn').className='pill '+(ok?'on':'err');
  $('connTxt').textContent=ok?'Live':'Offline';
  if(!ok){
    document.body.dataset.state='off';
    $('status').textContent='Connection lost';
    $('hint').textContent='Check that the ESP32 is powered and on the same Wi-Fi';
  }
}
function render(d){
  const st=!d.enabled?'off':(d.obstacle?'alert':'clear');
  document.body.dataset.state=st;
  $('status').textContent=TITLES[st];
  $('hint').textContent=HINTS[st];
  if(!busy)$('toggle').checked=d.enabled;
  $('count').textContent=d.count;
  $('last').textContent=d.obstacle?'Now':(d.lastAgo<0?'\u2014':dur(d.lastAgo)+' ago');
  $('up').textContent=dur(d.uptime);
}
async function poll(){
  if(inflight||document.hidden)return;
  inflight=true;
  const ac=new AbortController(),t=setTimeout(()=>ac.abort(),2500);
  try{
    const r=await fetch('/api/status',{cache:'no-store',signal:ac.signal});
    if(!r.ok)throw 0;
    render(await r.json());
    fails=0;conn(true);
  }catch(e){if(++fails>=2)conn(false);}
  clearTimeout(t);inflight=false;
}
$('toggle').addEventListener('change',async e=>{
  busy=true;
  try{
    const r=await fetch('/api/sensor/'+(e.target.checked?'on':'off'),{cache:'no-store'});
    render(await r.json());
  }catch(_){}
  busy=false;poll();
});
document.addEventListener('visibilitychange',poll);
setInterval(poll,500);poll();
</script>
</body></html>
)rawliteral";

// ---------- Sensor logic ----------
void setSensorEnabled(bool on) {
  if (on == sensorEnabled) return;
  sensorEnabled = on;
  obstacle = false;
  lastRaw = (digitalRead(SENSOR_PIN) == LOW);
  lastChangeMs = millis();
  digitalWrite(LED_PIN, LOW);
  Serial.println(on ? "Sensor input: ON" : "Sensor input: OFF");
}

void updateSensor() {
  if (!sensorEnabled) return;

  bool raw = (digitalRead(SENSOR_PIN) == LOW);          // LOW = obstacle
  if (raw != lastRaw) {                                 // Signal changed: restart debounce
    lastRaw = raw;
    lastChangeMs = millis();
  }
  if (raw != obstacle && millis() - lastChangeMs >= DEBOUNCE_MS) {
    obstacle = raw;
    if (obstacle) {
      detectionCount++;
      lastDetectionMs = millis();
      hasDetection = true;
      Serial.println("Obstacle detected");
    } else {
      Serial.println("Path clear");
    }
  }
  digitalWrite(LED_PIN, obstacle ? HIGH : LOW);
}

// ---------- HTTP helpers ----------
String statusJson() {
  String j = "{";
  j += "\"enabled\":";  j += sensorEnabled ? "true" : "false";
  j += ",\"obstacle\":"; j += obstacle ? "true" : "false";
  j += ",\"count\":";   j += String(detectionCount);
  j += ",\"lastAgo\":"; j += hasDetection ? String((millis() - lastDetectionMs) / 1000) : String(-1);
  j += ",\"uptime\":";  j += String(millis() / 1000);
  j += "}";
  return j;
}

void sendText(WiFiClient &client, const char* status, const char* type, const String &body) {
  client.printf("HTTP/1.1 %s\r\nContent-Type: %s\r\nCache-Control: no-store\r\nConnection: close\r\nContent-Length: %u\r\n\r\n",
                status, type, (unsigned)body.length());
  client.print(body);
}

void sendHtml(WiFiClient &client) {
  client.print("HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n");
  client.print(INDEX_HTML);
}

void handleRequest(WiFiClient &client, const String &req) {
  // req looks like: "GET /api/status HTTP/1.1"
  if (req.startsWith("GET /api/status")) {
    sendText(client, "200 OK", "application/json", statusJson());
  } else if (req.startsWith("GET /api/sensor/on")) {
    setSensorEnabled(true);
    sendText(client, "200 OK", "application/json", statusJson());
  } else if (req.startsWith("GET /api/sensor/off")) {
    setSensorEnabled(false);
    sendText(client, "200 OK", "application/json", statusJson());
  } else if (req.startsWith("GET / ") || req.startsWith("GET /index")) {
    sendHtml(client);
  } else if (req.startsWith("GET /favicon.ico")) {
    client.print("HTTP/1.1 204 No Content\r\nConnection: close\r\n\r\n");
  } else {
    sendText(client, "404 Not Found", "text/plain", "Not found");
  }
}

// ---------- Arduino entry points ----------
void setup() {
  Serial.begin(115200);

  pinMode(SENSOR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  lastRaw = (digitalRead(SENSOR_PIN) == LOW);

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);               // Lower latency for the live page
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("Open this address in your browser: http://");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {
  updateSensor();

  // Try to reconnect if Wi-Fi drops
  static unsigned long lastWifiCheck = 0;
  if (WiFi.status() != WL_CONNECTED && millis() - lastWifiCheck > 10000) {
    lastWifiCheck = millis();
    WiFi.reconnect();
  }

  WiFiClient client = server.available();
  if (!client) return;

  String currentLine = "";
  String requestLine = "";
  unsigned long start = millis();

  while (client.connected() && millis() - start < CLIENT_TIMEOUT_MS) {
    updateSensor();                    // Keep sampling while waiting on the browser
    if (!client.available()) { delay(1); continue; }

    char c = client.read();
    if (c == '\n') {
      if (currentLine.length() == 0) { // Blank line = end of request headers
        handleRequest(client, requestLine);
        break;
      }
      if (requestLine.length() == 0) requestLine = currentLine;  // First line: "GET /path HTTP/1.1"
      currentLine = "";
    } else if (c != '\r') {
      currentLine += c;
    }
  }
  client.stop();
}