#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <DHT.h>

// 1. 修改您的網路與 Google 資訊
const char* ssid = "STUDENT-C2-3";
const char* password = "28721940";
// 填入您部署的 Google Apps Script ID (即 /macros/s/ 與 /exec 之間那一長串字串)
const char* GScriptID = "AKfycbxj8ZOR9xdbyEMEvPoi01b7QOQ6ZXY0etCOHzpu17qpAHZ1ZgEmtFTG0sKBN70D5lQO9Q"; 

// 2. 初始化感測器腳位
#define DHTPIN D4        // DHT11 DATA 腳位接在 WeMos D4 (GPIO2)
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
Adafruit_INA219 ina219;

void setup() {
  Serial.begin(115200);
  delay(10);
  
  // 初始化 I2C 腳位：D1 為 SCL，D2 為 SDA
  Wire.begin(D2, D1); 
  
  Serial.println("\n--- 系統初始化 ---");
  dht.begin();
  
  if (!ina219.begin()) {
    Serial.println("警告：找不到 INA219 晶片，請檢查接線！");
  }

  // 開始連線 Wi-Fi
  Serial.print("正在連線至 Wi-Fi: "); Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi 連線成功！");
  Serial.print("IP 位址: "); Serial.println(WiFi.localIP());
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    // 讀取 INA219 電力數據
    float busvoltage = ina219.getBusVoltage_V();
    float current_mA = ina219.getCurrent_mA();
    float power_mW = ina219.getPower_mW();

    // 讀取 DHT11 溫溼度數據
    float temp = dht.readTemperature();
    float humi = dht.readHumidity();

    // 檢查 DHT11 是否讀取失敗
    if (isnan(temp) || isnan(humi)) {
      Serial.println("無法從 DHT11 感測器讀取資料！");
      temp = 0.0;
      humi = 0.0;
    }

    Serial.print("溫度: "); Serial.print(temp); Serial.print(" °C | ");
    Serial.print("電壓: "); Serial.print(busvoltage); Serial.println(" V");

    // 建立加密安全連線客戶端 (處理 Google HTTPS)
    WiFiClientSecure client;
    client.setInsecure(); // 簡化開發：忽略驗證證書
    
    HTTPClient http;
    
    // 構建帶有參數的 URL 請求網址
    String url = "https://script.google.com/macros/s/" + String(GScriptID) + "/exec?";
    url += "voltage=" + String(busvoltage);
    url += "&current=" + String(current_mA);
    url += "&power=" + String(power_mW);
    url += "&temp=" + String(temp);
    url += "&humi=" + String(humi);

    Serial.println("發送數據至 Google Sheets...");
    
    // 設定 http 允許跟隨 Google 的 302 重導向 (關鍵！)
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    
    if (http.begin(client, url)) {
      int httpCode = http.GET();
      if (httpCode > 0) {
        Serial.print("Google 回應狀態碼: "); Serial.println(httpCode);
        String payload = http.getString();
        Serial.print("伺服器訊息: "); Serial.println(payload);
      } else {
        Serial.print("發送失敗，錯誤代碼: "); Serial.println(http.errorToString(httpCode).c_str());
      }
      http.end();
    }
  } else {
    Serial.println("Wi-Fi 斷線，嘗試重新連線...");
    WiFi.begin(ssid, password);
  }

  delay(60000); // 每 60 秒讀取並上傳一次
}
