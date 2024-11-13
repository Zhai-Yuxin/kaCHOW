#include "key.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "pitches.h"
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#define LED1 12
#define LED2 13
#define LED3 32
#define LED4 33
#define LED5 25
#define BUZZER 14
#define BUZZER_CHANNEL 10
#define AVOIDANCE1 27
#define AVOIDANCE2 26
#define MOTOR1_IN1 18
#define MOTOR1_IN2 19
#define MOTOR2_IN3 16
#define MOTOR2_IN4 17
#define MOTOR1_CHANNEL_A 0  // PWM channel for forward direction
#define MOTOR1_CHANNEL_B 1  // PWM channel for reverse direction  
#define MOTOR2_CHANNEL_A 2  // PWM channel for forward direction
#define MOTOR2_CHANNEL_B 3  // PWM channel for reverse direction
#define MOTOR_FREQ 5000     // PWM frequency
#define RESOLUTION 8  // PWM resolution (8-bit: 0-255)
#define SERVO 5 
#define SERVO_CHANNEL 4
#define SERVO_FREQ 50
#define SERVO_RESOLUTION 16
// connect LCD_SDA 21
// connect LCD_SCL 22

// flags
volatile int control = 0;
volatile bool front_obstacle = 0;
volatile bool back_obstacle = 0;
volatile bool front_buzz = 0;
volatile bool back_buzz = 0;
volatile bool music = 0;
volatile bool wave = 0;
volatile int check = 10;

LiquidCrystal_I2C lcd(0x27,16,2);
volatile int latest_command = 0;

WiFiClientSecure espClient = WiFiClientSecure();
PubSubClient client(espClient);

volatile int pos = 0;
volatile int change = 10;

volatile unsigned long start_time = 0;

int melody[] = {
  NOTE_E5, NOTE_E5, NOTE_E5,
  NOTE_E5, NOTE_E5, NOTE_E5,
  NOTE_E5, NOTE_G5, NOTE_C5, NOTE_D5,
  NOTE_E5,
  NOTE_F5, NOTE_F5, NOTE_F5, NOTE_F5,
  NOTE_F5, NOTE_E5, NOTE_E5, NOTE_E5, NOTE_E5,
  NOTE_E5, NOTE_D5, NOTE_D5, NOTE_E5,
  NOTE_D5, NOTE_G5
};
// note durations: 4 = quarter note, 8 = eighth note, etc, also called tempo:
int melody_durations[] = {
  8, 8, 4,
  8, 8, 4,
  8, 8, 8, 8,
  2,
  8, 8, 8, 8,
  8, 8, 8, 16, 16,
  8, 8, 8, 8,
  4, 4
};


int front_alert = NOTE_G5;
int back_alert = NOTE_E5;

char *subscribeTopic = "kachow/control";
char *publishTopic = "kachow/interrupt";

