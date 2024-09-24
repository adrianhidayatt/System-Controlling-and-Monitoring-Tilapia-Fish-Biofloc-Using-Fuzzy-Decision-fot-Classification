#include <ESP32Firebase.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>    //https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library
#include <OneWire.h>
#include <DallasTemperature.h>
#include <HTTPClient.h>
#include <RTClib.h>
#include <WiFi.h>
#include <EEPROM.h>
#include <GravityTDS.h>

RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Minggu", "Senin", "Selasa", "Rabu", "Kamis", "Jum'at", "Sabtu"};
int jam, menit, detik;
int tanggal, bulan, tahun;
String hari;

#define WIFI_SSID "HelmiFarmMandiri"          // Your WiFi SSID
#define WIFI_PASSWORD "313354Okehh"      // Your WiFi Password
#define REFERENCE_URL "https://hadimmarwan-7bc1f-default-rtdb.asia-southeast1.firebasedatabase.app/"  // Your Firebase project reference url
#define R1 13
#define R2 12
#define R3 14
#define R4 27
#define TdsSensorPin A0
#define ONE_WIRE_BUS 25
Firebase firebase(REFERENCE_URL);
LiquidCrystal_I2C lcd(0x27, 20, 4);


/*
void TDS ();
int pin_TDS = A0;
float voltage;
float ntu;
float ntukoreksi;
*/
void TDS ();
GravityTDS gravityTds;
float Temperature , tdsValue;

void PH();
const int pin_Ph = 34;
float Po = 0;
float PH_step;
int nilai_analog_Ph;
double TeganganPh;
//untuk kalibrasi (diperoleh setelah melakukan kalibrasi dan  nilai bisa berubah-ubah)
float PH4 =3.300;
float PH7= 2.513;

void suhu ();
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
float Temperaturekoreksi;

// Google script Web_App_URL.
String Web_App_URL = "https://script.google.com/macros/s/AKfycbzsSWVr0GbCy4eHxbWxE4PjqCmQX6Bej-PeBRG1L7Fw29O7QbrPMWiSPXnoSBBr2jmi4A/exec"; //untuk database monitoring ikan lele

String Status_Read_Sensor = "";
String Status_Relay_1 = "";
String Status_Relay_2 = "";
String Status_Relay_3 = "";
String Status_Relay_4 = "";
String kualitas_air_gsheet = "";
String kualitas_air ;

// Himpunan Fuzzy pH (Fungsi Keanggotaan Fuzzy)
float ph_asam, ph_netral, ph_basa;

float func_ph_asam(float Po) {
    if (Po < 6.0) {
        ph_asam = 1;
    } else if (Po >= 6.0 && Po < 6.5) {
        ph_asam = (6.5 - Po) / (6.5 - 6.0);
    } else if (Po >= 6.5) {
        ph_asam = 0;
    }  else {
        ph_asam = 0;
    }
    return ph_asam;
}

float func_ph_netral(float Po) {
    if (Po < 6.5 || Po > 8.5) {
        ph_netral = 0;
    } else if (Po >= 6.0 && Po < 7.0) {
        ph_netral = (Po - 6.0) / (7.0 - 6.0);
    } else if (Po >= 7.0 && Po <= 8.0) {
        ph_netral = 1;
    } else if (Po > 8.0 && Po <= 8.5) {
        ph_netral = (Po - 8.0) / (8.5 - 8.0);
    } else {
        ph_netral = 0;
    }
    return ph_netral;
}

float func_ph_basa(float Po) {
    if (Po <= 8.0) {
        ph_basa = 0;
    } else if (Po > 8.0 && Po < 14.0) {
        ph_basa = (Po - 8.0) / (14.0 - 8.0);
    } else if (Po >= 14.0) {
        ph_basa = 1;
    }  else {
        ph_basa = 1;
    }
    return ph_basa;
}

