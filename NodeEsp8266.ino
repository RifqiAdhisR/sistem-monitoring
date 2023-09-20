#include <SoftwareSerial.h>
#include "TRIGGER_WIFI.h"
#include "TRIGGER_GOOGLESHEETS.h"
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h> 
LiquidCrystal_I2C lcd(0x27, 20, 4);

byte relay_pin_b D5
byte relay_pin_c D6
float kategoriWaterLevel = 0.0;
float kategoriSoilpH = 0.0;
float responseWaterLevel = 0.0;
float responseSoilpH = 0.0;


SoftwareSerial arduinoSerial(D2, D3); // RX, TX

char column_name_in_sheets[][20] = {"value1", "value2","value3","value4","value5","value6"};
String Sheets_GAS_ID = "AKfycbxUG4f8GCwuA-6eL6pPQ2d_wCu7f4kiXpsdlCR1Yu9zxjoSA9moAW5z1uaRR0x8npv6";
int No_of_Parameters = 6; // Ubah jumlah parameter menjadi 2

void setup() {
  Serial.begin(9600);
  arduinoSerial.begin(9600);
  pinMode(relay_pin_b,OUTPUT);
  pinMode(relay_pin_c,OUTPUT);
  WIFI_Connect("Nggih", "doboler1234");
  Google_Sheets_Init(column_name_in_sheets, Sheets_GAS_ID, No_of_Parameters);
}

void parseResponse(String response, const char *parameterName)
{
  // Buat buffer JSON dengan ukuran yang cukup
  StaticJsonDocument<200> jsonDoc;

  // Parse JSON
  DeserializationError error = deserializeJson(jsonDoc, response);

  // Cek jika terdapat error saat parsing
  if (error)
  {
    Serial.print("Gagal parsing JSON. Error: ");
    Serial.println(error.c_str());
    return;
  }

  // Cek jika kunci "response" ada dalam JSON
  if (jsonDoc.containsKey("response"))
  {
    // Ambil nilai dari kunci "response"
    int responseValue = jsonDoc["response"];

    if (jsonDoc.containsKey("kategori"))
    {
      const char *kategori = jsonDoc["kategori"];

      int kategoriAngka = 0;

      if (strcmp(parameterName, "waterlevel") == 0)
      {
        if (strcmp(kategori, "Rendah") == 0)
        {
          kategoriAngka = 1;
          kategoriWaterLevel = 1.0;
        }
        else if (strcmp(kategori, "Sedang") == 0)
        {
          kategoriAngka = 2;
          kategoriWaterLevel = 2.0;
        }
        else if (strcmp(kategori, "Tinggi") == 0)
        {
          kategoriAngka = 3;
          kategoriWaterLevel = 3.0;
        }
      }
      else if (strcmp(parameterName, "tanah") == 0)
      {
        if (strcmp(kategori, "Asam") == 0)
        {
          kategoriAngka = 1;
          kategoriSoilpH = 1.0;
        }
        else if (strcmp(kategori, "Optimal") == 0)
        {
          kategoriAngka = 2;
          kategoriSoilpH = 2.0;
        }
        else if (strcmp(kategori, "Basa") == 0)
        {
          kategoriAngka = 3;
          kategoriSoilpH = 3.0;
        }
      }

      // Tampilkan nilai kategori sebagai angka
      Serial.print("Kategori (");
      Serial.print(parameterName);
      Serial.print("): ");
      Serial.println(kategoriAngka);
    }

    // Simpan nilai response
    if (strcmp(parameterName, "ketinggian") == 0)
    {
      responseWaterLevel = static_cast<float>(responseValue);
    }
    else if (strcmp(parameterName, "tanah") == 0)
    {
      responseSoilpH = static_cast<float>(responseValue);
    }

    // Tampilkan nilai "response"
    Serial.print("Nilai response: ");
    Serial.println(responseValue);

    // Pengecekan aksi_waterlevel, aksi_phtanah
    if (jsonDoc.containsKey("aksi_waterlevel"))
    {
      double aksiWaterLevel = jsonDoc["aksi_waterlevel"].as<double>();
      Serial.print("Nilai bulat aksi WaterLevel: ");
      Serial.println(aksiWaterLevel, 3);
    }
    else if (jsonDoc.containsKey("aksi_soilph"))
    {
      double aksiSoilpH = jsonDoc["aksi_soilph"].as<double>();
      Serial.print("Nilai bulat aksi SoilpH: ");
      Serial.println(aksiSoilpH, 3);
    }
    Serial.println();
  }
  else
  {
    Serial.println("Kunci 'response' tidak ditemukan dalam JSON");
  }
}