void led(void * pvParameters) {
    while (1) {
        if (control == 0) {  // stop
            digitalWrite(LED1, HIGH);
            digitalWrite(LED2, HIGH);
            digitalWrite(LED3, HIGH);
            digitalWrite(LED4, HIGH);
            digitalWrite(LED5, HIGH);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        } else if (control == 1) {  // straight
            digitalWrite(LED1, HIGH); 
            digitalWrite(LED2, LOW); 
            digitalWrite(LED3, LOW); 
            digitalWrite(LED4, LOW); 
            digitalWrite(LED5, LOW); 
            vTaskDelay(300 / portTICK_PERIOD_MS);
            if (control == 1) {
                digitalWrite(LED1, LOW); 
                digitalWrite(LED2, HIGH);
                vTaskDelay(300 / portTICK_PERIOD_MS);
            }
            if (control == 1) {
                digitalWrite(LED2, LOW); 
                digitalWrite(LED3, HIGH);
                vTaskDelay(300 / portTICK_PERIOD_MS);
            }
            if (control == 1) {
                digitalWrite(LED3, LOW); 
                digitalWrite(LED4, HIGH);
                vTaskDelay(300 / portTICK_PERIOD_MS);
            }
            if (control == 1) {
                digitalWrite(LED4, LOW); 
                digitalWrite(LED5, HIGH);
                vTaskDelay(300 / portTICK_PERIOD_MS);
            }
        } else if (control == 2) {  // reverse
            digitalWrite(LED1, HIGH); 
            digitalWrite(LED2, HIGH);
            digitalWrite(LED3, LOW);
            digitalWrite(LED4, HIGH);
            digitalWrite(LED5, HIGH);
            vTaskDelay(300 / portTICK_PERIOD_MS);
            if (control == 2) {
                digitalWrite(LED4, LOW); 
                vTaskDelay(300 / portTICK_PERIOD_MS);
            }
            if (control == 2) {
                digitalWrite(LED3, HIGH); 
                digitalWrite(LED5, LOW);
                vTaskDelay(300 / portTICK_PERIOD_MS);
            }
            if (control == 2) {
                digitalWrite(LED4, HIGH); 
                digitalWrite(LED1, LOW);
                vTaskDelay(300 / portTICK_PERIOD_MS);
            }
            if (control == 2) {
                digitalWrite(LED5, HIGH); 
                digitalWrite(LED2, LOW);
                vTaskDelay(300 / portTICK_PERIOD_MS);
            }
            if (control == 2) {
                digitalWrite(LED1, HIGH); 
                vTaskDelay(300 / portTICK_PERIOD_MS);
            }
        } else if (control == 3) {  // left
            digitalWrite(LED1, LOW); 
            digitalWrite(LED2, LOW); 
            digitalWrite(LED3, HIGH); 
            digitalWrite(LED4, HIGH); 
            digitalWrite(LED5, LOW); 
            vTaskDelay(500 / portTICK_PERIOD_MS); 
            digitalWrite(LED3, LOW); 
            digitalWrite(LED4, LOW); 
            vTaskDelay(500 / portTICK_PERIOD_MS); 
        } else if (control == 4) {  // right
            digitalWrite(LED1, HIGH); 
            digitalWrite(LED2, HIGH); 
            digitalWrite(LED3, LOW); 
            digitalWrite(LED4, LOW); 
            digitalWrite(LED5, LOW); 
            vTaskDelay(500 / portTICK_PERIOD_MS); 
            digitalWrite(LED1, LOW); 
            digitalWrite(LED2, LOW); 
            vTaskDelay(500 / portTICK_PERIOD_MS); 
        } else {
            vTaskDelay(1000 / portTICK_PERIOD_MS); 
        }
    }
}

