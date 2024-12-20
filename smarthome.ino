#include "SIMGPRS.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <Adafruit_NeoPixel.h>


#define PHONENUMER "+843xxxxxxxxxxxxxxxx"
#define LED 2
#define NEOPIXEL_PIN 15
#define NUMPIXELS 8
#define LED1_PIN 5
#define LED2_PIN 18
#define LED3_PIN 19
#define BOUY_PIN 27
#define CO2_PIN 35
#define VIBRATE_PIN 14
#define DISTANCE_PIN 33
#define BUMP_PIN 26
#define BUZZER_PIN 25

#define BLYNK_TEMPLATE_ID "TMPL6uH5srfF7"
#define BLYNK_TEMPLATE_NAME "Nhà thông minh"
#define BLYNK_AUTH_TOKEN "kF-xxxxxxxx"
#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp32.h>
char ssid[] = "Linhtran";
char pass[] = "123454321";
struct Color
{
  int r;
  int g;
  int b;
};
Color red = {100, 0, 0};
Color green = {0, 100, 0};
Color white = {100, 100, 100};
bool co2Alarm, distanceAlarm, vibrateAlarm, bouyAlarm;
SIMGPRS sim(&Serial2, &Serial);
Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
void setup()
{
  pinMode(LED, OUTPUT);
  Serial.begin(115200);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);
  pinMode(BOUY_PIN, INPUT_PULLUP);
  pinMode(DISTANCE_PIN, INPUT);
  pinMode(CO2_PIN, INPUT);
  pinMode(VIBRATE_PIN, INPUT);
  pinMode(BUMP_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUMP_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  //sim.findBaud();
  Serial2.begin(115200);
  sim.init();
  pixels.begin();
  pixels.clear();
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(200);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  sim.deleteAllMessages();
}
void loop()
{
  Blynk.run();
  sim.run();
  readSesorAndRun();
  alarmTrigger();
}
void readSesorAndRun()
{
  uint32_t now = millis();
  static uint32_t lastReadCo2, lastReadDistance, lastWiteVibrateVal, lastReadBouy;
  static uint32_t startTimeVibrateAlarm;
  
  if (lastReadCo2 + 200 < now)
  {
    lastReadCo2 = now;
    int co2 = analogRead(CO2_PIN);
    co2 = map(co2, 0, 4096, 0, 100);
    co2Alarm = co2 > 25 ? true : false;
    if (co2Alarm)
    {
      pixels.setPixelColor(0, pixels.Color(red.r, red.g, red.b));
    }
    else
    {
      pixels.setPixelColor(0, pixels.Color(white.r, white.g, white.b));
    }
    
    //Blynk.virtualWrite(V7, co2);
    // Serial.println(co2);
  }
  if (lastReadDistance + 200 < now)
  {
    lastReadDistance = now;
    distanceAlarm = !digitalRead(DISTANCE_PIN);
    //Blynk.virtualWrite(V6, distanceAlarm);
    if (distanceAlarm)
    {
      pixels.setPixelColor(1, pixels.Color(red.r, red.g, red.b));
    }
    else
    {
      pixels.setPixelColor(1, pixels.Color(white.r, white.g, white.b));
    }
  }
  bool vibration = detectContinuousVibration();
  if (vibration)
  {
    vibrateAlarm = true;
    startTimeVibrateAlarm = millis();
  }
  if (startTimeVibrateAlarm + 3000 < now && !vibration)
  {
    vibrateAlarm = false;
  }
  if (lastWiteVibrateVal + 200 < now || vibrateAlarm == true)
  {
    lastWiteVibrateVal = now;
    if (vibrateAlarm)
    {
      pixels.setPixelColor(2, pixels.Color(red.r, red.g, red.b));
    }
    else
    {
      pixels.setPixelColor(2, pixels.Color(white.r, white.g, white.b));
    }
    //Blynk.virtualWrite(V4, vibrateAlarm);
  }

  if (lastReadBouy + 200 < now)
  {
    bouyAlarm = digitalRead(BOUY_PIN);
    //Blynk.virtualWrite(V5, bouyAlarm);
    if (bouyAlarm)
    {
      pixels.setPixelColor(3, pixels.Color(red.r, red.g, red.b));
    }
    else
    {
      pixels.setPixelColor(3, pixels.Color(white.r, white.g, white.b));
    }
  }
  pixels.show();

}

