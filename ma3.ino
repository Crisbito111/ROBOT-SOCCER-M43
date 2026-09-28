#include <WiFi.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include "esp_wifi.h"

// ================= RED (MODO ACCESS POINT) =================
const char* ap_ssid     = "MA3";
const char* ap_password = "123456Bito";

IPAddress ipLocal(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);

// ================= PINES (LOS QUE YA PROBASTE) =================
const int AIN1_PIN = 18;
const int AIN2_PIN = 17;
const int BIN1_PIN = 3;
const int BIN2_PIN = 46;
const int PWMA_PIN = 16;
const int PWMB_PIN = 9;
const int STBY_PIN = 8;

// ================= ESTADO =================
int VELOCIDAD = 180;                 // tope de PWM (0-255)
int cmdA = 0, cmdB = 0;              // -100..100 (porcentaje por motor)
unsigned long ultimoComando = 0;     // para el failsafe
const unsigned long TIMEOUT_MS = 800;

// ================= INTERFAZ WEB =================
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<title>MA3 - SOCCERBOT PRO</title>
<style>
:root{
  --bg-gradient: radial-gradient(circle at center,#0f172a 0%,#020617 100%);
  --glass-bg: rgba(15,23,42,0.65);
  --glass-border: rgba(56,189,248,0.2);
  --neon-cyan:#00f2fe; --neon-magenta:#f43f5e; --neon-green:#10b981;
  --text-main:#f8fafc; --radius-lg:20px; --radius-md:12px;
}
*{box-sizing:border-box;margin:0;padding:0;user-select:none;-webkit-user-select:none;-webkit-touch-callout:none;-webkit-tap-highlight-color:transparent}
html,body{height:100%}
body{font-family:'Segoe UI',system-ui,-apple-system,sans-serif;background:var(--bg-gradient);color:var(--text-main);display:flex;flex-direction:column;overflow:hidden;padding:10px;touch-action:none}
.glass-panel{background:var(--glass-bg);backdrop-filter:blur(16px);-webkit-backdrop-filter:blur(16px);border:1px solid var(--glass-border);border-radius:var(--radius-lg);box-shadow:0 0 25px rgba(0,242,254,0.08)}
header{display:flex;justify-content:space-between;align-items:center;padding:12px 18px;margin-bottom:10px}
.brand-title{font-size:24px;font-weight:900;letter-spacing:2px;background:linear-gradient(90deg,var(--neon-cyan),#38bdf8);-webkit-background-clip:text;-webkit-text-fill-color:transparent}
.brand-sub{font-size:10px;color:#64748b;letter-spacing:1px;font-weight:700}
.telemetry-group{display:flex;gap:8px}
.telemetry-card{display:flex;flex-direction:column;align-items:center;justify-content:center;padding:6px 10px;background:rgba(255,255,255,0.03);border-radius:var(--radius-md);border:1px solid rgba(255,255,255,0.05);min-width:55px}
.telemetry-card span:first-child{font-size:9px;text-transform:uppercase;color:#94a3b8;font-weight:600}
.telemetry-card span:last-child{font-size:14px;font-weight:800;color:var(--neon-cyan);font-family:monospace}
.status-online{color:var(--neon-green)!important}
.status-off{color:var(--neon-magenta)!important}
.mode-selector{display:flex;justify-content:space-between;padding:4px;gap:6px;margin-bottom:10px}
.mode-btn{flex:1;padding:10px 4px;border:1px solid transparent;background:transparent;color:#64748b;font-weight:700;font-size:11px;letter-spacing:.5px;border-radius:10px;cursor:pointer}
.mode-btn.active{background:linear-gradient(135deg,rgba(0,242,254,0.2),rgba(56,189,248,0.1));color:var(--neon-cyan);border:1px solid var(--neon-cyan);box-shadow:0 0 12px rgba(0,242,254,0.2)}
.view-viewport{flex:1;position:relative;display:flex;align-items:center;justify-content:center;padding:10px}
.control-view{position:absolute;width:100%;height:100%;display:none;align-items:center;justify-content:center;flex-direction:column}
.control-view.active{display:flex}
.dpad-container{display:grid;grid-template-columns:repeat(3,1fr);gap:12px;width:250px;margin-bottom:20px}
.dpad-btn{width:72px;height:72px;background:rgba(255,255,255,0.03);border:1px solid var(--glass-border);border-radius:var(--radius-md);color:var(--text-main);font-size:22px;cursor:pointer;display:flex;align-items:center;justify-content:center;touch-action:none;box-shadow:0 4px 12px rgba(0,0,0,0.4)}
.dpad-btn.down{background:var(--neon-cyan);color:#020617;transform:scale(.93);box-shadow:0 0 20px var(--neon-cyan)}
.dpad-btn.btn-stop{background:rgba(244,63,94,0.15);border-color:rgba(244,63,94,0.4);color:var(--neon-magenta)}
.dpad-btn.btn-stop.down{background:var(--neon-magenta);color:#fff;box-shadow:0 0 20px var(--neon-magenta)}
.slider-box{width:85%;padding:12px 18px;background:rgba(0,0,0,0.3);border-radius:var(--radius-md);border:1px solid rgba(255,255,255,0.05);display:flex;flex-direction:column;gap:6px}
.slider-header{display:flex;justify-content:space-between;font-size:11px;font-weight:700;color:#94a3b8}
.slider-box input[type=range]{width:100%;-webkit-appearance:none;appearance:none;background:#1e293b;height:8px;border-radius:4px;outline:none;touch-action:pan-x}
.slider-box input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:22px;height:22px;border-radius:50%;background:var(--neon-cyan);box-shadow:0 0 10px var(--neon-cyan);cursor:pointer}
.joystick-pad{width:220px;height:220px;background:radial-gradient(circle,rgba(0,242,254,0.03) 0%,rgba(0,0,0,0.4) 100%);border:2px solid var(--glass-border);border-radius:50%;position:relative;display:flex;align-items:center;justify-content:center;touch-action:none;box-shadow:inset 0 0 20px rgba(0,0,0,0.8)}
.joystick-handle{width:65px;height:65px;background:linear-gradient(135deg,#1e293b,#0f172a);border:2px solid var(--neon-cyan);border-radius:50%;position:absolute;pointer-events:none;box-shadow:0 0 15px rgba(0,242,254,0.4)}
.dual-container{display:flex;width:100%;justify-content:space-around;gap:10px}
.dual-label{font-size:10px;color:#64748b;font-weight:800;margin-bottom:6px;text-align:center;letter-spacing:1px}
</style>
</head>
<body>
<header class="glass-panel">
  <div>
    <div class="brand-title">MA3</div>
    <div class="brand-sub">SOCCERBOT PRO</div>
  </div>
  <div class="telemetry-group">
    <div class="telemetry-card"><span>NET</span><span id="txt-net" class="status-online">ON</span></div>
    <div class="telemetry-card"><span>PWM</span><span id="txt-pwm">180</span></div>
  </div>
</header>

<nav class="mode-selector glass-panel">
  <button class="mode-btn active" onclick="switchMode(1)">D-PAD</button>
  <button class="mode-btn" onclick="switchMode(2)">JOYSTICK 360&deg;</button>
  <button class="mode-btn" onclick="switchMode(3)">DUAL PRO</button>
</nav>

<main class="view-viewport glass-panel">
  <div id="mode-1" class="control-view active">
    <div class="dpad-container">
      <div></div><button class="dpad-btn" data-cmd="adelante">&#9650;</button><div></div>
      <button class="dpad-btn" data-cmd="izquierda">&#9664;</button>
      <button class="dpad-btn btn-stop" data-cmd="detener">&#9632;</button>
      <button class="dpad-btn" data-cmd="derecha">&#9654;</button>
      <div></div><button class="dpad-btn" data-cmd="atras">&#9660;</button><div></div>
    </div>
    <div class="slider-box">
      <div class="slider-header"><span>POTENCIA DE TRACCI&Oacute;N</span><span id="txt-pct">71%</span></div>
      <input id="speed" type="range" min="0" max="255" value="180">
    </div>
  </div>

  <div id="mode-2" class="control-view">
    <div class="joystick-pad" id="pad-360"><div class="joystick-handle" id="handle-360"></div></div>
  </div>

  <div id="mode-3" class="control-view">
    <div class="dual-container">
      <div>
        <div class="dual-label">DIRECCI&Oacute;N (X)</div>
        <div class="joystick-pad" id="pad-left" style="width:140px;height:140px"><div class="joystick-handle" id="handle-left" style="width:48px;height:48px"></div></div>
      </div>
      <div>
        <div class="dual-label">AVANCE (Y)</div>
        <div class="joystick-pad" id="pad-right" style="width:140px;height:140px"><div class="joystick-handle" id="handle-right" style="width:48px;height:48px"></div></div>
      </div>
    </div>
  </div>
</main>

<script>
let heldCmd = null;     // boton del D-PAD mantenido
let vx = 0, vy = 0;     // vector de los joysticks (-1..1)
let failCount = 0;

function net(ok){
  failCount = ok ? 0 : failCount + 1;
  const el = document.getElementById('txt-net');
  const off = failCount >= 3;
  el.textContent = off ? 'OFF' : 'ON';
  el.className = off ? 'status-off' : 'status-online';
}
function req(url){
  return fetch(url, {cache:'no-store'}).then(()=>net(true)).catch(()=>net(false));
}
function sendCmd(cmd){ req('/' + cmd + '?t=' + Date.now()); }

/* ---------- Modo ---------- */
function switchMode(n){
  document.querySelectorAll('.mode-btn').forEach((b,i)=>b.classList.toggle('active', i===n-1));
  document.querySelectorAll('.control-view').forEach((v,i)=>v.classList.toggle('active', i===n-1));
  heldCmd = null; vx = 0; vy = 0;
  sendCmd('detener');
}

/* ---------- D-PAD (pointer events: sirve para touch y mouse) ---------- */
document.querySelectorAll('.dpad-btn').forEach(btn=>{
  const cmd = btn.dataset.cmd;
  const press = e=>{
    e.preventDefault();
    btn.setPointerCapture(e.pointerId);
    btn.classList.add('down');
    if(cmd === 'detener'){ heldCmd = null; sendCmd('detener'); }
    else { heldCmd = cmd; sendCmd(cmd); }
  };
  const release = e=>{
    btn.classList.remove('down');
    if(cmd !== 'detener' && heldCmd === cmd){ heldCmd = null; sendCmd('detener'); }
  };
  btn.addEventListener('pointerdown', press);
  btn.addEventListener('pointerup', release);
  btn.addEventListener('pointercancel', release);
  btn.addEventListener('contextmenu', e=>e.preventDefault());
});

/* ---------- Velocidad ---------- */
const speed = document.getElementById('speed');
let lastVel = 0;
function sendVel(force){
  const now = Date.now();
  if(!force && now - lastVel < 100) return;
  lastVel = now;
  req('/set_vel?v=' + speed.value);
}
speed.addEventListener('input', ()=>{
  document.getElementById('txt-pwm').textContent = speed.value;
  document.getElementById('txt-pct').textContent = Math.round(speed.value/255*100) + '%';
  sendVel(false);
});
speed.addEventListener('change', ()=>sendVel(true));

/* ---------- Joysticks ---------- */
let lastDrive = 0, driveTimer = null;
function sendDrive(force){
  const now = Date.now();
  if(!force && now - lastDrive < 70){
    clearTimeout(driveTimer);
    driveTimer = setTimeout(()=>sendDrive(true), 70);
    return;
  }
  clearTimeout(driveTimer);
  lastDrive = now;
  // Motor A = derecha fisica, Motor B = izquierda fisica (segun tu mapeo original)
  let a = vy - vx, b = vy + vx;
  const m = Math.max(1, Math.abs(a), Math.abs(b));
  a /= m; b /= m;
  req('/drive?a=' + Math.round(a*100) + '&b=' + Math.round(b*100) + '&t=' + now);
}

function setupJoystick(padId, handleId, axisLock, onMove){
  const pad = document.getElementById(padId);
  const handle = document.getElementById(handleId);
  let active = false;

  const move = e=>{
    const r = pad.getBoundingClientRect();
    let x = e.clientX - r.left - r.width/2;
    let y = e.clientY - r.top  - r.height/2;
    if(axisLock === 'X') y = 0;
    if(axisLock === 'Y') x = 0;
    const maxDist = r.width/2 - handle.offsetWidth/2;
    const d = Math.hypot(x, y);
    if(d > maxDist){ x = x/d*maxDist; y = y/d*maxDist; }
    handle.style.transform = 'translate(' + x + 'px,' + y + 'px)';
    let nx = x/maxDist, ny = -y/maxDist;
    if(Math.abs(nx) < 0.12) nx = 0;   // zona muerta
    if(Math.abs(ny) < 0.12) ny = 0;
    onMove(nx, ny);
  };
  const end = ()=>{
    if(!active) return;
    active = false;
    handle.style.transform = 'translate(0px,0px)';
    onMove(0, 0, true);
  };
  pad.addEventListener('pointerdown', e=>{ e.preventDefault(); active = true; pad.setPointerCapture(e.pointerId); move(e); });
  pad.addEventListener('pointermove', e=>{ if(active) move(e); });
  pad.addEventListener('pointerup', end);
  pad.addEventListener('pointercancel', end);
}

setupJoystick('pad-360','handle-360','NONE',(x,y,fin)=>{ vx = x; vy = y; sendDrive(fin === true); });
setupJoystick('pad-left','handle-left','X',(x,y,fin)=>{ vx = x; sendDrive(fin === true); });
setupJoystick('pad-right','handle-right','Y',(x,y,fin)=>{ vy = y; sendDrive(fin === true); });

/* ---------- Keepalive: el robot se detiene solo si dejan de llegar comandos ---------- */
setInterval(()=>{
  if(heldCmd) sendCmd(heldCmd);
  else if(vx !== 0 || vy !== 0) sendDrive(true);
}, 300);

document.addEventListener('visibilitychange', ()=>{ if(document.hidden){ heldCmd = null; vx = 0; vy = 0; sendCmd('detener'); }});
</script>
</body>
</html>
)rawliteral";

// ================= CONTROL DE MOTORES =================
// pct: -100..100. Positivo = adelante, negativo = atras, 0 = parado.
void aplicarMotor(int in1, int in2, int pwmPin, int pct) {
    pct = constrain(pct, -100, 100);
    int duty = abs(pct) * VELOCIDAD / 100;
    if (pct > 0)      { digitalWrite(in1, HIGH); digitalWrite(in2, LOW); }
    else if (pct < 0) { digitalWrite(in1, LOW);  digitalWrite(in2, HIGH); }
    else              { digitalWrite(in1, LOW);  digitalWrite(in2, LOW); }
    ledcWrite(pwmPin, duty);
}

void aplicar() {
    aplicarMotor(AIN1_PIN, AIN2_PIN, PWMA_PIN, cmdA);
    aplicarMotor(BIN1_PIN, BIN2_PIN, PWMB_PIN, cmdB);
}

void mover(int a, int b) {
    cmdA = a;
    cmdB = b;
    ultimoComando = millis();
    aplicar();
}

void moverAdelante()  { mover( 100,  100); }
void moverAtras()     { mover(-100, -100); }
void girarIzquierda() { mover( 100, -100); }   // igual que tu codigo original
void girarDerecha()   { mover(-100,  100); }
void detener()        { mover(   0,    0); }

void responderOK() {
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "text/plain", "OK");
}

// ================= SETUP =================
void setup() {
    Serial.begin(115200);

    pinMode(AIN1_PIN, OUTPUT); pinMode(AIN2_PIN, OUTPUT);
    pinMode(BIN1_PIN, OUTPUT); pinMode(BIN2_PIN, OUTPUT);
    pinMode(STBY_PIN, OUTPUT);
    digitalWrite(STBY_PIN, HIGH);

    ledcAttach(PWMA_PIN, 1000, 8);
    ledcAttach(PWMB_PIN, 1000, 8);
    detener();

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(ipLocal, gateway, subnet);
    WiFi.softAP(ap_ssid, ap_password, 1, 0, 4);   // canal 1, visible, max 4 clientes
    esp_wifi_set_ps(WIFI_PS_NONE);

    ArduinoOTA.begin();

    // Pagina principal
    server.on("/", []() {
        server.sendHeader("Cache-Control", "no-store");
        server.send(200, "text/html", INDEX_HTML);
    });

    // Deteccion de portal cautivo: responder "hay internet" para que el celular NO se desconecte
    server.on("/generate_204", []() { server.send(204, "text/plain", ""); });
    server.on("/gen_204",      []() { server.send(204, "text/plain", ""); });
    server.on("/hotspot-detect.html", []() {
        server.send(200, "text/html", "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>");
    });

    // Comandos
    server.on("/adelante",  []() { moverAdelante();  responderOK(); });
    server.on("/atras",     []() { moverAtras();     responderOK(); });
    server.on("/izquierda", []() { girarIzquierda(); responderOK(); });
    server.on("/derecha",   []() { girarDerecha();   responderOK(); });
    server.on("/detener",   []() { detener();        responderOK(); });

    // Control diferencial (joysticks): a y b en -100..100
    server.on("/drive", []() {
        int a = server.arg("a").toInt();
        int b = server.arg("b").toInt();
        mover(a, b);
        responderOK();
    });

    // Velocidad maxima: NO mueve los motores por si sola
    server.on("/set_vel", []() {
        if (server.hasArg("v")) {
            VELOCIDAD = constrain(server.arg("v").toInt(), 0, 255);
            aplicar();   // si esta parado, duty = 0 y no pasa nada
        }
        responderOK();
    });

    server.onNotFound([]() { server.send(404, "text/plain", "No encontrado"); });
    server.begin();

    Serial.print("MA3 listo. IP: ");
    Serial.println(WiFi.softAPIP());
}

// ================= LOOP =================
void loop() {
    ArduinoOTA.handle();
    server.handleClient();

    // Failsafe: si se pierde el WiFi con un boton apretado, el robot se detiene
    if ((cmdA != 0 || cmdB != 0) && millis() - ultimoComando > TIMEOUT_MS) {
        detener();
    }
}