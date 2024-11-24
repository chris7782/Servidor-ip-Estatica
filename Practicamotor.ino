#include <WiFi.h>
#include <WebServer.h>

// Configuración de red WiFi
const char *ssid = "UPPue-WiFi";
const char *password = "";

// Configuración de IP estática
IPAddress ip(192, 168, 64, 50);
IPAddress gateway(192, 168, 64, 1);
IPAddress subnet(255, 255, 255, 0);

// Pin de control del relé (foco)
const int relePin = 23; // Pin donde está conectado el relé

// Pines del motor paso a paso
#define IN1 32
#define IN2 33
#define IN3 25
#define IN4 26

// Secuencia de pasos para el motor 28BYJ-48 (8 pasos por ciclo)
const int stepsSequence[8][4] = {
  {1, 0, 0, 0},
  {1, 1, 0, 0},
  {0, 1, 0, 0},
  {0, 1, 1, 0},
  {0, 0, 1, 0},
  {0, 0, 1, 1},
  {0, 0, 0, 1},
  {1, 0, 0, 1}
};

int stepDelay = 3;  // Delay entre pasos (ms)
int currentStep = 0;
bool direction = true;  // Dirección inicial: true = horario, false = antihorario
int stepsToMove = 0;  // Número de pasos a ejecutar

// Crear servidor web
WebServer server(80);

// Prototipos de funciones
void moveMotor(int steps, bool forward);
void stepMotor(bool forward);

// Página HTML
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Control ESP32</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      display: flex;
      justify-content: center;
      align-items: center;
      height: 100vh;
      margin: 0;
      background-color: #00CCFF;
    }
    .content {
      text-align: center;
      background-color: white;
      padding: 20px;
      border-radius: 8px;
      box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
      width: 80%;
      max-width: 600px;
    }
    h1, h2 {
      color: #333;
    }
    button {
      padding: 10px 20px;
      font-size: 16px;
      margin: 10px;
      cursor: pointer;
      background-color: #4CAF50;
      color: white;
      border: none;
      border-radius: 5px;
    }
    button:hover {
      background-color: #45a049;
    }
    form {
      margin-top: 20px;
    }
    input[type="text"], input[type="number"], select {
      padding: 8px;
      font-size: 16px;
      width: 100%;
      margin: 5px 0;
      border-radius: 5px;
      border: 1px solid #ccc;
    }
    input[type="submit"] {
      background-color: #4CAF50;
      color: white;
      padding: 10px 20px;
      border: none;
      border-radius: 5px;
      cursor: pointer;
    }
    input[type="submit"]:hover {
      background-color: #45a049;
    }
    /* Estilo para las imágenes en las esquinas */
    .top-left {
      position: absolute;
      top: 10px;
      left: 10px;
      width: 180px;
      height: auto;
    }
    .top-right {
      position: absolute;
      top: 10px;
      right: 10px;
      width: 180px;
      height: auto;
    }
  </style>
</head>
<body>
  <img src="https://pbs.twimg.com/profile_images/1495410579967721481/6tQlzI0n_200x200.jpg" alt="Logo Mecatrónica UPPuebla 1" class="top-left">
  <img src="https://www.uppuebla.edu.mx/wp-content/uploads/2022/08/cropped-LOGO-UPP-HD-300x300.png" alt="Logo Mecatrónica UPPuebla 2" class="top-right">
  
  <div class="content">
    <h1>Control del ESP32</h1>
    <h2>Control de Foco</h2>
    <button onclick="location.href='/on'">Encender</button>
    <button onclick="location.href='/off'">Apagar</button>
    <h2>Control de Motor</h2>
    <form action="/motor" method="get">
      <label for="direction">Direccion:</label>
      <select name="direction" id="direction">
        <option value="horario">Horario</option>
        <option value="antihorario">Antihorario</option>
      </select><br><br>
      <label for="steps">Pasos:</label>
      <input type="number" id="steps" name="steps" min="1" required><br><br>
      <input type="submit" value="Mover Motor">
    </form>
    <h2>Configuracion de IP</h2>
    <form action="/config" method="get">
      <label for="ipSelect">Tipo de IP:</label>
      <select name="ipSelect" id="ipSelect">
        <option value="static">Estatica</option>
        <option value="dynamic">Dinamica</option>
      </select><br><br>
      <label for="ip">IP:</label>
      <input type="text" id="ip" name="ip" placeholder="192.168.1.100"><br><br>
      <label for="gateway">Gateway:</label>
      <input type="text" id="gateway" name="gateway" placeholder="192.168.1.1"><br><br>
      <label for="subnet">Subnet:</label>
      <input type="text" id="subnet" name="subnet" placeholder="255.255.255.0"><br><br>
      <input type="submit" value="Aplicar">
    </form>
  </div>
