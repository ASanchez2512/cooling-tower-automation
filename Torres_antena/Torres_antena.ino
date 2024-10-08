#include <WiFi.h>
#include <esp_now.h>
#include <WebServer.h>

// Define your Wi-Fi credentials
const char* ssid = "HONOR Magic5 Lite 5G";
const char* password = "samiwa777";

WebServer server(80);


TaskHandle_t Task1;

// ESP-NOW broadcast address (MAC of the target device)
uint8_t broadcastAddress[] = {0x70, 0xB8, 0xF6, 0x5C, 0x73, 0xD0};

typedef struct packet_in{
    float   temp_in;
    float   temp_out;
    float   temp_air;
    float   humedad;
    uint8_t ventilador;
    uint8_t caudal;
} packet_in;

typedef struct packet_out{
    uint8_t ventilador;
    uint8_t caudal;
    char instruction;
} packet_out;

packet_in data_in;
packet_out data_out;

void handleRoot() {
  server.send(200, "text/plain", "Hello from ESP32!");
}

void handleT_I() {
  String temperature = String(data_in.temp_in);  // Example temperature data
  server.send(200, "text/plain", temperature);
}

void handleT_O() {
  String temperature = String(data_in.temp_out);  // Example temperature data
  server.send(200, "text/plain", temperature);
}

void handleT_A() {
  String temperature = String(data_in.temp_air);  // Example temperature data
  server.send(200, "text/plain", temperature);
}

void handleHumidity() {
  String humidity = String(data_in.humedad);  // Example humidity data
  server.send(200, "text/plain", humidity);
}


void handleFan() {
  if (server.hasArg("value")) {
    int value = server.arg("value").toInt();

    // Ensure the value is within the valid range
    switch (value){
      case 1:
        if (data_out.ventilador < 100) data_out.ventilador += 10;
        server.send(200, "text/plain", "Sent: Fan speed up");
        break;
      case 0:
        if (data_out.ventilador > 0) data_out.ventilador -= 10;
        server.send(200, "text/plain", "Sent: Fan speed down");
        break;
      default:
        server.send(200, "text/plain", "Invalid");
        break;
    }
  } else {
    String ventilador = String(data_out.ventilador);  // Example temperature data
    server.send(200, "text/plain", ventilador);
  }
}

void handlePump() {
  if (server.hasArg("value")) {
    int value = server.arg("value").toInt();

    // Ensure the value is within the valid range
    switch (value){
      case 1:
        if (data_out.caudal < 100) data_out.caudal += 10;
        server.send(200, "text/plain", "Sent: Pump speed up");
        break;
      case 0:
        if (data_out.caudal > 0) data_out.caudal -= 10;
        server.send(200, "text/plain", "Sent: Pump speed down");
        break;
      default:
        server.send(200, "text/plain", "Invalid");
        break;
    }
  } else {
    String caudal = String(data_out.caudal);  // Example temperature data
    server.send(200, "text/plain", caudal);
  }
}

void handleInstruction() {
  if (server.hasArg("value")) {
    uint8_t value = server.arg("value").toInt();

    switch (value){
      case 1:
        data_out.instruction = 2;
        server.send(200, "text/plain", "Sent: Paro");
        break;
      case 0:
        data_out.instruction = 1;
        server.send(200, "text/plain", "Sent: Inicio");
        break;
      default:
        data_out.instruction = '\0';
        server.send(200, "text/plain", "No such instruction");
        break;
    }
  } else {
    server.send(200, "text/plain", "No instruction provided");
  }
}


// Callback function when ESP-NOW data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status != ESP_NOW_SEND_SUCCESS) {
      data_out.instruction = 'D';
    }
}

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&data_in, incomingData, sizeof(data_in));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("T_in: ");
  Serial.println(data_in.temp_in);
  Serial.print("T_out: ");
  Serial.println(data_in.temp_out);
  Serial.print("T_air: ");
  Serial.println(data_in.temp_air);
  Serial.print("Humedad: ");
  Serial.println(data_in.humedad);
  Serial.print("Ventilador: ");
  Serial.println(data_out.ventilador);
  Serial.print("Caudal: ");
  Serial.println(data_out.caudal);
  Serial.println();
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);

  // Initialize Wi-Fi and connect to network
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.print(".");
  }
  Serial.println(' ');
  Serial.println("Connected to WiFi!");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/T_I", handleT_I);
  server.on("/T_O", handleT_O);
  server.on("/T_A", handleT_A);
  server.on("/H", handleHumidity);

  server.on("/F", handleFan);
  server.on("/P", handlePump);
  server.on("/I", handleInstruction);

  server.begin();
  Serial.println("HTTP server started");

  data_out.ventilador = 0;
  data_out.caudal = 0;
  data_out.instruction = '\0';

  data_in.temp_in = 0.0f;
  data_in.temp_out = 0.0f;
  data_in.temp_air = 0.0f;
  data_in.humedad = 0.0f;
  data_in.ventilador = 100;
  data_in.caudal = 100;

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
      Serial.println("Error initializing ESP-NOW");
      return;
  }

  // Register the send callback function
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  // Add the peer (target device for ESP-NOW)
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  // Channel 0 means it will match the current Wi-Fi channel
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Failed to add peer");
      return;
  }

  xTaskCreatePinnedToCore(
                    Task1code,   // Task function.
                    "Data out",     // name of task.
                    1000,       // Stack size of task
                    NULL,        // parameter of the task
                    tskIDLE_PRIORITY,           // priority of the task
                    &Task1,      // Task handle to keep track of created task
                    0);          // pin task to core 1

}

void loop() {
    server.handleClient();
}


void Task1code(void * parameter) {
  for(;;) {
    // Send ESP-NOW data
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &data_out, sizeof(data_out));

    // Wait
    delay(500);
  }
}