void motor(void * pvParameters) { 
    while (1) {
        if (control == 0) { // || (front_obstacle == 1) || (back_obstacle == 1)) {
            ledcWriteChannel(MOTOR1_CHANNEL_A, 0);
            ledcWriteChannel(MOTOR1_CHANNEL_B, 0);
            ledcWriteChannel(MOTOR2_CHANNEL_A, 0);
            ledcWriteChannel(MOTOR2_CHANNEL_B, 0);
            vTaskDelay(500 / portTICK_PERIOD_MS); 
        } else if (control == 1) {
            ledcWriteChannel(MOTOR1_CHANNEL_A, 225);
            ledcWriteChannel(MOTOR1_CHANNEL_B, 0);
            ledcWriteChannel(MOTOR2_CHANNEL_A, 220);
            ledcWriteChannel(MOTOR2_CHANNEL_B, 0);
            if (check > 0) {
                vTaskDelay(200 / portTICK_PERIOD_MS); 
                check = check - 1;
            } else {
                control = 0;
            }
        } else if (control == 2) {
            ledcWriteChannel(MOTOR1_CHANNEL_A, 0);
            ledcWriteChannel(MOTOR1_CHANNEL_B, 225);
            ledcWriteChannel(MOTOR2_CHANNEL_A, 0);
            ledcWriteChannel(MOTOR2_CHANNEL_B, 220);
            if (check > 0) {
                vTaskDelay(200 / portTICK_PERIOD_MS); 
                check = check - 1;
            } else {
                control = 0;
            }
        } else if (control == 3) {
            ledcWriteChannel(MOTOR1_CHANNEL_A, 170);
            ledcWriteChannel(MOTOR1_CHANNEL_B, 0);
            ledcWriteChannel(MOTOR2_CHANNEL_A, 220);
            ledcWriteChannel(MOTOR2_CHANNEL_B, 0);
            if (check > 0) {
                vTaskDelay(200 / portTICK_PERIOD_MS); 
                check = check - 1;
            } else {
                control = 0;
            }
        } else if (control == 4) {
            ledcWriteChannel(MOTOR1_CHANNEL_A, 220);
            ledcWriteChannel(MOTOR1_CHANNEL_B, 0);
            ledcWriteChannel(MOTOR2_CHANNEL_A, 170);
            ledcWriteChannel(MOTOR2_CHANNEL_B, 0);
            if (check > 0) {
                vTaskDelay(200 / portTICK_PERIOD_MS); 
                check = check - 1;
            } else {
                control = 0;
            }
        } else {
            vTaskDelay(1000 / portTICK_PERIOD_MS); 
        }
    }
}

void buzz(void * pvParameters) {
    while (1) {
        // if (front_obstacle || back_obstacle) {
        if (music) {
            int size = sizeof(melody_durations) / sizeof(int);
            for (int thisNote = 0; thisNote < size; thisNote++) {
                int noteDuration = 1000 / melody_durations[thisNote];
                ledcWriteTone(BUZZER, melody[thisNote]);
                ledcWrite(BUZZER, 127);
                vTaskDelay(noteDuration / portTICK_PERIOD_MS);
                ledcWrite(BUZZER, 0);
                int pauseBetweenNotes = noteDuration * 1.30;
                vTaskDelay(pauseBetweenNotes / portTICK_PERIOD_MS);
            }
            music = 0;
        } else if (front_buzz) {
            ledcWriteTone(BUZZER, front_alert);
            ledcWrite(BUZZER, 127);
            vTaskDelay(500 / portTICK_PERIOD_MS);
            ledcWrite(BUZZER, 0);
            vTaskDelay(500 / portTICK_PERIOD_MS);
        } else if (back_buzz) {
            ledcWriteTone(BUZZER, back_alert);
            ledcWrite(BUZZER, 127);
            vTaskDelay(500 / portTICK_PERIOD_MS);
            ledcWrite(BUZZER, 0);
            vTaskDelay(500 / portTICK_PERIOD_MS);
        } else {
            vTaskDelay(500 / portTICK_PERIOD_MS); 
        }
    }
}

void avoidance1(void * pvParameters) {
    while (1) {
        bool obstacle = digitalRead(AVOIDANCE1);
        if (!obstacle) {
            front_buzz = 1;
            if (control != 2) {
              ledcWriteChannel(MOTOR1_CHANNEL_A, 0);
              ledcWriteChannel(MOTOR1_CHANNEL_B, 0);
              ledcWriteChannel(MOTOR2_CHANNEL_A, 0);
              ledcWriteChannel(MOTOR2_CHANNEL_B, 0);
              control = 0;
            }
            if (front_obstacle == 0) {
                while (!publishMessage("front obstacle detected")){
                    vTaskDelay(10 / portTICK_PERIOD_MS); 
                }
            }
            front_obstacle = 1;
        } else {
            front_buzz = 0;
            if (front_obstacle == 1) {
                while (!publishMessage("front obstacle cleared")){
                    vTaskDelay(100 / portTICK_PERIOD_MS); 
                }
            }
            front_obstacle = 0;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS); 
    }
}

