#include <Encoder.h>

Encoder enc(2, 3);
long positionLeft  = -999;

void setup() {
  Serial.begin(9600);
  Serial.println("TwoKnobs Encoder Test:");
 }

void loop() {
  long newLeft;
  newLeft = enc.read();
  
  if (newLeft != positionLeft) {
    Serial.print("Left = ");
    Serial.print(newLeft);
    Serial.println();
    positionLeft = newLeft;
  }
  // if a character is sent from the serial monitor,
  // reset both back to zero.
  if (Serial.available()) {
    Serial.read();
    Serial.println("Reset both knobs to zero");
    enc.write(0);
   }
}