// Himpunan Fuzzy kekeruhan (Fungsi Keanggotaan Fuzzy)
float kekeruhan_rendah, kekeruhan_sedang, kekeruhan_tinggi;

float func_kekeruhan_rendah(float tdsValue) {
    if (tdsValue <= 190) {
        kekeruhan_rendah = 1;
    } else if (tdsValue > 190 && tdsValue <= 200) {
        kekeruhan_rendah = (200 - tdsValue) / (200 - 190);
    } else if (tdsValue > 200) {
        kekeruhan_rendah = 0;
    } else {
        kekeruhan_rendah = 0;
    }
    return kekeruhan_rendah;
}

float func_kekeruhan_sedang(float tdsValue) {
    if (tdsValue < 200 || tdsValue > 300) {
        kekeruhan_sedang = 0;
    } else if (tdsValue >= 200 && tdsValue < 250) {
        kekeruhan_sedang = (250 - tdsValue) / (250 - 200);
    } else if (tdsValue >= 250 && tdsValue <= 300) {
        kekeruhan_sedang = 1;
    }  else {
        kekeruhan_sedang = 0;
    }
    return kekeruhan_sedang;
}

float func_kekeruhan_tinggi(float tdsValue) {
    if (tdsValue <= 250) {
        kekeruhan_tinggi = 0;
    } else if (tdsValue > 250 && tdsValue <= 300) {
        kekeruhan_tinggi = (300 - tdsValue) / (300 - 250);
    } else if (tdsValue > 300) {
        kekeruhan_tinggi = 1;
    } else {
        kekeruhan_tinggi = 1;
    }
    return kekeruhan_tinggi;
}


// Himpunan Fuzzy Suhu (Fungsi Keanggotaan Fuzzy)
float suhu_dingin, suhu_sedang, suhu_panas;

float func_suhu_dingin(float Temperaturekoreksi) {
    if (Temperaturekoreksi < 20) {
        suhu_dingin = 1;
    } else if (Temperaturekoreksi >= 20 && Temperaturekoreksi < 25) {
        suhu_dingin = (Temperaturekoreksi - 20) / (25 - 20);
    } else if (Temperaturekoreksi >= 25) {
        suhu_dingin = 0;
    } else {
        suhu_dingin = 0;
    }
    return suhu_dingin;
}

float func_suhu_sedang(float Temperaturekoreksi) {
    if (Temperaturekoreksi < 25 || Temperaturekoreksi > 30) {
        suhu_sedang = 0;
    } else if (Temperaturekoreksi >= 25 && Temperaturekoreksi < 28) {
        suhu_sedang = (28 - Temperaturekoreksi) / (28 - 25);
    } else if (Temperaturekoreksi >= 28 && Temperaturekoreksi <= 30) {
        suhu_sedang = 1;
    } else {
        suhu_sedang = 0;
    }
    return suhu_sedang;
}

float func_suhu_panas(float Temperaturekoreksi) {
    if (Temperaturekoreksi < 30) {
        suhu_panas = 0;
    } else if (Temperaturekoreksi >= 30 && Temperaturekoreksi < 35) {
        suhu_panas = (Temperaturekoreksi - 30) / (35 - 30);
    }  else if (Temperaturekoreksi >= 35) {
        suhu_panas = 1;
    } else {
        suhu_panas = 1;
    }
    return suhu_panas;
} 

 
// Fungsi untuk mengambil nilai minimum dari dua bilangan 
float min_f(float a, float b) { 
    return (a < b) ? a : b; 
} 
 
