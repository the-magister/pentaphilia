#include <Streaming.h>

void setup() {
  Serial.begin(115200);
  // put your setup code here, to run once:
  pinMode(2, OUTPUT);
pinMode(14, OUTPUT);
pinMode(7, OUTPUT);
pinMode(8, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(2, LOW);
  digitalWrite(14, LOW);
  digitalWrite(7, LOW);
  digitalWrite(8, LOW);
  Serial << "Low" << endl;
  delay(2000);
 
  // put your main code here, to run repeatedly:
  digitalWrite(2, HIGH);
  digitalWrite(14, HIGH);
  digitalWrite(7, HIGH);
  digitalWrite(8, HIGH);
  Serial << "HIGH" << endl;
  delay(2000);
  
}