</body>
</html>
)rawliteral";

// Funciones del servidor
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleOn() {
  digitalWrite(relePin, HIGH);
  Serial.println("Foco encendido.");
  server.send(200, "text/html", htmlPage);
}

void handleOff() {
  digitalWrite(relePin, LOW);
  Serial.println("Foco apagado.");
  server.send(200, "text/html", htmlPage);
}

void handleConfigIP() {
  String ipSelect = server.arg("ipSelect");
  if (ipSelect == "static") {
    String ipStr = server.arg("ip");
    String gatewayStr = server.arg("gateway");
    String subnetStr = server.arg("subnet");

    IPAddress newIP, newGateway, newSubnet;
    newIP.fromString(ipStr);
    newGateway.fromString(gatewayStr);
    newSubnet.fromString(subnetStr);

    WiFi.config(newIP, newGateway, newSubnet);
    Serial.print("Configuración estática aplicada. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE); // DHCP
    Serial.println("Configuración dinámica aplicada. IP asignada por DHCP");
    unsigned long startMillis = millis();
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      if (millis() - startMillis > 30000) {
        Serial.println("No se pudo obtener la IP después de 30 segundos.");
        break;
      }
    }
    Serial.print("IP obtenida por DHCP: ");
    Serial.println(WiFi.localIP());
  }
  server.send(200, "text/html", htmlPage);
}

void handleMotor() {
  String directionStr = server.arg("direction");
  stepsToMove = server.arg("steps").toInt();

  if (directionStr == "horario") {
    direction = true;
    Serial.println("Dirección: Horario");
  } else if (directionStr == "antihorario") {
    direction = false;
    Serial.println("Dirección: Antihorario");
  }

  if (stepsToMove > 0) {
    Serial.print("Moviendo ");
    Serial.print(stepsToMove);
    Serial.println(" pasos.");
    moveMotor(stepsToMove, direction);
    stepsToMove = 0;
  }

  server.send(200, "text/html", htmlPage);
}

void moveMotor(int steps, bool forward) {
  for (int i = 0; i < steps; i++) {
    stepMotor(forward);
    delay(stepDelay);
  }
}

void stepMotor(bool forward) {
  currentStep = forward ? (currentStep + 1) % 8 : (currentStep + 7) % 8;
  digitalWrite(IN1, stepsSequence[currentStep][0]);
  digitalWrite(IN2, stepsSequence[currentStep][1]);
  digitalWrite(IN3, stepsSequence[currentStep][2]);
  digitalWrite(IN4, stepsSequence[currentStep][3]);
}

void setup() {
  Serial.begin(115200);

  pinMode(relePin, OUTPUT);
  digitalWrite(relePin, LOW);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);

  Serial.print("Conectando a WiFi: ");
  Serial.println(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConexión a WiFi exitosa.");
  Serial.print("IP asignada: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/on", HTTP_GET, handleOn);
  server.on("/off", HTTP_GET, handleOff);
  server.on("/motor", HTTP_GET, handleMotor);
  server.on("/config", HTTP_GET, handleConfigIP);

  server.begin();
}

void loop() {
  server.handleClient();
}