// Fungsi untuk mengambil nilai maksimum dari dua bilangan 
float max_f(float a, float b) { 
    return (a > b) ? a : b; 
} 
// Fungsi untuk menghitung tingkat keanggotaan Kualitas Air Buruk, menghitung nilai minimum dari tiga variabel input (pH, kekeruhan, dan suhu) dengan menggunakan fungsi min_f.
// Fungsi min_f digunakan untuk melakukan operasi AND (intersection) pada variabel input
// Misal rule 1,  "If (pH is asam) and (Suhu is Dingin) and (Kekeruhan is rendah) then (Kualitas_air is Buruk) (1)"
// Pada rule Fungsi min_f digunakan dua kali untuk mendapatkan nilai minimum dari ketiga variabel input.
float kualitas_air_buruk(float keanggotaan_ph, float keanggotaan_kekeruhan, float keanggotaan_suhu) {
    float rule1 = min_f(min_f(keanggotaan_ph, keanggotaan_kekeruhan), keanggotaan_suhu); // Rule 1
    float rule2 = min_f(keanggotaan_ph, min_f(keanggotaan_kekeruhan, keanggotaan_suhu)); // Rule 2
    float rule3 = min_f(keanggotaan_ph, min_f(keanggotaan_kekeruhan, keanggotaan_suhu)); // Rule 3
    float rule4 = min_f(keanggotaan_ph, min_f(keanggotaan_kekeruhan, keanggotaan_suhu)); // Rule 4
    float rule5 = min_f(keanggotaan_ph, min_f(keanggotaan_kekeruhan, keanggotaan_suhu)); // Rule 5
    float rule6 = min_f(keanggotaan_ph, min_f(keanggotaan_kekeruhan, keanggotaan_suhu)); // Rule 6
    float rule7 = min_f(keanggotaan_ph, min_f(keanggotaan_kekeruhan, keanggotaan_suhu)); // Rule 7
    float rule8 = min_f(keanggotaan_ph, min_f(keanggotaan_kekeruhan, keanggotaan_suhu)); // Rule 8
    float rule9 = min_f(keanggotaan_ph, min_f(keanggotaan_kekeruhan, keanggotaan_suhu)); // Rule 9
    float rule10 = min_f(keanggotaan_ph, min_f(keanggotaan_kekeruhan, keanggotaan_suhu)); // Rule 10
    float rule11 = min_f(keanggotaan_ph, min_f(keanggotaan_kekeruhan, keanggotaan_suhu)); // Rule 11
    float rule12 = min_f(keanggotaan_ph, min_f(keanggotaan_kekeruhan, keanggotaan_suhu)); // Rule 12
    float rule13 = min_f(keanggotaan_ph, min_f(keanggotaan_kekeruhan, keanggotaan_suhu)); // Rule 13
    float rule14 = min_f(keanggotaan_ph, min_f(keanggotaan_kekeruhan, keanggotaan_suhu)); // Rule 14
    float rule15 = min_f(keanggotaan_ph, min_f(keanggotaan_kekeruhan, keanggotaan_suhu)); // Rule 15
    float rule16 = min_f(keanggotaan_ph, min_f(keanggotaan_kekeruhan, keanggotaan_suhu)); // Rule 16
    float rule17 = min_f(keanggotaan_ph, min_f(keanggotaan_kekeruhan, keanggotaan_suhu)); // Rule 17
    float rule18 = min_f(keanggotaan_ph, min_f(keanggotaan_kekeruhan, keanggotaan_suhu)); // Rule 18
    float rule19 = min_f(keanggotaan_ph, min_f(keanggotaan_kekeruhan, keanggotaan_suhu)); // Rule 19
    float rule20 = min_f(keanggotaan_ph, min_f(keanggotaan_kekeruhan, keanggotaan_suhu)); // Rule 20
    float rule21 = min_f(keanggotaan_ph, min_f(keanggotaan_kekeruhan, keanggotaan_suhu)); // Rule 21
    float rule22 = min_f(keanggotaan_ph, min_f(keanggotaan_kekeruhan, keanggotaan_suhu)); // Rule 22
    float rule23 = min_f(keanggotaan_ph, min_f(keanggotaan_kekeruhan, keanggotaan_suhu)); // Rule 23
    float rule24 = min_f(keanggotaan_ph, min_f(keanggotaan_kekeruhan, keanggotaan_suhu)); // Rule 24
    float rule25 = min_f(keanggotaan_ph, min_f(keanggotaan_kekeruhan, keanggotaan_suhu)); // Rule 25

    return max_f(max_f(max_f(max_f(rule1, rule2), max_f(rule3, rule4)), max_f(rule5, rule6)), max_f(max_f(rule7, rule8), max_f(rule9, rule10))),
           max_f(max_f(rule11, rule12), max_f(rule13, rule14)), max_f(max_f(rule15, rule16), max_f(rule17, rule18)),
          max_f(max_f(rule19, rule20), max_f(rule21, rule22)), max_f(max_f(rule23, rule24), rule25);
}
// Fungsi untuk menghitung tingkat keanggotaan Kualitas Air Baik
float kualitas_air_baik(float keanggotaan_ph, float keanggotaan_kekeruhan, float keanggotaan_suhu) {
   float rule26 = min_f(keanggotaan_ph, min_f(keanggotaan_kekeruhan, keanggotaan_suhu)); // Rule 26
   float rule27 = min_f(keanggotaan_ph, min_f(keanggotaan_kekeruhan, keanggotaan_suhu)); // Rule 27
   return max_f(rule26, rule27);
}

 
// Deklarasi variabel global untuk menampung tingkat keanggotaan dan nilai minimum 
float tingkat_keanggotaan[2]; 
float nilai_minimum[2] = {1, 0}; // Nilai minimum untuk kualitas air buruk, baik 
 