void avoidance2(void * pvParameters) {
    while (1) {
        bool obstacle = digitalRead(AVOIDANCE2);
        if (!obstacle) {
            back_buzz = 1;
            if (control == 2) {
              ledcWriteChannel(MOTOR1_CHANNEL_A, 0);
              ledcWriteChannel(MOTOR1_CHANNEL_B, 0);
              ledcWriteChannel(MOTOR2_CHANNEL_A, 0);
              ledcWriteChannel(MOTOR2_CHANNEL_B, 0);
              control = 0;
            }
            if (back_obstacle == 0) {
                while (!publishMessage("back obstacle detected")){
                    vTaskDelay(100 / portTICK_PERIOD_MS); 
                }
            }
            back_obstacle = 1;
        } else {
            back_buzz = 0;
            if (back_obstacle == 1) {
                while (!publishMessage("back obstacle cleared")){
                    vTaskDelay(100 / portTICK_PERIOD_MS); 
                }
            }
            back_obstacle = 0;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS); 
    }
}

bool publishMessage(const String& message) {
    StaticJsonDocument<1024> pubMsg;
    pubMsg["interrupt"] = message;
    String payload;
    serializeJson(pubMsg, payload);
    if (!client.connected()) {
    Serial.println("MQTT Disconnected, trying to reconnect...");
    if (!client.connect(THINGNAME)) {
        Serial.println("Failed to reconnect, retrying in 1 second...");
        vTaskDelay(1000 / portTICK_PERIOD_MS); 
        return false;
    }
  }

  if (client.publish(publishTopic, payload.c_str())) {
    return true;
  } else {
    Serial.println("Failed to publish message.");
    return false;
  }
}


void display(void * pvParameters) {
    while (1) {
        lcd.setCursor(0, 0);
        lcd.print("Detected:");
        lcd.setCursor(0,1);
        if (latest_command == 0) {
            lcd.print("stop");
        } else if (latest_command == 1) {
            lcd.print("straight");
        } else if (latest_command == 2) {
            lcd.print("reverse");
        } else if (latest_command == 3) {
            lcd.print("left");
        } else if (latest_command == 4) {
            lcd.print("right");
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS); 
        lcd.clear();
    }
}

void servo(void * pvParameters) {
    while (1) {
        if (wave) {
            unsigned long curr_time = millis();
            if (curr_time - start_time < 4000) {
                int dutyCycle = map(pos, 0, 180, 1638, 7864);
                ledcWriteChannel(SERVO_CHANNEL, dutyCycle);
                change = (pos<0)? 10 : ((pos>180)? -10: change);
                pos += change;
                vTaskDelay(50 / portTICK_PERIOD_MS);
            } else {
                wave = 0;
            }
            // int dutyCycle = map(pos, 0, 180, 1638, 7864);
            // ledcWriteChannel(SERVO_CHANNEL, dutyCycle);
            // change = (pos<0)? 10 : ((pos>180)? -10: change);
            // pos += change;
            // vTaskDelay(50 / portTICK_PERIOD_MS);
        } else {
            vTaskDelay(1000 / portTICK_PERIOD_MS); 
        }
    }
}

void serial(void * pvParameters){
    while (1) {
        if (Serial.available() > 0) {
            Serial.println(control); 
            String input = Serial.readString();
            input.trim();
            Serial.print("You entered: ");
            Serial.println(input); 
            if (input == "stop") {
                control = 0;
            } else if (input == "straight") {
                control = 1;
            } else if (input == "reverse") {
                control = 2;
            } else if (input == "left") {
                control = 3;
            } else if (input == "right") {
                control = 4;
            } else if (input == "wave") {
                wave = 1;
                start_time = millis();
            } else if (input == "nowave") {
                wave = 0;
            } else if (input == "music") {
                music = 1;
            } else if (input == "obstacle") {
                front_obstacle = 1;
            } else if (input == "no_obstacle") {
                front_obstacle = 0;
            }
            Serial.println("control="+String(control)+" ; wave="+String(wave)+" ; music="+String(music)); 
          } else {
            vTaskDelay(1000 / portTICK_PERIOD_MS); 
        }
    }
}

