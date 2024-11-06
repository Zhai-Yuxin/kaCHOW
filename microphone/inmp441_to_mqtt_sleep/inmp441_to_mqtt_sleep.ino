#include "wav_header.h"
#include <PubSubClient.h>
#include <WiFi.h>
#include <driver/i2s.h>
#include <SPIFFS.h>
#include "esp_eap_client.h"

#define I2S_WS 25
#define I2S_SD 32
#define I2S_SCK 33
#define I2S_PORT I2S_NUM_0
#define I2S_SAMPLE_RATE   16000
#define I2S_SAMPLE_BITS   16
#define I2S_READ_LEN      16 * 1024
#define RECORD_TIME       1 //Seconds
#define I2S_CHANNEL_NUM   1
#define FLASH_RECORD_SIZE (I2S_CHANNEL_NUM * I2S_SAMPLE_RATE * I2S_SAMPLE_BITS / 8 * RECORD_TIME)

#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  20

#define LEDPIN 13
#define SOUNDPIN GPIO_NUM_35 // for KY-038

RTC_DATA_ATTR int bootCount = 0;

const char* ssid = "S22";
const char* password = "123456789";

WiFiClient espClient;
PubSubClient client(espClient);
  
const char* mqtt_server = "192.168.87.196";
const char* mqtt_topic = "voice/wav";

File file;
const char filename[] = "/recording.wav";

void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:     Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1:     Serial.println("Woken up by sound sensor"); break;
    case ESP_SLEEP_WAKEUP_TIMER:    Serial.println("Woken up by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Woken up by touch"); break;
    case ESP_SLEEP_WAKEUP_ULP:      Serial.println("Wakeup caused by ULP program"); break;
    default:                        Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(LEDPIN, OUTPUT);
 
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
  print_wakeup_reason();

  SPIFFSInit();
  i2sInit();
  i2s_adc();

  // Connect to wifi
  setup_wifi();

  // Connect to MQTT Server
  connect_mqtt();

  // Send recording to MQTT Server
  send_wav_to_mqtt();

  // Deep sleep
  Serial.println("Going to sleep now");

  // Wake up by time 
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");

  // Wake up by touch
  touchAttachInterrupt(T5, touch_isr_handler, 50);
  esp_sleep_enable_touchpad_wakeup();

  // Wake up by sound
  pinMode(SOUNDPIN, INPUT);
  esp_sleep_enable_ext1_wakeup(1ULL << SOUNDPIN, ESP_EXT1_WAKEUP_ANY_HIGH);

  Serial.flush();
  esp_deep_sleep_start();
}

void loop() {
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to Wifi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void touch_isr_handler() {
  Serial.println("Touch");
}

void connect_mqtt() {
  Serial.print("Connecting to MQTT: ");
  Serial.println(mqtt_server);
  client.setServer(mqtt_server, 1883);
  while (client.connect(mqtt_server) == false) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Connected to mqtt server");
}

void light(int time) {
  Serial.print("Blink for ");
  Serial.print(time);
  Serial.print(" ");
  Serial.println("ms");
  digitalWrite(LEDPIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(time);                      // wait for a second
  digitalWrite(LEDPIN, LOW);   // turn the LED off by making the voltage LOW
  delay(100); 
}

void SPIFFSInit() {
  if(!SPIFFS.begin(true)) {
    Serial.println("SPIFFS initialisation failed!");
    while(1) yield();
  }

  SPIFFS.remove(filename);
  file = SPIFFS.open(filename, FILE_WRITE);
  if(!file){
    Serial.println("File is not available!");
  }

  byte header[headerSize];
  wavHeader(header, FLASH_RECORD_SIZE);

  file.write(header, headerSize);
  listSPIFFS();
}

void i2sInit() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = I2S_SAMPLE_RATE,
    .bits_per_sample = i2s_bits_per_sample_t(I2S_SAMPLE_BITS),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = 0,
    .dma_buf_count = 64,
    .dma_buf_len = 1024,
    .use_apll = 1
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);

  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };

  i2s_set_pin(I2S_PORT, &pin_config);
}