// Fungsi untuk melakukan defuzzifikasi 
float defuzzyfikasi() { 
    float A = 0; 
    float B = 0; 
 
    for (int i = 0; i < 2; i++) { 
        A += tingkat_keanggotaan[i] * nilai_minimum[i]; 
        B += tingkat_keanggotaan[i]; 
    } 
 
    if (B != 0) { 
        return A / B; // Menghitung nilai defuzzifikasi 
    } else { 
        return 0; // Menghindari pembagian dengan nol 
    } 
}
 
void setup() {
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(1000);

  // Connect to WiFi
  Serial.println();
  Serial.println();
  Serial.print("Connecting to: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.print("-");

  }

  Serial.println("");
  Serial.println("WiFi Connected");

  // Print the IP address
  Serial.print("IP Address: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

    EEPROM.begin(512);  //Initialize EEPROM untuk menyimpan nilai kalibrasi sensor TDS
    gravityTds.setPin(TdsSensorPin);
    gravityTds.setAref(3.3);  //reference voltage on ADC, default 5.0V on Arduino UNO
    gravityTds.setAdcRange(4096);  //1024 for 10bit ADC;4096 for 12bit ADC
    gravityTds.begin();  //initialization

    //set lcd
  lcd.begin(20, 4);
  lcd.backlight();
  lcd.clear();
  lcd.noCursor();
  lcd.init();

  //atur relay sebagai output
  pinMode (R1, OUTPUT);
  pinMode (R2, OUTPUT);
  pinMode (R3, OUTPUT);
  pinMode (R4, OUTPUT);

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

} 


void PH(){
    int nilai_analog_Ph=0;

    for (int i = 0; i < 10; i++)
    {
    nilai_analog_Ph += analogRead(pin_Ph);
        delay(100);

    }

  TeganganPh = (nilai_analog_Ph * (3.3 / 4095.0))/10  ; //nilai tegangan bisa berubah tergantung ouput tegangan mikrokontroller
  Serial.print("Tegangan Ph: ");
  Serial.print(TeganganPh, 3); //desimal 3 angka 
  Serial.print (" | ");

  PH_step = (PH4 - PH7) / 3;  
  Po = 7.00 + ((PH7 - TeganganPh) / PH_step)   ;  // Po = 7.00 + ((TeganganPh7 - TeganganPh) / PH_step) + phkalibrasi
  Serial.print("Nilai Ph air: ");
  Serial.print(Po, 2); //desimal 2 angka 
  Serial.println (" | ");
  //value1 = Po;
  //delay (2000);

    lcd.setCursor(0,0);
    lcd.print("PH: ");
    lcd.print(Po);


    delay(1000);

}


void TDS (){

    gravityTds.setTemperature(Temperaturekoreksi);  // set the temperature and execute temperature compensation
    gravityTds.update();  //sample and calculate
    tdsValue = gravityTds.getTdsValue();  // then get the value
      Serial.print("Kekeruhan: ");
    Serial.print(tdsValue);
    Serial.println(" PPM");
    delay(1000);
    lcd.setCursor(0,1);
    lcd.print("Kekeruhan: ");
    lcd.print(tdsValue);
    lcd.print(" PPM");

    delay(1000);

}


void suhu(){
    sensors.requestTemperatures();
    Temperature = sensors.getTempCByIndex(0);
    Temperaturekoreksi = 0.8948 * Temperature + 2.89 ; //intercept = 2.89 ,  suhu bertambah sekitar 0.10

    //Serial.println("");
    Serial.print("Temperature Air sebelum koreksi: ");
    Serial.print(Temperature);
    Serial.print (" | ");
    //delay (1000);
    Serial.print("Temperature Air sesudah koreksi: ");
    Serial.print(Temperaturekoreksi);    
    Serial.println("°C");


    lcd.setCursor(0,2);
    lcd.print("Suhu: ");
    lcd.print(Temperaturekoreksi);
    lcd.print(" oC");


    delay(1000);

} 

void controlPH (){
       if (Po < 6.5) {
      digitalWrite(R1, HIGH); // terhubung ke NO
      digitalWrite(R3, HIGH);
      Status_Relay_1 = "ON";
      delay (30000);
      digitalWrite (R1, LOW);
      digitalWrite (R2, LOW);
      digitalWrite (R3, LOW);      
      Status_Relay_2 = "OFF";
    }

    else if (Po > 8.5) {
      digitalWrite (R2, HIGH);
      digitalWrite (R3, HIGH);
      Status_Relay_2 = "ON";      
      delay (30000);
      digitalWrite (R2, LOW);
      digitalWrite (R1, LOW); 
      digitalWrite (R3, LOW);     
      Status_Relay_1 = "OFF";
    }
    else {
      digitalWrite (R1, LOW);
      Status_Relay_1 = "OFF";      
      digitalWrite (R2, LOW);
      Status_Relay_2 = "OFF";      
      digitalWrite (R3, LOW);
      Status_Relay_3 = "OFF";
    }
}
void controlKekeruhan (){
      if (tdsValue > 300) {
      digitalWrite(R3, HIGH); // terhubung ke NO
      delay(600000);
      Status_Relay_3 = "ON";
      digitalWrite(R3, LOW);
    }
    else {    
      digitalWrite (R3, LOW);
      Status_Relay_3 = "OFF";
    }
}
void controlSuhu (){
  if (Temperaturekoreksi < 25) {
      digitalWrite(R4, HIGH); // terhubung ke NO
      delay(120000);
      digitalWrite(R4, LOW);  
      Status_Relay_4 = "ON";    
      digitalWrite(R3, HIGH); // terhubung ke NO
      delay(120000);
      digitalWrite(R3, LOW);
    }
    else {     
      digitalWrite (R3, LOW);
      Status_Relay_3 = "OFF";
      digitalWrite (R4, LOW);
      Status_Relay_4 = "OFF";
    }
}

void Controller (){
  controlPH();
  controlKekeruhan();
  controlSuhu();
}

void Read_Actuator_State() {
  firebase.setString("/R1", Status_Relay_1); 
  firebase.setString("/R2", Status_Relay_2); 
  firebase.setString("/R3", Status_Relay_3); 
  firebase.setString("/R4", Status_Relay_4); 

  Serial.println();
  Serial.println("-------------");
  Serial.print(F("Relay 1 : "));
  Serial.print(Status_Relay_1);
  Serial.print(F(" | Relay_2 : "));
  Serial.println(Status_Relay_2);
  Serial.print(F("Relay 3 : "));
  Serial.print(Status_Relay_3);
  Serial.print(F(" | Relay_4 : "));
  Serial.println(Status_Relay_4);
  Serial.println("-------------");
  delay(1000);
}

void fuzzy() { 

    // Lakukan inferensi dan hitung tingkat keanggotaan serta nilai minimum untuk setiap aturan fuzzy (rule) 
    float kualitas_air_buruk_1 = kualitas_air_buruk(func_ph_asam(Po), func_kekeruhan_sedang(tdsValue), func_suhu_sedang(Temperaturekoreksi));
    float kualitas_air_buruk_2 = kualitas_air_buruk(func_ph_asam(Po), func_kekeruhan_rendah(tdsValue), func_suhu_dingin(Temperaturekoreksi));
    float kualitas_air_buruk_3 = kualitas_air_buruk(func_ph_asam(Po), func_kekeruhan_tinggi(tdsValue), func_suhu_panas(Temperaturekoreksi));
    float kualitas_air_buruk_4 = kualitas_air_buruk(func_ph_basa(Po), func_kekeruhan_sedang(tdsValue), func_suhu_sedang(Temperaturekoreksi));
    float kualitas_air_buruk_5 = kualitas_air_buruk(func_ph_basa(Po), func_kekeruhan_rendah(tdsValue), func_suhu_dingin(Temperaturekoreksi));
    float kualitas_air_buruk_6 = kualitas_air_buruk(func_ph_basa(Po), func_kekeruhan_tinggi(tdsValue), func_suhu_panas(Temperaturekoreksi));
    float kualitas_air_buruk_7 = kualitas_air_buruk(func_ph_netral(Po), func_kekeruhan_rendah(tdsValue), func_suhu_dingin(Temperaturekoreksi));
    float kualitas_air_buruk_8 = kualitas_air_buruk(func_ph_netral(Po), func_kekeruhan_tinggi(tdsValue), func_suhu_panas(Temperaturekoreksi));
    float kualitas_air_baik_1 = kualitas_air_baik(func_ph_netral(Po), func_kekeruhan_sedang(tdsValue), func_suhu_sedang(Temperaturekoreksi));
    float kualitas_air_baik_2 = kualitas_air_baik(func_ph_netral(Po), func_kekeruhan_rendah(tdsValue), func_suhu_sedang(Temperaturekoreksi));
    
 
// Mengisi tingkat keanggotaan untuk setiap aturan berdasarkan hasil inferensi
    tingkat_keanggotaan[1] = max(kualitas_air_buruk_1, max(kualitas_air_buruk_2, max(kualitas_air_buruk_3, max(kualitas_air_buruk_4, max(kualitas_air_buruk_5, max(kualitas_air_buruk_6, max(kualitas_air_buruk_7, kualitas_air_buruk_8)))))));
    tingkat_keanggotaan[0] = max(kualitas_air_baik_1, kualitas_air_baik_2); 
 
    // Melakukan defuzzifikasi dan mendapatkan output dari sistem 
    float output_sistem = defuzzyfikasi(); 
 
    // Lakukan tindakan selanjutnya berdasarkan output sistem 
 
if (output_sistem <= 0) { 
    // Kualitas air Buruk 
    kualitas_air = "Non-Ideal"; 

} else if ( 0 < output_sistem && output_sistem <= 1) { 
    // Kualitas air Baik 
    kualitas_air = "Ideal"; 
} 
    // Print hasil kualitas air 
    Serial.print("Hasil Defuzzifikasi = ");
    Serial.print(output_sistem);
    Serial.print(" | ");
    // Print hasil kualitas air 
    Serial.println("Status kualitas air = " + kualitas_air);

//menampilkan status kualitas air pada display alat
    lcd.setCursor(0,3);
    lcd.print("Status Air:");
    lcd.print(kualitas_air);
    delay(1000);
}


  void send_data_from_googlesheet () {

    //SEND DATA TO GOOGLE SHEET
      // Check if any reads failed and exit early (to try again).
  if (isnan(Po) || isnan(tdsValue)  || isnan (Temperaturekoreksi)) {
    Serial.println();
    Serial.println(F("Failed to read from sensor!"));
    Serial.println();

    Status_Read_Sensor = "Failed";

  } 
  else {
    Status_Read_Sensor = "Success";
  }
  Serial.println();
  Serial.println("-------------");
  Serial.print(F("Status_Read_Sensor : "));
  Serial.print(Status_Read_Sensor);

  if (WiFi.status() == WL_CONNECTED) {
    // Create a URL for sending or writing data to Google Sheets.
    String Send_Data_URL = Web_App_URL + "?sts=write";
    Send_Data_URL += "&srs=" + Status_Read_Sensor;
    Send_Data_URL += "&PH=" + String(Po);
    Send_Data_URL += "&Kekeruhan=" + String(tdsValue);
    Send_Data_URL += "&Suhu=" + String(Temperaturekoreksi);
    Send_Data_URL += "&R1=" + Status_Relay_1;
    Send_Data_URL += "&R2=" + Status_Relay_2;
    Send_Data_URL += "&R3=" + Status_Relay_3;
    Send_Data_URL += "&R4=" + Status_Relay_4;
    Send_Data_URL += "&kualitas_air_gsheet=" + kualitas_air;
    Serial.println();
    Serial.println("-------------");
    Serial.println("Send data to Google Spreadsheet...");
    Serial.print("URL : ");
    Serial.println(Send_Data_URL);

    //::::::::::::::::::The process of reading or getting data from Google Sheets.
      // Initialize HTTPClient as "http".
      HTTPClient http;

      // HTTP GET Request.
      http.begin(Send_Data_URL.c_str());
      //http.begin(Read_Data_URL.c_str());
      http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

      // Gets the HTTP status code.
      int httpCode = http.GET(); 
      Serial.print("HTTP Status Code : ");
      Serial.println(httpCode);
  
      // Getting response from google sheet.
      String payload;
      if (httpCode > 0) {
        payload = http.getString();
        Serial.println("Payload : " + payload); }
        
      http.end(); 

  }
      delay(1000);
  }


 void send_to_firebase () {
        // Kirim data ke Firebase 
      firebase.setFloat("/Data_PH", Po);    
      firebase.setFloat("/Data_TDS", tdsValue);    
      firebase.setFloat("/Data_suhu", Temperaturekoreksi);    
      firebase.setString("/kualitas air", kualitas_air);

  delay (1000);
 }


void loop() {
  DateTime now = rtc.now();
  jam     = now.hour();
  menit   = now.minute();
  detik   = now.second();
  tanggal = now.day();
  bulan   = now.month();
  tahun   = now.year();
  hari    = daysOfTheWeek[now.dayOfTheWeek()];
  //Serial.println(String() + hari + ", " + tanggal + "/" + bulan + "/" + tahun + "  " + jam + ":" + menit + ":" + detik);

    if (jam == 0 && menit == 0 && detik == 0) {
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
    else if(jam == 0 && menit == 15 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 0 && menit == 30 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
  else if(jam == 0 && menit == 45 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 deti
  } 
  else if(jam == 1 && menit == 0 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
    else if(jam == 1 && menit == 15 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 1 && menit == 30 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
  else if(jam == 1 && menit == 45 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  } 
  else if(jam == 2 && menit == 0 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  } 
    else if(jam == 2 && menit == 15 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 2 && menit == 30 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
  else if(jam == 2 && menit == 45 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 3 && menit == 0 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  } 
    else if(jam == 3 && menit == 15 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 3 && menit == 30 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
  else if(jam == 3 && menit == 45 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 4 && menit == 0 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  } 
    else if(jam == 4 && menit == 15 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 4 && menit == 30 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
  else if(jam == 4 && menit == 45 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 5 && menit == 0 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  } 
    else if(jam == 5 && menit == 15 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 5 && menit == 30 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
  else if(jam == 5 && menit == 45 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 6 && menit == 0 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  } 
    else if(jam == 6 && menit == 15 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 6 && menit == 30 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
  else if(jam == 6 && menit == 45 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 7 && menit == 0 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  } 
    else if(jam == 7 && menit == 15 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 7 && menit == 30 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
  else if(jam == 7 && menit == 45 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 8 && menit == 0 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  } 
    else if(jam == 8 && menit == 15 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 8 && menit == 30 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
  else if(jam == 8 && menit == 45 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 9 && menit == 0 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  } 
    else if(jam == 9 && menit == 15 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 9 && menit == 30 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
  else if(jam == 9 && menit == 45 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 10 && menit == 0 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
    else if(jam == 10 && menit == 15 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 10 && menit == 30 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
  else if(jam == 10 && menit == 45 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  } 
  else if(jam == 11 && menit == 0 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  } 
    else if(jam == 11 && menit == 15 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 11 && menit == 30 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
  else if(jam == 11 && menit == 45 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 12 && menit == 0 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  } 
    else if(jam == 12 && menit == 15 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 12 && menit == 30 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
  else if(jam == 12 && menit == 45 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 13 && menit == 0 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  } 
  else if(jam == 13 && menit == 15 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 13 && menit == 30 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
  else if(jam == 13 && menit == 45 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 14 && menit == 0 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  } 
  else if(jam == 14 && menit == 15 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 14 && menit == 30 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
  else if(jam == 14 && menit == 45 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 15 && menit == 0 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  } 
  else if(jam == 15 && menit == 15 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 15 && menit == 30 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
  else if(jam == 15 && menit == 45 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 16 && menit == 0 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  } 
  else if(jam == 16 && menit == 15 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 16 && menit == 30 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
  else if(jam == 16 && menit == 45 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 17 && menit == 0 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  } 
  else if(jam == 17 && menit == 15 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 17 && menit == 30 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
  else if(jam == 17 && menit == 45 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 18 && menit == 0 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
  else if(jam == 18 && menit == 15 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 18 && menit == 30 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
  else if(jam == 18 && menit == 45 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  } 
  else if(jam == 19 && menit == 0 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
  else if(jam == 19 && menit == 15 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 19 && menit == 30 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
  else if(jam == 19 && menit == 45 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  } 
  else if(jam == 20 && menit == 0 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik
  send_to_firebase (); // 1 detik
  }
  else if(jam == 20 && menit == 15 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 20 && menit == 30 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
  else if(jam == 20 && menit == 45 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  } 
  else if(jam == 21 && menit == 0 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
  else if(jam == 21 && menit == 15 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 21 && menit == 30 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
  else if(jam == 21 && menit == 45 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 22 && menit == 0 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
  else if(jam == 22 && menit == 15 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 22 && menit == 30 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
  else if(jam == 22 && menit == 45 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 23 && menit == 0 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik
  } 
  else if(jam == 23 && menit == 15 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else if(jam == 23 && menit == 30 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay
  
  fuzzy (); //1 detik
  send_to_firebase (); // 1 detik
  Controller();
  Read_Actuator_State(); // 1 detik
  send_data_from_googlesheet (); // 1 detik

  }
  else if(jam == 23 && menit == 45 && detik == 0){
  PH(); //61 detik , 30 detik data, 30 detik relay, 1 detik delay
  suhu(); // 241 detik,  120 detik relay, 120 detik, 1 detik delay
  TDS(); // 602 detik, 1 detik data, 600 detik relay, 1 detik delay

  fuzzy (); //1 detik
  Read_Actuator_State(); // 1 detik
  }
  else{
  } 
}