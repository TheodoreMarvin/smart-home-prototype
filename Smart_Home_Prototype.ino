#define BLYNK_TEMPLATE_ID "TMPL6EHt5vKlJ"
#define BLYNK_TEMPLATE_NAME "Penyaing Bardi"
#define BLYNK_AUTH_TOKEN ""   // enter correct token

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <AccelStepper.h>
#include <time.h>

char ssid[] = "My Hotspot";
char pass[] = "";

int counter = 0;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7 * 3600;
const int   daylightOffset_sec = 0;

#define ruang_tamu 16
#define ruang_keluarga 17
#define kamar 18

// stepper
// 2048 steps = 1 revolution
#define MotorInterfaceType AccelStepper::FULL4WIRE
#define IN1 14
#define IN2 27
#define IN3 26
#define IN4 25

#define STEPS_PER_REV 2048
int rainThreshold = 3500;
bool isRetracted = false;
bool isMoving = false;

unsigned long lastSensorCheck = 0;
const unsigned long sensorInterval = 1000; // check every 1 second

AccelStepper stepper(MotorInterfaceType, IN1, IN3, IN2, IN4);


// rain sensor
// #define DO_PIN 21
#define AO_PIN 33
#define PWR_PIN 19

#define LDR_PIN 35

// kamar_lt1
BLYNK_WRITE(V1) {
  int value = param.asInt();
  digitalWrite(ruang_tamu, !value);
  // pin P16
}

// // kamar_lt2
BLYNK_WRITE(V2) {
  int value = param.asInt();
  digitalWrite(ruang_keluarga, !value);
  // pin P17
}

// // family_room
BLYNK_WRITE(V3) {
  int value = param.asInt();
  digitalWrite(kamar, !value);
  // pin P18
}

unsigned long prevMillis = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(ruang_tamu, OUTPUT);
  pinMode(ruang_keluarga, OUTPUT);
  pinMode(kamar, OUTPUT);
  digitalWrite(ruang_tamu, HIGH);
  digitalWrite(ruang_keluarga, HIGH);
  digitalWrite(kamar, HIGH);
  pinMode(AO_PIN, INPUT);
  analogSetAttenuation(ADC_11db);

  stepper.setMaxSpeed(800);
  stepper.setAcceleration(300);

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect();
  // Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
}

void loop() {
  stepper.run();

  if (!isMoving) {
    struct tm timeinfo;
    bool isNight = false;
    if (getLocalTime(&timeinfo)) {
      int hour = timeinfo.tm_hour;
      if (hour >= 7 && hour < 18) {
        isNight = false;
        Serial.println("It's daytime");
      } else {
        isNight = true;
        Serial.println("It's nighttime");
      }
    } else {
      Serial.println("Failed to obtain time");
    }

    int sensorValue = analogRead(AO_PIN);
    Serial.print("Rain Sensor Value: ");
    Serial.println(sensorValue);

    if (sensorValue < rainThreshold && !isRetracted) {
      // basah
      Serial.println("Rain detected: Retracting...");
      Blynk.virtualWrite(V4, "Hujan");
      Blynk.logEvent("cuaca_rumah", "Di rumah sedang hujan. Jemuran masuk ke rumah");
      stepper.move(STEPS_PER_REV * 6);  // 1 rev CW
      isRetracted = true;
      isMoving = true;
    } 

    else if (isNight) {
      // night no rain
      if (!isRetracted) {
        Serial.println("Rain detected: Retracting...");
        Blynk.virtualWrite(V4, "Malam");
        Blynk.logEvent("waktu_operasional", "Waktu sudah malam. Jemuran masuk ke rumah");
        stepper.move(STEPS_PER_REV * 6);  // 1 rev CW
        isRetracted = true;
        isMoving = true;
      }
    }

    // else if (sensorValue >= rainThreshold && isRetracted) {
    else {
      // cek terang
      int ldrValue = analogRead(LDR_PIN);
      Serial.print("LDR: ");
      Serial.println(ldrValue);

      if (ldrValue > 2500) {
        // terang
        Blynk.virtualWrite(V4, "Cerah");
        if (isRetracted) {
          Serial.println("Cerah detected: Extending...");
          Blynk.logEvent("cuaca_rumah", "Di rumah sedang cerah. Jemuran dikeluarkan");
          stepper.move(-STEPS_PER_REV * 6);  // Extend
          isRetracted = false;
          isMoving = true;
        }
      }
      else {
        // mendung not night no rain
        if (!isRetracted) {
          Serial.println("Rain detected: Retracting...");
          Blynk.virtualWrite(V4, "Mendung");
          Blynk.logEvent("cuaca_rumah", "Di rumah sedang mendung. Jemuran masuk ke rumah");
          stepper.move(STEPS_PER_REV * 6);  // 1 rev CW
          isRetracted = true;
          isMoving = true;
        }
      }

      // Serial.println("Dry detected: Extending...");
      // Blynk.virtualWrite(V4, "Cerah");
      // Blynk.logEvent("cuaca_rumah", "Rumah cerah kembali");
      // stepper.move(-STEPS_PER_REV * 6);  // 1 rev CCW
      // isRetracted = false;
      // isMoving = true;
    }
  }

  // Only check rain sensor every 1 second (non-blocking)
  // if (millis() - lastSensorCheck > sensorInterval && !isMoving) {
  //   lastSensorCheck = millis();

  //   int sensorValue = analogRead(AO_PIN);
  //   Serial.print("Rain Sensor Value: ");
  //   Serial.println(sensorValue);

  //   if (sensorValue < rainThreshold && !isRetracted) {
  //     Serial.println("Rain detected: Retracting...");
  //     Blynk.virtualWrite(V4, "Hujan");
  //     Blynk.logEvent("cuaca_rumah", "Rumah sedang hujan");
  //     stepper.move(STEPS_PER_REV * 6);  // 1 rev CW
  //     isRetracted = true;
  //     isMoving = true;
  //   } 
  //   else if (sensorValue >= rainThreshold && isRetracted) {
  //     Serial.println("Dry detected: Extending...");
  //     Blynk.virtualWrite(V4, "Cerah");
  //     Blynk.logEvent("cuaca_rumah", "Rumah cerah kembali");
  //     stepper.move(-STEPS_PER_REV * 6);  // 1 rev CCW
  //     isRetracted = false;
  //     isMoving = true;
  //   }
  // }

  // Check if move is complete
  if (isMoving && stepper.distanceToGo() == 0) {
    isMoving = false;
    Serial.println("Move complete.");
    if (isRetracted) {
      Blynk.virtualWrite(V5, "Di Dalam");
    }
    else {
      Blynk.virtualWrite(V5, "Di Luar");
    }
  }

  Blynk.run();
}

void printLocalTime() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    Serial.println(&timeinfo, "Current WIB Time: %Y-%m-%d %H:%M:%S");
  } else {
    Serial.println("Failed to obtain time");
  }
}