void i2s_adc_data_scale(uint8_t * d_buff, uint8_t* s_buff, uint32_t len) {
    uint32_t j = 0;
    uint32_t dac_value = 0;
    for (int i = 0; i < len; i += 2) {
        dac_value = ((((uint16_t) (s_buff[i + 1] & 0xf) << 8) | ((s_buff[i + 0]))));
        d_buff[j++] = 0;
        d_buff[j++] = dac_value * 256 / 2048;
    }
}

void i2s_adc() {
    
    int i2s_read_len = I2S_READ_LEN;
    int flash_wr_size = 0;
    size_t bytes_read;

    char* i2s_read_buff = (char*) calloc(i2s_read_len, sizeof(char));
    uint8_t* flash_write_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));

    i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
    i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
    
    light(1500);
    Serial.println(" *** Recording Start *** ");
    while (flash_wr_size < FLASH_RECORD_SIZE) {
        //read data from I2S bus, in this case, from ADC.
        i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
        //save original data from I2S(ADC) into flash.
        i2s_adc_data_scale(flash_write_buff, (uint8_t*)i2s_read_buff, i2s_read_len);
        file.write((const byte*) flash_write_buff, i2s_read_len);
        flash_wr_size += i2s_read_len;
        printf("Sound recording %u%%\n", flash_wr_size * 100 / FLASH_RECORD_SIZE);
        printf("Never Used Stack Size: %u\n", uxTaskGetStackHighWaterMark(NULL));
    }
    light(300);
    light(300);
    file.close();
    free(i2s_read_buff);
    i2s_read_buff = NULL;
    free(flash_write_buff);
    flash_write_buff = NULL;
    
    listSPIFFS();
}

void listSPIFFS(void) {
  Serial.println(F("\r\nListing SPIFFS files:"));
  static const char line[] PROGMEM =  "=================================================";

  Serial.println(FPSTR(line));
  Serial.println(F("  File name                              Size"));
  Serial.println(FPSTR(line));

  fs::File root = SPIFFS.open("/");
  if (!root) {
    Serial.println(F("Failed to open directory"));
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(F("Not a directory"));
    return;
  }

  fs::File file = root.openNextFile();
  while (file) {

    if (file.isDirectory()) {
      Serial.print("DIR : ");
      String fileName = file.name();
      Serial.print(fileName);
    } else {
      String fileName = file.name();
      Serial.print("  " + fileName);
      // File path can be 31 characters maximum in SPIFFS
      int spaces = 33 - fileName.length(); // Tabulate nicely
      if (spaces < 1) spaces = 1;
      while (spaces--) Serial.print(" ");
      String fileSize = (String) file.size();
      spaces = 10 - fileSize.length(); // Tabulate nicely
      if (spaces < 1) spaces = 1;
      while (spaces--) Serial.print(" ");
      Serial.println(fileSize + " bytes");
    }

    file = root.openNextFile();
  }

  Serial.println(FPSTR(line));
  Serial.println();
  delay(1000);
}

void send_wav_to_mqtt() {
  File file = SPIFFS.open(filename, "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  size_t fileSize = file.size();
  Serial.println(fileSize);
  const size_t chunkSize = 128;  // Adjust chunk size if needed
  size_t bytesRead;
  byte buffer[chunkSize];
  Serial.println("Sending chunks over");

  while ((bytesRead = file.read(buffer, chunkSize)) > 0) {
    if (client.connected()) {
      client.publish(mqtt_topic, (const uint8_t*)buffer, bytesRead);
      Serial.printf("Published %d bytes\n", bytesRead);
    }
  }

  file.close();
  Serial.println("File sent over to MQTT");

  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1) {
    String message = "check crying";
    client.publish("voice/stop", message.c_str());
  } else if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TOUCHPAD) {
    String message = "check voice command";
    client.publish("voice/stop", message.c_str());
  } else {
    String message = "check crying";
    client.publish("voice/stop", message.c_str());    
  }
  
  listSPIFFS();
}