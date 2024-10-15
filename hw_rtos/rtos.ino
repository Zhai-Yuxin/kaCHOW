#include "Cdrv8833.h"
#include "pitches.h"

#define LED1 5
#define LED2 5
#define LED3 5
#define LED4 5
#define LED5 5
#define BUZZER 5
#define AVOIDANCE1 5
#define AVOIDANCE2 5
#define MOTOR1_IN1 21
#define MOTOR1_IN2 22
#define MOTOR2_IN3 23
#define MOTOR2_IN4 24

#define LCD 5
#define SERVO 5  // waving small flag constantly when on?

// flags
volatile int control = 0;
volatile bool front_obstacle = 0;
volatile bool back_obstacle = 0;

#define CHANNEL 0 // there are 16 unique PWM channels (0..15)
#define SWAP false // swap motor rotation direction
Cdrv8833 myMotor1;
Cdrv8833 myMotor2;

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
int noteDurations[] = {
  8, 8, 4,
  8, 8, 4,
  8, 8, 8, 8,
  2,
  8, 8, 8, 8,
  8, 8, 8, 16, 16,
  8, 8, 8, 8,
  4, 4
};

void left() {
    digitalWrite(LED1, HIGH); 
    digitalWrite(LED2, HIGH); 
    digitalWrite(LED3, LOW); 
    digitalWrite(LED4, LOW); 
    digitalWrite(LED5, LOW); 
    vTaskDelay(500 / portTICK_PERIOD_MS); 
    digitalWrite(LED1, LOW); 
    digitalWrite(LED2, LOW); 
    vTaskDelay(500 / portTICK_PERIOD_MS); 
} // control = 3

void right() {
    digitalWrite(LED1, LOW); 
    digitalWrite(LED2, LOW); 
    digitalWrite(LED3, LOW); 
    digitalWrite(LED4, HIGH); 
    digitalWrite(LED5, HIGH); 
    vTaskDelay(500 / portTICK_PERIOD_MS); 
    digitalWrite(LED4, LOW); 
    digitalWrite(LED5, LOW); 
    vTaskDelay(500 / portTICK_PERIOD_MS); 
} // control = 4

void straight() {
    digitalWrite(LED1, HIGH); 
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
    digitalWrite(LED4, HIGH);
    digitalWrite(LED5, HIGH);
} // control = 1

void reverse() {
    digitalWrite(LED1, LOW); 
    digitalWrite(LED2, LOW); 
    digitalWrite(LED3, LOW); 
    digitalWrite(LED4, LOW); 
    digitalWrite(LED5, LOW); 
} // control = 2

void stop() {
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
    digitalWrite(LED4, HIGH);
    digitalWrite(LED5, HIGH);
    vTaskDelay(1000 / portTICK_PERIOD_MS); // block for 1000ms
    digitalWrite(LED1, LOW); 
    digitalWrite(LED2, LOW); 
    digitalWrite(LED3, LOW); 
    digitalWrite(LED4, LOW); 
    digitalWrite(LED5, LOW); 
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}  // control = 0

void led(void * pvParameters) {
    if (control == 0) {
        stop();
    } else if (control == 1) {
        straight();
    } else if (control == 2) {
        reverse();
    } else if (control == 3) {
        left();
    } else if (control == 4) {
        right();
    }
}

void motor(void * pvParameters) {
    if ((control == 0) || (front_obstacle == 1) || (back_obstacle == 1)) {
        myMotor1.stop();
        myMotor2.stop();
    } else if (control == 1) {
        myMotor1.move(50); //rotation power -100..100
        myMotor2.move(50);
    } else if (control == 2) {
        myMotor1.move(-50);
        myMotor2.move(-50);
    } else if (control == 3) {
        myMotor1.move(20);
        myMotor2.move(40);
    } else if (control == 4) {
        myMotor1.move(40);
        myMotor2.move(20);
    }
}

void buzz() {
    if ((front_obstacle == 1) || (back_obstacle == 1)) {
        int size = sizeof(noteDurations) / sizeof(int);
        for (int thisNote = 0; thisNote < size; thisNote++) {
            int noteDuration = 1000 / noteDurations[thisNote];
            tone(BUZZER, melody[thisNote], noteDuration);
            int pauseBetweenNotes = noteDuration * 1.30;
            vTaskDelay(pauseBetweenNotes / portTICK_PERIOD_MS);
            noTone(BUZZER);
        }
    }
}

void setup() {
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);
  pinMode(LED5, OUTPUT);
  // pinMode(LED_BUILTIN, OUTPUT)
  xTaskCreate(led, "led", 2048, NULL, 1, NULL);

  myMotor1.init(MOTOR1_IN1, MOTOR1_IN2, CHANNEL, SWAP);
  myMotor2.init(MOTOR2_IN3, MOTOR2_IN4, CHANNEL, SWAP);
//   xTaskCreate(motor, "motor", 2048, NULL, 1, NULL);  -- be handled as interrupt

  xTaskCreate(buzz, "buzz", 2048, NULL, 1, NULL);
}

void loop() {
}