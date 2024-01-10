
#include <WiFi.h>
#include <ArduinoMqttClient.h>


// WiFi

const char* ssid = "td";   // network SSID (name) 
const char* password = "imelab2lolada";   // network password

void setup_wifi() {

  delay(10);

  // connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // disable WiFi sleep mode
  WiFi.setSleep(WIFI_PS_NONE);

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

}

// MQTT

WiFiClient espClient;

MqttClient mqttClient(espClient);
const char broker[] = "172.20.10.2";
int        port     = 1883;


// AFE

#define ADC_PIN 34

//#define pre_offset 1.60714
//#define pre_gain 304

#define pre_offset 1.60
#define pre_gain 176

#define pos_gain 6.21978
#define pos_offset 9.6577654

// buffer for samples
float x[50] = {0};

float sum = 0.0;
double in_v = 0.0;
float samples_avg = 0.0;
float spl_value = 0.0;

int lastTimePrinted = 0;


void setup() {

  Serial.begin(115200);
  
  // WiFi setup
  setup_wifi();

  //MQTT setup
  while(!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
  } 
  Serial.println("You're connected to the MQTT broker!");
  Serial.println();

}

void loop() {
  // put your main code here, to run repeatedly:
  
  mqttClient.poll();

  // shift the buffer
  for(int i = 49; i > 0; i--) {
    x[i] = x[i-1];
  }

  //take new sample
  x[0] = (float) analogRead(ADC_PIN)*3.3/4095;

  //average last 50 samples
  for(int j = 0; j<50; j++){
    sum = sum + x[j];
  }
  samples_avg = sum/50;

  //calculate original mic signal amplitude
  in_v = (double) ((((samples_avg + pos_offset)/pos_gain)-pre_offset)/pre_gain);

  spl_value = (float) 20*log10((in_v/((6.31*0.001)*(20*0.000001))));

  sum = 0;

    if (millis() - lastTimePrinted >= 1000) {
      // Serial port to show in GUI
      Serial.println(spl_value);

      char spl_value_string[8];
      dtostrf(spl_value, 1, 2, spl_value_string);

      // Construct the MQTT message
      char spl_message[20]; // Adjust the size based on your needs
      snprintf(spl_message, sizeof(spl_message), "audio spl=%s", spl_value_string);

      // Publish the message

      mqttClient.beginMessage("ems/t3/g10");  //topic
      mqttClient.print(spl_message);
      mqttClient.endMessage();

      lastTimePrinted = millis();
    }

}