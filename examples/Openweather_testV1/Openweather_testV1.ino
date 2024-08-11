#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "SEAN_LAPTOP_4896";
const char* password = "123456789";

// Biến lưu trữ tọa độ
float lat = 11.948036748747358;  // Ví dụ: Hà Nội
float lon =  108.43815802610034; // Ví dụ: Hà Nội

String apiKey = "1e572ba239e0349a69a29be7466a3b6c"; // API key mặc định

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Tạo URL động dựa trên giá trị lat, lon và API key
    String serverPath = "http://api.openweathermap.org/data/2.5/weather?lat=" + String(lat) + "&lon=" + String(lon) + "&appid=" + apiKey + "&units=metric";

    http.begin(serverPath);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String payload = http.getString();
      Serial.println(payload);

      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);

      float temperature = doc["main"]["temp"];
      int humidity = doc["main"]["humidity"];
      String weather = doc["weather"][0]["main"];

      Serial.print("Temperature: ");
      Serial.println(temperature);
      Serial.print("Humidity: ");
      Serial.println(humidity);
      Serial.print("Weather: ");
      Serial.println(weather);
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
  delay(60000); // Lấy dữ liệu mỗi phút một lần
}

// Hàm để cập nhật API key
void updateApiKey(String newApiKey) {
  apiKey = newApiKey;
  Serial.print("API key updated to: ");
  Serial.println(apiKey);
}
