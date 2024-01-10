
#include <driver/i2s.h>
#include <WiFi.h>
#include <ArduinoMqttClient.h>


// I2S

#define I2S_SAMPLE_RATE (48000)
#define ADC_INPUT (ADC1_CHANNEL_7) //pin (GPIO 35)
#define I2S_DMA_BUF_LEN (1024)

// The 4 high bits are the channel, and the data is inverted

size_t bytes_read;
uint16_t buffer[I2S_DMA_BUF_LEN] = {0};

unsigned long lastTimePrinted;
unsigned long loopTime = 0;

void i2sInit() {

  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),

    .sample_rate =  I2S_SAMPLE_RATE,              // The format of the signal using ADC_BUILT_IN

    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // is fixed at 12bit, stereo, MSB

    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,

    .communication_format = I2S_COMM_FORMAT_I2S_MSB,

    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,

    .dma_buf_count = 2,

    .dma_buf_len = I2S_DMA_BUF_LEN,

    .use_apll = false,

    .tx_desc_auto_clear = false,

    .fixed_mclk = 0
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);

  i2s_set_adc_mode(ADC_UNIT_1, ADC_INPUT);

  i2s_adc_enable(I2S_NUM_0);

  adc1_config_channel_atten(ADC_INPUT, ADC_ATTEN_DB_11);
}



// A-Weighting filter

#define FILTER_ORDER 6
#define BUFFER_SIZE  7

// Filter coefficients

// numerator
float b[FILTER_ORDER + 1] = {0.169994948147430, 0.280415310498794, -1.120574766348363,
                             0.131562559965936, 0.974153561246036, -0.282740857326553, -0.152810756202003};

// denominator
float a[FILTER_ORDER + 1] = {1.000000000000000, -2.12979364760736134, 0.42996125885751674,
                             1.62132698199721426, -0.96669962900852902, 0.00121015844426781, 0.04400300696788968};


float inputBuffer[FILTER_ORDER + 1] = {0};  // Input buffer for storing past samples
float outputBuffer[FILTER_ORDER + 1] = {0}; // Output buffer for storing past output samples


float applyFilter(float input) {

  // Shift the input and output buffer
  for (int i = BUFFER_SIZE - 1; i > 0; i--) {
    inputBuffer[i] = inputBuffer[i - 1];
    outputBuffer[i] = outputBuffer[i - 1];
  }

  // Add the new input to the buffer
  inputBuffer[0] = input;

  // Calculate the filtered output using the difference equation
  // Update the first element of the output buffer with the new output
  outputBuffer[0] = 
         b[0] * inputBuffer[0] +
         b[1] * inputBuffer[1] - a[1] * outputBuffer[1] +
         b[2] * inputBuffer[2] - a[2] * outputBuffer[2] +
         b[3] * inputBuffer[3] - a[3] * outputBuffer[3] +
         b[4] * inputBuffer[4] - a[4] * outputBuffer[4] +
         b[5] * inputBuffer[5] - a[5] * outputBuffer[5];      
  
  return outputBuffer[0];

}


// Variables
float input;
float filter_output;


// Pre-Amplifier
float offset = 1.607;
int preamp_gain = 304;


// RMS detector
float sum_squared = 0.0;
float rms_value = 0.0;

// others
float amplitude;
float spl_value;



// WiFi

const char* ssid = "td";   // network SSID (name) 
const char* password = "imelab2lolada";   // network password

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
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
const char broker[] = "172.20.10.2"; // broker ip address
int        port     = 1883;          // broker port


void setup() {

  Serial.begin(115200);

  i2sInit();

  setup_wifi();

  while(!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
  } 

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();

}

void loop() {

  mqttClient.poll();
 
  // Get I2S data and place in data buffer
  size_t bytesIn = 0;
  esp_err_t result = i2s_read(I2S_NUM_0, &buffer, sizeof(buffer), &bytesIn, 0);
 
  if (result == ESP_OK)
  {
    // Read I2S data buffer
    int16_t samples_read = bytesIn / 8;

    if (samples_read > 0) {

      sum_squared = 0;

      for (int16_t i = 0; i < samples_read; ++i) {

        //Convert read value into voltage 
        input = (float)(buffer[i] * 3.3 / 65535); // 65535 = 2^16 - 1 because 16 bit ADC

        //Apply A-Filter
        filter_output = applyFilter(input - offset);

        //Calculate sum of all squares
        sum_squared += filter_output * filter_output;
      }

      // Average the rms values readings
      rms_value = sqrt(sum_squared/samples_read);

      //Calculate amplitude of mic signal assuming sine wave
      amplitude = (rms_value * sqrt(2)) / preamp_gain;

      //Calculate SPL value based on mic sensitivity
      spl_value = (float) 20*log10(amplitude/((6.31*0.001)*(20*0.000001)));
 
      // Print to serial plotter
      Serial.println(spl_value);

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
  }
}
