#define PUMP_1    4
#define PUMP_2    0
#define FAN       2
#define RES       15
#define BUTTON    16

#define TEMP1     26
#define TEMP2     27
#define DHTPIN    14

#define DHTTYPE   DHT11

#include <OneWire.h>                
#include <DallasTemperature.h>
#include "DHT.h"
#include <WiFi.h>
#include <esp_now.h>

/*/////////////////////////////////////////////////////////////
//                Declaracion de Variables                   //
/////////////////////////////////////////////////////////////*/
float temp1 = 0.0f;
float temp2 = 0.0f;
float temp_aire = 0.0f;
float humedad = 0.0f;

uint8_t fan = 0, pump = 0;

TaskHandle_t Task1;
TaskHandle_t Task2;
TaskHandle_t Task3;
TaskHandle_t Proceso1;

bool input = true;
bool exhaust = false;

// ESP-NOW broadcast address (MAC of the target device)
uint8_t broadcastAddress[] = {0x0C, 0xB8, 0x15, 0x75, 0xE5, 0x94};

// Define your Wi-Fi credentials
const char* ssid = "HONOR Magic5 Lite 5G";
const char* password = "samiwa777";

typedef struct packet_in{
    uint8_t ventilador;
    uint8_t caudal;
    char instruction;
} packet_in;

typedef struct packet_out{
    float   temp_in;
    float   temp_out;
    float   temp_air;
    float   humedad;
    uint8_t ventilador;
    uint8_t caudal;
} packet_out;

packet_in data_in;
packet_out data_out;

/*/////////////////////////////////////////////////////////////
//                Declaracion de Sensores                    //
/////////////////////////////////////////////////////////////*/

OneWire ourWire1(TEMP1);                //Se establece el pin 2  como bus OneWire
OneWire ourWire2(TEMP2);

DallasTemperature sensor1(&ourWire1); //Se declara una variable u objeto para nuestro sensor
DallasTemperature sensor2(&ourWire2); //Se declara una variable u objeto para nuestro sensor
DHT dht(DHTPIN, DHTTYPE);

/*/////////////////////////////////////////////////////////////
//                 Funciones IO de ESPNOW                    //
/////////////////////////////////////////////////////////////*/

// Callback function when ESP-NOW data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  //Serial.print("Last Packet Send Status: ");
  //Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&data_in, incomingData, sizeof(data_in));

  Serial.print("V: ");
  Serial.println(fan);
  Serial.print("C: ");
  Serial.println(pump);

  switch (data_in.instruction){
  case 'P':
    input = false;
    exhaust = false;
    break;
  case 'I':
    input = !input;
    break;
  case 'O':
    exhaust = !exhaust;
    break;
  case 'D':
    fan = data_in.ventilador;
    pump = data_in.caudal;
    break;
  default:
    break;
  }
}

/*/////////////////////////////////////////////////////////////
//                   Funcion de Inicio                       //
/////////////////////////////////////////////////////////////*/
void setup() {

  sensor1.begin();   //Se inicia el sensor
  sensor2.begin();
  dht.begin();

  pinMode(PUMP_1, OUTPUT);
  pinMode(PUMP_2, OUTPUT);
  pinMode(FAN, OUTPUT);
  pinMode(RES, OUTPUT);

  pinMode(BUTTON, INPUT);

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
                    "Sensor_Tin",     // name of task.
                    1000,       // Stack size of task
                    NULL,        // parameter of the task
                    tskIDLE_PRIORITY,           // priority of the task
                    &Task1,      // Task handle to keep track of created task
                    0);          // pin task to core 1
  
  xTaskCreatePinnedToCore(
                    Task2code,   // Task function.
                    "Sensor_Tout",     // name of task.
                    1000,       // Stack size of task
                    NULL,        // parameter of the task
                    tskIDLE_PRIORITY,           // priority of the task
                    &Task2,      // Task handle to keep track of created task
                    0);          // pin task to core 1

  xTaskCreatePinnedToCore(
                    Task3code,   // Task function.
                    "SensorAire",     // name of task.
                    2000,       // Stack size of task
                    NULL,        // parameter of the task
                    tskIDLE_PRIORITY,           // priority of the task
                    &Task3,      // Task handle to keep track of created task
                    0);          // pin task to core 1
  
  xTaskCreatePinnedToCore(
                    TareaControl1,   // Task function.
                    "Proceso",     // name of task.
                    10000,       // Stack size of task
                    NULL,        // parameter of the task
                    tskIDLE_PRIORITY,           // priority of the task
                    &Proceso1,      // Task handle to keep track of created task
                    0);          // pin task to core 1

}


/*/////////////////////////////////////////////////////////////
//                   Funcion de Bucle                        //
/////////////////////////////////////////////////////////////*/
void loop() {
  
  if (input){
    analogWrite(PUMP_1, pump * 2);
    analogWrite(FAN, fan * 2);
    Serial.print("active");
  } else {
    analogWrite(PUMP_1, 0);
    analogWrite(FAN, 0);
  }

  digitalWrite(PUMP_2, (exhaust)? HIGH : LOW);

  // Send ESP-NOW data
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &data_out, sizeof(data_out));

  if (result == ESP_OK) {
      Serial.println("Data sent successfully");
  } else {
      Serial.println("Error sending data");
  }

  delay(500);
}

uint8_t percent(uint8_t n){
  return (uint8_t)(((int)n * 255) / 100);
}

/*/////////////////////////////////////////////////////////////
//                  Funciones de Sensores                    //
/////////////////////////////////////////////////////////////*/
void Task1code(void * parameter) {
  for(;;) {
    sensor1.requestTemperatures();   //Se envía el comando para leer la temperatura
    data_out.temp_in = sensor1.getTempCByIndex(0); //Se obtiene la temperatura en ºC
  }
}

void Task2code(void * parameter) {
  for(;;) {
    sensor2.requestTemperatures();   //Se envía el comando para leer la temperatura
    data_out.temp_out = sensor2.getTempCByIndex(0); //Se obtiene la temperatura en ºC
  }
}

void Task3code(void * parameter) {
  for(;;) {
    data_out.humedad = dht.readHumidity();
    data_out.temp_air = dht.readTemperature();
  }
}


/*/////////////////////////////////////////////////////////////
//                  Funciones de Control                     //
/////////////////////////////////////////////////////////////*/
void TareaControl1(void * parameter){
  for(;;){
    if(temp1 < 50.0f && temp1 > 0.0f){
      digitalWrite(RES, HIGH);
    } else {
      digitalWrite(RES, LOW);
    }
    delay(100);
  }
}