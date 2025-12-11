#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// --- KONFIGURASI WIFI & MQTT (UBAH DISINI) ---
const char* ssid = "hiqaffyed";
const char* password = "12345678";

// PENTING: Gunakan IP Laptop (Windows) Anda, BUKAN IP VM Ubuntu
const char* mqtt_server = "10.147.86.132"; 

// --- DEFINISI PIN ---
#define DHTPIN 4      // Pin Data DHT11
#define DHTTYPE DHT11 // Jenis Sensor DHT
#define LED_PIN 2     // Pin LED Eksternal
#define LDR_PIN 34    // Pin Analog Sensor Cahaya

// --- OBJEK & VARIABEL ---
WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Menghubungkan ke WiFi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi Terhubung!");
  Serial.print("IP Address ESP32: ");
  Serial.println(WiFi.localIP());
}

// Fungsi untuk menerima pesan dari MQTT (Contoh: Menyalakan LED)
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Pesan masuk [");
  Serial.print(topic);
  Serial.print("]: ");

  String messageTemp;
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    messageTemp += (char)payload[i];
  }
  Serial.println();

  // Cek jika topik adalah kontrol LED
  if (String(topic) == "33424216/led") {
    if (messageTemp == "ON") {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("-> LED DINYALAKAN");
    }
    else if (messageTemp == "OFF") {
      digitalWrite(LED_PIN, LOW);
      Serial.println("-> LED DIMATIKAN");
    }
  }
}

void reconnect() {
  // Loop sampai terhubung kembali
  while (!client.connected()) {
    Serial.print("Mencoba koneksi MQTT...");
    
    // ID Client Random agar tidak bentrok
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    // Mencoba connect
    if (client.connect(clientId.c_str())) {
      Serial.println("Berhasil Terhubung!");
      
      // Subscribe ke topik kontrol LED
      client.subscribe("33424216/led");
    } else {
      Serial.print("Gagal, rc=");
      Serial.print(client.state());
      Serial.println(" coba lagi dalam 5 detik");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // Setup Pin
  pinMode(LED_PIN, OUTPUT);
  pinMode(LDR_PIN, INPUT);
  
  dht.begin();
  setup_wifi();
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Kirim data setiap 2 detik
  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;

    // 1. Baca Sensor Suhu (Sama seperti sebelumnya)
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    // 2. Baca Sensor Cahaya (DIGITAL)
    // Hasilnya hanya HIGH (1) atau LOW (0)
    int ldrStatus = digitalRead(LDR_PIN); 

    // Cek jika sensor DHT error
    if (isnan(h) || isnan(t)) {
      Serial.println("Gagal membaca sensor DHT!");
      return;
    }

    // Publish Suhu
    String suhustr = String(t);
    client.publish("33424216/suhu", suhustr.c_str());

    // Publish Kelembaban
    String humstr = String(h);
    client.publish("33424216/kelembaban", humstr.c_str());

    // 3. Publish Cahaya (Logika Digital)
    // Biasanya modul LDR: 
    // HIGH (1) = Gelap (Lampu indikator di modul mati)
    // LOW (0)  = Terang (Lampu indikator di modul nyala)
    // Tapi bisa terbalik tergantung merk, silakan dites.
    
    String cahayaPesan = "";
    if (ldrStatus == HIGH) {
      cahayaPesan = "Gelap";
    } else {
      cahayaPesan = "Terang";
    }
    
    client.publish("33424216/cahaya", cahayaPesan.c_str());
    
    // Tampilkan di Serial Monitor untuk debugging
    Serial.print("Suhu: "); Serial.print(t);
    Serial.print(" | Lembab: "); Serial.print(h);
    Serial.print(" | Cahaya: "); Serial.println(cahayaPesan);
  }
}