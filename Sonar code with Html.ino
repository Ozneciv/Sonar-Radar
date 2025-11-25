#include <Servo.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>

// Pinos
const int trigPin = D6;
const int echoPin = D5;
const int servoPin = D3;
const int buzzerPin = D4;
const int ledVerde = D7;
const int ledVermelho = D8;

// Configurações
#define VELOCIDADE_SOM 0.034
#define DISTANCIA_ALERTA 10
#define ANGLE_INCREMENT 3

// WiFi e WebSocket
const char* ssid = "Vicenzo";
const char* password = "Vicenzo1";
ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

Servo meuServo;
int angulo = 0;
bool direcao = true;
bool objetoDetectado = false;

void setup() {
  Serial.begin(115200);
  
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(ledVerde, OUTPUT);
  pinMode(ledVermelho, OUTPUT);
  
  meuServo.attach(servoPin);
  digitalWrite(ledVerde, HIGH);
  
  // Conecta ao WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi...");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  
  // Inicia servidor WebSocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  
  // Configura servidor web
  server.on("/", handleRoot);
  server.begin();
  
  Serial.println("Servidor HTTP iniciado!");
}

void loop() {
  server.handleClient();
  webSocket.loop();
  
  // Movimento do servo
  if (!objetoDetectado) {
    if (direcao) {
      angulo += ANGLE_INCREMENT;
      if (angulo >= 180) {
        direcao = false;
        angulo = 180;
      }
    } else {
      angulo -= ANGLE_INCREMENT;
      if (angulo <= 0) {
        direcao = true;
        angulo = 0;
      }
    }
    meuServo.write(angulo);
  }

  // Medição da distância
  float distancia = medirDistancia();
  
  // Controle de alerta
  bool objetoNaPosicao = (distancia <= DISTANCIA_ALERTA && distancia > 0);
  
  if (objetoNaPosicao) {
    if (!objetoDetectado) {
      objetoDetectado = true;
      digitalWrite(ledVerde, LOW);
      digitalWrite(ledVermelho, HIGH);
    }
    tone(buzzerPin, 1000, 100);
  } else {
    if (objetoDetectado) {
      objetoDetectado = false;
      digitalWrite(ledVerde, HIGH);
      digitalWrite(ledVermelho, LOW);
      noTone(buzzerPin);
    }
  }

  // Envia dados via WebSocket (incluindo estado de alerta)
  String jsonData = "{\"angle\":" + String(angulo) + ",\"distance\":" + String(distancia) + ",\"alert\":" + (objetoNaPosicao ? "true" : "false") + "}";
  webSocket.broadcastTXT(jsonData.c_str(), jsonData.length());
  
  delay(50);
}

float medirDistancia() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duracao = pulseIn(echoPin, HIGH);
  return duracao * VELOCIDADE_SOM / 2;
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Desconectado!\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Conectado em %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
      }
      break;
  }
}

void handleRoot() {
  String html = R"=====(
  <!DOCTYPE html>
  <html>
  <head>
      <title>Sonar - Enriquecimento Instrumental </title>
      <style>
          body { font-family: Arial, sans-serif; text-align: center; margin: 0; padding: 20px; background-color: #f0f0f0; }
          canvas { background-color: #000; border-radius: 50%; margin: 20px auto; display: block; }
          .info { margin: 10px; padding: 10px; background-color: #fff; border-radius: 5px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
      </style>
  </head>
  <body>
      <h1>Presence detection Sonar - Enriquecimento Instrumental </h1>
      <div class="info">
          <p>Angle: <span id="angle">0</span>° | Distance: <span id="distance">0</span> cm</p>
          <p>Status: <span id="status" style="color:green">Livre</span></p>
      </div>
      <canvas id="sonar" width="500" height="500"></canvas>

      <script>
          const canvas = document.getElementById('sonar');
          const ctx = canvas.getContext('2d');
          const centerX = canvas.width / 2;
          const centerY = canvas.height / 2;
          const radius = Math.min(centerX, centerY) - 10;
          
          let distances = Array(181).fill(0);
          let alerts = Array(181).fill(false); // Armazena estados de alerta por ângulo

          const ws = new WebSocket('ws://' + window.location.hostname + ':81');
          
          ws.onmessage = function(event) {
              const data = JSON.parse(event.data);
              const angle = parseInt(data.angle);
              const distance = parseFloat(data.distance);
              const alert = data.alert; // Novo: estado de alerta
              
              document.getElementById('angle').textContent = angle;
              document.getElementById('distance').textContent = distance.toFixed(1);
              
              distances[angle] = distance;
              alerts[angle] = alert; // Atualiza o estado de alerta
              
              if (alert) {
                  document.getElementById('status').textContent = "Objeto detectado!";
                  document.getElementById('status').style.color = "red";
              } else {
                  document.getElementById('status').textContent = "Livre";
                  document.getElementById('status').style.color = "green";
              }
          };

          function drawSonar() {
              ctx.clearRect(0, 0, canvas.width, canvas.height);
              
              // Desenha círculos de distância
              ctx.strokeStyle = 'rgba(0, 255, 0, 0.3)';
              for (let r = radius/5; r <= radius; r += radius/5) {
                  ctx.beginPath();
                  ctx.arc(centerX, centerY, r, 0, Math.PI * 2);
                  ctx.stroke();
                  ctx.fillStyle = 'rgba(0, 255, 0, 0.7)';
                  ctx.font = '12px Arial';
                  ctx.fillText(((r/radius)*100).toFixed(0) + 'cm', centerX + r, centerY);
              }
              
              // Desenha linhas de ângulo
              ctx.strokeStyle = 'rgba(0, 255, 0, 0.2)';
              for (let a = 0; a < 180; a += 30) {
                  const rad = (a - 90) * Math.PI / 180;
                  ctx.beginPath();
                  ctx.moveTo(centerX, centerY);
                  ctx.lineTo(centerX + radius * Math.cos(rad), centerY + radius * Math.sin(rad));
                  ctx.stroke();
                  ctx.fillStyle = 'rgba(0, 255, 0, 0.7)';
                  ctx.fillText(a + '°', centerX + (radius + 15) * Math.cos(rad), centerY + (radius + 15) * Math.sin(rad));
              }
              
              // Desenha pontos detectados (vermelho se alerta ativo, verde se não)
              for (let a = 0; a <= 180; a++) {
                  if (distances[a] > 0) {
                      const dist = (distances[a] / 100) * radius;
                      const rad = (a - 90) * Math.PI / 180;
                      const x = centerX + dist * Math.cos(rad);
                      const y = centerY + dist * Math.sin(rad);
                      
                      ctx.fillStyle = alerts[a] ? 'rgba(255, 0, 0, 0.7)' : 'rgba(0, 255, 0, 0.7)';
                      ctx.beginPath();
                      ctx.arc(x, y, 3, 0, Math.PI * 2);
                      ctx.fill();
                  }
              }
              
              requestAnimationFrame(drawSonar);
          }
          
          drawSonar();
      </script>
  </body>
  </html>
  )=====";
  
  server.send(200, "text/html", html);
}