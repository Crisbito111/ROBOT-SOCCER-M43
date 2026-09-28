#include <WiFi.h>
#include <DNSServer.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include "esp_wifi.h"

// ================= CONFIGURACIÓN DE RED (MODO ACCESS POINT) =================
const char* ap_ssid     = "MA3"; 
const char* ap_password = "123456Bito";    

IPAddress ipLocal(192, 168, 4, 1);   
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);
DNSServer dnsServer; // Servidor DNS para evitar desconexiones en el celular

// ================= ASIGNACIÓN DE PINES (TB6612FNG - ACTUALIZADA) =================
const int AIN1_PIN  = 18;  
const int AIN2_PIN  = 17;  
const int BIN1_PIN  = 3;   
const int BIN2_PIN  = 46;  
const int PWMA_PIN  = 16;  
const int PWMB_PIN  = 9;   
const int STBY_PIN  = 8;   

int VELOCIDAD = 180;

// ================= INTERFAZ WEB PROFESIONAL "MA3 PRO" =================
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>MA3 - SOCCERBOT PRO</title>
    <style>
        :root {
            --bg-gradient: radial-gradient(circle at center, #0f172a 0%, #020617 100%);
            --glass-bg: rgba(15, 23, 42, 0.65);
            --glass-border: rgba(56, 189, 248, 0.2);
            --neon-cyan: #00f2fe;
            --neon-magenta: #f43f5e;
            --neon-green: #10b981;
            --neon-amber: #f59e0b;
            --text-main: #f8fafc;
            --radius-lg: 20px;
            --radius-md: 12px;
        }
        * { box-sizing: border-box; margin: 0; padding: 0; user-select: none; -webkit-user-select: none; }
        body {
            font-family: 'Segoe UI', system-ui, -apple-system, sans-serif;
            background: var(--bg-gradient);
            color: var(--text-main);
            min-height: 100vh;
            display: flex;
            flex-direction: column;
            overflow: hidden;
            padding: 10px;
        }
        .glass-panel {
            background: var(--glass-bg);
            backdrop-filter: blur(16px);
            -webkit-backdrop-filter: blur(16px);
            border: 1px solid var(--glass-border);
            border-radius: var(--radius-lg);
            box-shadow: 0 0 25px rgba(0, 242, 254, 0.08);
        }
        header { 
            display: flex; 
            justify-content: space-between; 
            align-items: center; 
            padding: 12px 18px; 
            margin-bottom: 10px; 
        }
        .brand-title {
            font-size: 24px;
            font-weight: 900;
            letter-spacing: 2px;
            background: linear-gradient(90deg, var(--neon-cyan), #38bdf8);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            text-shadow: 0 0 15px rgba(0, 242, 254, 0.3);
        }
        .brand-sub {
            font-size: 10px;
            color: #64748b;
            letter-spacing: 1px;
            font-weight: 700;
        }
        .telemetry-group { display: flex; gap: 8px; }
        .telemetry-card { 
            display: flex; 
            flex-direction: column; 
            align-items: center; 
            justify-content: center; 
            padding: 6px 10px; 
            background: rgba(255,255,255,0.03); 
            border-radius: var(--radius-md);
            border: 1px solid rgba(255,255,255,0.05);
            min-width: 55px;
        }
        .telemetry-card span:first-child { font-size: 9px; text-transform: uppercase; color: #94a3b8; font-weight: 600; }
        .telemetry-card span:last-child { font-size: 14px; font-weight: 800; color: var(--neon-cyan); font-family: monospace; }
        .status-online { color: var(--neon-green) !important; text-shadow: 0 0 8px rgba(16, 185, 129, 0.4); }
        
        .mode-selector { display: flex; justify-content: space-between; padding: 4px; gap: 6px; margin-bottom: 10px; }
        .mode-btn { 
            flex: 1; 
            padding: 10px 4px; 
            border: none; 
            background: transparent; 
            color: #64748b; 
            font-weight: 700; 
            font-size: 11px; 
            letter-spacing: 0.5px;
            border-radius: calc(var(--radius-md) - 2px); 
            cursor: pointer; 
            transition: all 0.2s ease;
        }
        .mode-btn.active { 
            background: linear-gradient(135deg, rgba(0,242,254,0.2) 0%, rgba(56,189,248,0.1) 100%); 
            color: var(--neon-cyan); 
            border: 1px solid var(--neon-cyan); 
            box-shadow: 0 0 12px rgba(0,242,254,0.2);
        }

        .view-viewport { flex: 1; position: relative; display: flex; align-items: center; justify-content: center; padding: 10px; }
        .control-view { position: absolute; width: 100%; height: 100%; display: none; align-items: center; justify-content: center; flex-direction: column; }
        .control-view.active { display: flex; }

        /* D-PAD PRO */
        .dpad-container { display: grid; grid-template-columns: repeat(3, 1fr); gap: 12px; width: 250px; margin-bottom: 20px; }
        .dpad-btn { 
            width: 72px; 
            height: 72px; 
            background: rgba(255,255,255,0.03); 
            border: 1px solid var(--glass-border); 
            border-radius: var(--radius-md); 
            color: var(--text-main); 
            font-size: 22px; 
            cursor: pointer; 
            display: flex;
            align-items: center;
            justify-content: center;
            transition: all 0.08s ease;
            box-shadow: 0 4px 12px rgba(0,0,0,0.4);
        }
        .dpad-btn:active { 
            background: var(--neon-cyan); 
            color: #020617; 
            transform: scale(0.93);
            box-shadow: 0 0 20px var(--neon-cyan);
        }
        .dpad-btn.btn-stop { 
            background: rgba(244, 63, 94, 0.15); 
            border-color: rgba(244, 63, 94, 0.4); 
            color: var(--neon-magenta); 
        }
        .dpad-btn.btn-stop:active { 
            background: var(--neon-magenta); 
            color: #fff; 
            box-shadow: 0 0 20px var(--neon-magenta);
        }

        /* SLIDER DE VELOCIDAD */
        .slider-box { 
            width: 85%; 
            padding: 12px 18px; 
            background: rgba(0,0,0,0.3); 
            border-radius: var(--radius-md); 
            border: 1px solid rgba(255,255,255,0.05);
            display: flex;
            flex-direction: column;
            gap: 6px;
        }
        .slider-header { display: flex; justify-content: space-between; font-size: 11px; font-weight: 700; color: #94a3b8; }
        .slider-box input[type=range] { 
            width: 100%; 
            -webkit-appearance: none; 
            background: #1e293b; 
            height: 8px; 
            border-radius: 4px; 
            outline: none; 
        }
        .slider-box input[type=range]::-webkit-slider-thumb { 
            -webkit-appearance: none; 
            width: 22px; 
            height: 22px; 
            border-radius: 50%; 
            background: var(--neon-cyan); 
            box-shadow: 0 0 10px var(--neon-cyan); 
            cursor: pointer; 
        }

        /* JOYSTICKS PRO */
        .joystick-pad { 
            width: 220px; 
            height: 220px; 
            background: radial-gradient(circle, rgba(0,242,254,0.03) 0%, rgba(0,0,0,0.4) 100%);
            border: 2px solid var(--glass-border); 
            border-radius: 50%; 
            position: relative; 
            display: flex; 
            align-items: center; 
            justify-content: center; 
            touch-action: none; 
            box-shadow: inset 0 0 20px rgba(0,0,0,0.8);
        }
        .joystick-handle { 
            width: 65px; 
            height: 65px; 
            background: linear-gradient(135deg, #1e293b 0%, #0f172a 100%); 
            border: 2px solid var(--neon-cyan); 
            border-radius: 50%; 
            position: absolute; 
            touch-action: none; 
            box-shadow: 0 0 15px rgba(0,242,254,0.4);
            transition: transform 0.05s ease-out;
        }
        .dual-container { display: flex; width: 100%; justify-content: space-around; gap: 10px; }
        .dual-label { font-size: 10px; color: #64748b; font-weight: 800; margin-bottom: 6px; text-align: center; letter-spacing: 1px; }
    </style>
</head>
<body>
    <header class="glass-panel">
        <div>
            <div class="brand-title">MA3</div>
            <div class="brand-sub">SOCCERBOT PRO</div>
        </div>
        <div class="telemetry-group">
            <div class="telemetry-card"><span>NET</span><span class="status-online">ON</span></div>
            <div class="telemetry-card"><span>BAT</span><span>7.4V</span></div>
            <div class="telemetry-card"><span>PWM</span><span id="txt-pwm">180</span></div>
        </div>
    </header>

    <nav class="mode-selector glass-panel">
        <button class="mode-btn active" onclick="switchMode(1)">D-PAD</button>
        <button class="mode-btn" onclick="switchMode(2)">JOYSTICK 360°</button>
        <button class="mode-btn" onclick="switchMode(3)">DUAL PRO</button>
    </nav>

    <main class="view-viewport glass-panel">
        <div id="mode-1" class="control-view active">
            <div class="dpad-container">
                <div></div>
                <button class="dpad-btn" ontouchstart="sendCmd('adelante')" ontouchend="sendCmd('detener')" onmousedown="sendCmd('adelante')" onmouseup="sendCmd('detener')">▲</button>
                <div></div>
                <button class="dpad-btn" ontouchstart="sendCmd('izquierda')" ontouchend="sendCmd('detener')" onmousedown="sendCmd('izquierda')" onmouseup="sendCmd('detener')">◀</button>
                <button class="dpad-btn btn-stop" onclick="sendCmd('detener')">■</button>
                <button class="dpad-btn" ontouchstart="sendCmd('derecha')" ontouchend="sendCmd('detener')" onmousedown="sendCmd('derecha')" onmouseup="sendCmd('detener')">▶</button>
                <div></div>
                <button class="dpad-btn" ontouchstart="sendCmd('atras')" ontouchend="sendCmd('detener')" onmousedown="sendCmd('atras')" onmouseup="sendCmd('detener')">▼</button>
                <div></div>
            </div>
            <div class="slider-box">
                <div class="slider-header"><span>POTENCIA DE TRACCIÓN</span><span id="txt-pct">70%</span></div>
                <input type="range" min="0" max="255" value="180" oninput="updateSpeedLimit(this.value)">
            </div>
        </div>

        <div id="mode-2" class="control-view">
            <div class="joystick-pad" id="pad-360"><div class="joystick-handle" id="handle-360"></div></div>
        </div>

        <div id="mode-3" class="control-view">
            <div class="dual-container">
                <div>
                    <div class="dual-label">DIRECCIÓN (X)</div>
                    <div class="joystick-pad" id="pad-left" style="width:140px; height:140px;"><div class="joystick-handle" id="handle-left" style="width:48px; height:48px;"></div></div>
                </div>
                <div>
                    <div class="dual-label">AVANCE (Y)</div>
                    <div class="joystick-pad" id="pad-right" style="width:140px; height:140px;"><div class="joystick-handle" id="handle-right" style="width:48px; height:48px;"></div></div>
                </div>
            </div>
        </div>
    </main>

    <script>
        let currentSpeed = 180;
        function switchMode(modeNum) {
            document.querySelectorAll('.mode-btn').forEach((btn, idx) => btn.classList.toggle('active', idx === (modeNum - 1)));
            document.querySelectorAll('.control-view').forEach((view, idx) => view.classList.toggle('active', idx === (modeNum - 1)));
            sendCmd('detener');
        }
        function updateSpeedLimit(v) {
            currentSpeed = v;
            document.getElementById('txt-pwm').innerText = v;
            document.getElementById('txt-pct').innerText = Math.round((v / 255) * 100) + '%';
            fetch(`/set_vel?v=${v}`).catch(() => {});
        }
        function sendCmd(cmd) { fetch(`/${cmd}`).catch(() => {}); }

        function setupJoystick(padId, handleId, axisLock) {
            const pad = document.getElementById(padId);
            const handle = document.getElementById(handleId);
            let active = false; let center = { x: 0, y: 0 };

            const initTrack = (e) => { active = true; const rect = pad.getBoundingClientRect(); center = { x: rect.width / 2, y: rect.height / 2 }; moveHandle(e); };
            const moveHandle = (e) => {
                if (!active) return;
                const rect = pad.getBoundingClientRect();
                const pointer = e.touches ? e.touches[0] : e;
                let x = pointer.clientX - rect.left - center.x;
                let y = pointer.clientY - rect.top - center.y;
                if (axisLock === 'X') y = 0; if (axisLock === 'Y') x = 0;
                const maxDist = rect.width / 2 - handle.offsetWidth / 2;
                const distance = Math.sqrt(x*x + y*y);
                if (distance > maxDist) { const angle = Math.atan2(y, x); x = Math.cos(angle) * maxDist; y = Math.sin(angle) * maxDist; }
                handle.style.transform = `translate(${x}px, ${y}px)`;
                processVector(padId, parseFloat((x / maxDist).toFixed(2)), parseFloat((-y / maxDist).toFixed(2)));
            };
            const endTrack = () => { if (!active) return; active = false; handle.style.transform = 'translate(0px, 0px)'; processVector(padId, 0, 0); };
            pad.addEventListener('mousedown', initTrack); window.addEventListener('mousemove', moveHandle); window.addEventListener('mouseup', endTrack);
            pad.addEventListener('touchstart', initTrack, { passive: true }); window.addEventListener('touchmove', moveHandle, { passive: false }); window.addEventListener('touchend', endTrack);
        }

        let lastSent = 0;
        function processVector(id, x, y) {
            const now = Date.now();
            if (now - lastSent < 60 && (x !== 0 || y !== 0)) return; 
            lastSent = now;
            
            if(x === 0 && y === 0) { sendCmd('detener'); return; }

            if(id === 'pad-360' || id === 'pad-right') {
                if(y > 0.2) sendCmd('adelante');
                else if(y < -0.2) sendCmd('atras');
            }
            if(id === 'pad-360' || id === 'pad-left') {
                if(x > 0.2) sendCmd('derecha');
                else if(x < -0.2) sendCmd('izquierda');
            }
        }
        setupJoystick('pad-360', 'handle-360', 'NONE');
        setupJoystick('pad-left', 'handle-left', 'X');
        setupJoystick('pad-right', 'handle-right', 'Y');
    </script>
</body>
</html>
)rawliteral";

// ================= CONTROL DE MOTORES =================
void moverAdelante() {
    digitalWrite(AIN1_PIN, HIGH); digitalWrite(AIN2_PIN, LOW);
    digitalWrite(BIN1_PIN, HIGH); digitalWrite(BIN2_PIN, LOW);
    ledcWrite(PWMA_PIN, VELOCIDAD); ledcWrite(PWMB_PIN, VELOCIDAD);
}

void moverAtras() {
    digitalWrite(AIN1_PIN, LOW); digitalWrite(AIN2_PIN, HIGH);
    digitalWrite(BIN1_PIN, LOW); digitalWrite(BIN2_PIN, HIGH);
    ledcWrite(PWMA_PIN, VELOCIDAD); ledcWrite(PWMB_PIN, VELOCIDAD);
}

void girarIzquierda() {
    digitalWrite(AIN1_PIN, HIGH); digitalWrite(AIN2_PIN, LOW);
    digitalWrite(BIN1_PIN, LOW);  digitalWrite(BIN2_PIN, HIGH);
    ledcWrite(PWMA_PIN, VELOCIDAD); ledcWrite(PWMB_PIN, VELOCIDAD);
}

void girarDerecha() {
    digitalWrite(AIN1_PIN, LOW);  digitalWrite(AIN2_PIN, HIGH);
    digitalWrite(BIN1_PIN, HIGH); digitalWrite(BIN2_PIN, LOW);
    ledcWrite(PWMA_PIN, VELOCIDAD); ledcWrite(PWMB_PIN, VELOCIDAD);
}

void detener() {
    ledcWrite(PWMA_PIN, 0); ledcWrite(PWMB_PIN, 0);
}

// ================= SETUP GENERAL =================
void setup() {
    Serial.begin(115200);

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(ipLocal, gateway, subnet);
    WiFi.softAP(ap_ssid, ap_password);

    // Desactivar ahorro de energía en antena WiFi para máxima velocidad de respuesta
    esp_wifi_set_ps(WIFI_PS_NONE);

    // Servidor DNS Cautivo para estabilizar la conexión del celular
    dnsServer.start(53, "*", ipLocal);

    ArduinoOTA.begin();

    pinMode(AIN1_PIN, OUTPUT); pinMode(AIN2_PIN, OUTPUT);
    pinMode(BIN1_PIN, OUTPUT); pinMode(BIN2_PIN, OUTPUT);
    pinMode(STBY_PIN, OUTPUT);
    digitalWrite(STBY_PIN, HIGH); 
    
    // Configuración PWM moderna compatible con ESP32 Core v3.x
    ledcAttach(PWMA_PIN, 1000, 8); 
    ledcAttach(PWMB_PIN, 1000, 8);
    detener(); 

    // RUTAS HTTP Y CAPTIVE PORTAL REDIRECT
    server.on("/", []() { server.send(200, "text/html", INDEX_HTML); });
    server.on("/generate_204", []() { server.send(200, "text/html", INDEX_HTML); });
    server.on("/fwlink", []() { server.send(200, "text/html", INDEX_HTML); });
    
    server.on("/adelante", []() { moverAdelante(); server.send(200, "text/plain", "OK"); });
    server.on("/atras", []() { moverAtras(); server.send(200, "text/plain", "OK"); });
    server.on("/izquierda", []() { girarIzquierda(); server.send(200, "text/plain", "OK"); });
    server.on("/derecha", []() { girarDerecha(); server.send(200, "text/plain", "OK"); });
    server.on("/detener", []() { detener(); server.send(200, "text/plain", "OK"); });
    
    server.on("/set_vel", []() {
        if (server.hasArg("v")) {
            VELOCIDAD = server.arg("v").toInt();
            ledcWrite(PWMA_PIN, VELOCIDAD);
            ledcWrite(PWMB_PIN, VELOCIDAD);
        }
        server.send(200, "text/plain", "OK");
    });

    server.onNotFound([]() { server.send(200, "text/html", INDEX_HTML); });
    server.begin();
    Serial.println("MA3 Ready for Competition.");
}

void loop() {
    dnsServer.processNextRequest(); 
    ArduinoOTA.handle();      
    server.handleClient();    
}