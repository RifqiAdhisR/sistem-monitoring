#include <SoftwareSerial.h>

// Inisialisasi objek SoftwareSerial
SoftwareSerial espSerial(2, 3); // RX, TX

void setup() {
  // Inisialisasi komunikasi serial dengan baud rate 9600
  Serial.begin(9600);
  espSerial.begin(9600);
}

void loop() {
  // Baca nilai sensor water level
  int waterLevel = analogRead(A0);
  // Baca nilai sensor pH tanah menggunakan relay
  int sensorValue = analogRead(A1);
  float soilPh = (1023 - sensorValue) * 14.0 / 1023.0;
  // Baca nilai sensor pH air
  int watermeter = analogRead(A2);
  double teganganPh = 5 / 1024.0 * watermeter;
  float PH0 = 3.1;
  float PH_step = PH0 / 3;
  float waterPh = 7.00 + ((PH0 - teganganPh) / PH_step);

  // Kirim data melalui komunikasi serial ke ESP8266
  espSerial.print(waterLevel);
  espSerial.print(",");
  espSerial.print(soilPh);
  espSerial.print(",");
  espSerial.println(waterPh);

  delay(1000); // Tunda 1 detik
}