void sendDataToAPI(float value, const char *parameterName, const char *url)
{
  HTTPClient http;

  // Buat objek JSON
  StaticJsonDocument<200> jsonDoc;
  jsonDoc[parameterName] = value;

  // Buat string untuk menyimpan data JSON
  String jsonString;
  serializeJson(jsonDoc, jsonString);

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  // Kirim data ke API menggunakan metode POST
  int httpResponseCode = http.POST(jsonString);

  // Jika berhasil mengirim data (httpResponseCode = 200)
  if (httpResponseCode == 200)
  {
    Serial.println("Data terkirim ke API");
    // Baca respons dari API
    String response = http.getString();
    Serial.print("Respons dari API: ");
    Serial.print(response);

    // Parse respons JSON
    parseResponse(response, parameterName);
  }
  else
  {
    Serial.print("Gagal mengirim data. Error code: ");
    Serial.println(httpResponseCode);
    Serial.println();
  }

  http.end();
}


void loop() {
  if (arduinoSerial.available()) {
    String dataString = arduinoSerial.readStringUntil('\n');
    dataString.trim();
    Serial.println("Data dari Arduino: " + dataString); // Menampilkan data dari Arduino di Serial Monitor
    // Pisahkan data menjadi 2 nilai
    int delimiterIndex1 = dataString.indexOf(',');
    int delimiterIndex2 = dataString.indexOf(',', delimiterIndex1 + 1);

    String waterLevelStr = dataString.substring(0, delimiterIndex1);
    String soilPhStr = dataString.substring(delimiterIndex1 + 1, delimiterIndex2);

    // Konversi nilai ke tipe float
    float waterLevel = waterLevelStr.toFloat();
    float soilPh = soilPhStr.toFloat();

    // Kirim data ke server
    sendDataToAPI(waterLevel, "ketinggian", "http://verticalfarming.zaws.net/water_level");
    sendDataToAPI(soilPh, "tanah", "http://verticalfarming.zaws.net/phtanah");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WLevel: ");
    lcd.setCursor(8, 0);
    lcd.print(waterLevel);

    lcd.setCursor(0, 1);
    lcd.print("PHTanah: ");
    lcd.setCursor(10, 1);
    lcd.print(soilPh);

    if (responseWaterLevel == 1) {
      lcd.setCursor(0, 3);
      lcd.print("SW=ON");
      digitalWrite(relay_pin_b, LOW);
      Serial.println("solenoid WaterLevel ON");
    } else {
      digitalWrite(relay_pin_b, HIGH);
      lcd.setCursor(0, 3);
      lcd.print("SW=OFF");
      Serial.println("solenoid WaterLevel OFF");
    }

    if (responseSoilpH == 1) {
      lcd.setCursor(8, 3);
      lcd.print("SS=ON");
      digitalWrite(relay_pin_c, LOW);
      Serial.println("solenoid pH Tanah ON");
    } else {
      lcd.setCursor(8, 3);
      lcd.print("SS=OFF");
      digitalWrite(relay_pin_c, HIGH);
      Serial.println("solenoid pH Tanah OFF");
    }

    // Kirim data ke Google Sheets
    Data_to_Sheets(No_of_Parameters, waterLevel, soilPh, kategoriWaterLevel, kategoriSoilpH, responseWaterLevel, responseSoilpH);
    Serial.println();
  }
  delay(15000); // Delay 1 detik sebelum membaca data baru dari Arduino
}
