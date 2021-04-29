#include <Arduino.h>
const int TST_PIN = D5;

enum SuperState{
    NONE,
    SENDER,
    RECEIVER
};

void setup() 
{
  Serial.begin(115200);
  delay(5000);
  pinMode(TST_PIN, OUTPUT);
  unsigned long timeBegin = micros();

  // unsigned int arr[500];
  // unsigned int IRcode = 0b11000001110001111100000000111111;
  // int maxBitNumber = 34;
  // for (int i = 0; i < 500; ++i)
  // {
  //   int errorCounter = 0;
  // }
  
  unsigned long timeEnd = micros();
  unsigned long totalDuration = timeEnd - timeBegin;
  double duration = (double)totalDuration / 500.0;
  Serial.print("Test Enum equals int: ");
  Serial.print(1 == SENDER);
  Serial.println();
  Serial.print("Total duration: ");
  Serial.print(totalDuration);
  Serial.println(" µs");
  Serial.print("Duration for this command: ");
  Serial.print(duration);
  Serial.println(" µs");
}
void loop() {}

/**
 * ESP8266
 * digitalWrite(D5, HIGH) Duration: 0.96 µs
 * digitalWrite(D5, LOW) Duration: 1.01 µs
 */