void setup() {
  Serial.begin(9600);
  // xTaskCreate(serial, "serial", 2048, NULL, 1, NULL);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);
  pinMode(LED5, OUTPUT);
  // pinMode(LED_BUILTIN, OUTPUT)
  xTaskCreate(led, "led", 2048, NULL, 1, NULL);

  ledcAttachChannel(MOTOR1_IN1, MOTOR_FREQ, RESOLUTION, MOTOR1_CHANNEL_A);
  ledcAttachChannel(MOTOR1_IN2, MOTOR_FREQ, RESOLUTION, MOTOR1_CHANNEL_B);
  ledcAttachChannel(MOTOR2_IN3, MOTOR_FREQ, RESOLUTION, MOTOR2_CHANNEL_A);
  ledcAttachChannel(MOTOR2_IN4, MOTOR_FREQ, RESOLUTION, MOTOR2_CHANNEL_B);
  xTaskCreate(motor, "motor", 2048, NULL, 1, NULL);

  ledcAttachChannel(BUZZER, 100, RESOLUTION, BUZZER_CHANNEL);
  // ledcAttach(BUZZER, 100, RESOLUTION);
  xTaskCreate(buzz, "buzz", 2048, NULL, 1, NULL);

  pinMode(AVOIDANCE1, INPUT);
  xTaskCreate(avoidance1, "avoidance1", 4096, NULL, 1, NULL);
  pinMode(AVOIDANCE2, INPUT);
  xTaskCreate(avoidance2, "avoidance2", 4096, NULL, 1, NULL);

  lcd.init();
  lcd.backlight();
  xTaskCreate(display, "display", 2048, NULL, 1, NULL);

  ledcAttachChannel(SERVO, SERVO_FREQ, SERVO_RESOLUTION, SERVO_CHANNEL);
  xTaskCreate(servo, "servo", 2048, NULL, 1, NULL);

  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(500 / portTICK_PERIOD_MS); 
    Serial.print(".");
  }
  Serial.println("");
  Serial.println(F("WiFi is connected!"));
  Serial.println(F("IP address set: "));
  Serial.println(WiFi.localIP());

  espClient.setCACert(AWS_CERT_CA);
  espClient.setCertificate(AWS_CERT_CRT);
  espClient.setPrivateKey(AWS_CERT_PRIVATE);
  client.setServer(AWS_IOT_ENDPOINT, 8883);

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    vTaskDelay(1000 / portTICK_PERIOD_MS); 
  }
  if (!client.connected()) {
    Serial.println("AWS IoT Timeout!");
    return;
  }
  if (client.subscribe(subscribeTopic)) {
    Serial.println("Subscribed successfully!");
  } else {
      Serial.println("Subscription failed!");
  }
  client.setCallback(commandCallback);
  client.subscribe(subscribeTopic);
  Serial.println("AWS IoT Connected!");
}

void loop() {
  client.loop();
}
 
void commandCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
      message += (char)payload[i];
  }
  Serial.println(message);
       
  if (message == "stop") {
      control = 0;
      latest_command = 0;
  } else if (message == "straight") {
      control = 1;
      check = 10;
      latest_command = 1;
  } else if (message == "reverse") {
      control = 2;
      check = 10;
      latest_command = 2;
  } else if (message == "left") {
      control = 3;
      check = 10;
      latest_command = 3;
  } else if (message == "right") {
      control = 4;
      check = 10;
      latest_command = 4;
  } else if (message == "wave") {
      wave = 1;
      start_time = millis();
  } else if (message == "nowave") {
      wave = 0;  
  } else if (message == "music") {
      music = 1;
  }
}