bool detectContinuousVibration()
{
  const int NUM_VIBRATIONS = 3; // Số lần rung yêu cầu
  const int DETECT_TIME = 200;  // Thời gian phát hiện rung
  const int RESET_TIME = 500;   // Thời gian reset nếu chưa đủ số lần rung

  static int vibrationCount = 0;
  static unsigned long previousMillis = 0;
  static bool isVibrating = false;

  int sensorValue = !digitalRead(VIBRATE_PIN);
  unsigned long currentMillis = millis();

  if (sensorValue )
  {
    vibrationCount++;
    isVibrating = true;
    previousMillis = currentMillis;
  }
  else
  {
    if (isVibrating && (currentMillis - previousMillis >= DETECT_TIME))
    {
      if (vibrationCount >= NUM_VIBRATIONS)
      {
        vibrationCount = 0; // Reset số lần đếm
        isVibrating = false;
        return true; // Đã rung đủ 5 lần
      }
      else
      {
        vibrationCount = 0; // Reset số lần đếm
        isVibrating = false;
        previousMillis = currentMillis; // Reset thời gian bắt đầu
      }
    }
  }

  if (isVibrating && (currentMillis - previousMillis >= DETECT_TIME + RESET_TIME))
  {
    vibrationCount = 0;
    isVibrating = false;
  }

  return false;
}
void alarmTrigger()
{
  if (co2Alarm)
  {
    sendMessage("canh bao chay");
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(BUMP_PIN, HIGH);
  }

  if (distanceAlarm)
  {
    sendMessage("canh bao trom");
    digitalWrite(BUZZER_PIN, HIGH);
  }
  
  if (vibrateAlarm)
  {
    sendMessage("canh bao dong dat");
    digitalWrite(BUZZER_PIN, HIGH);
  }

  if (bouyAlarm)
  {
    sendMessage("canh bao ngap lut");
    digitalWrite(BUZZER_PIN, HIGH);
  }
  
  if (!co2Alarm && !distanceAlarm && !vibrateAlarm && !bouyAlarm)
  {
    digitalWrite(BUMP_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }
  
}
void sendMessage(String message)
{
  static String lastMess;
  String phoneNumber = String(PHONENUMER);
  static uint32_t lastSendSMS;
  if (lastSendSMS + 2000 <= millis() && lastMess != message)
  {
    lastSendSMS = millis();
    lastMess = message;
    sim.sendSMS(&phoneNumber, &lastMess);
  }
}


BLYNK_WRITE(V0)
{                               // V0 là Virtual Pin kết nối với button trong Blynk app
  int pinValue = param.asInt(); // Đọc giá trị từ button (0 = tắt, 1 = bật)

  if (pinValue == 1)
  {
    digitalWrite(LED1_PIN, HIGH); // Bật đèn
  }
  else
  {
    digitalWrite(LED1_PIN, LOW); // Tắt đèn
  }
}
BLYNK_WRITE(V1)
{                               // V1 là Virtual Pin kết nối với button2 trong Blynk app
  int pinValue = param.asInt(); // Đọc giá trị từ button (0 = tắt, 1 = bật)

  if (pinValue == 1)
  {
    digitalWrite(LED2_PIN, HIGH); // Bật đèn
  }
  else
  {
    digitalWrite(LED2_PIN, LOW); // Tắt đèn
  }
}
BLYNK_WRITE(V2)
{                               // V1 là Virtual Pin kết nối với button2 trong Blynk app
  int pinValue = param.asInt(); // Đọc giá trị từ button (0 = tắt, 1 = bật)

  if (pinValue == 1)
  {
    digitalWrite(LED3_PIN, HIGH); // Bật đèn
  }
  else
  {
    digitalWrite(LED3_PIN, LOW); // Tắt đèn
  }
}
