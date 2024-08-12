#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

// Thông tin WiFi
const char* ssid = "HshopLTK";
const char* password = "HshopLTK@2311";

// Thông tin API
float lat = 11.948036748747358;  // Tọa độ hiện tại
float lon = 108.43815802610034; // Tọa độ hiện tại
String apiKey = "1e572ba239e0349a69a29be7466a3b6c"; // API key mặc định

// Chân điều khiển LED RGB cơ bản
const int redPin = 20;
const int greenPin = 21;
const int bluePin = 10;

// Chân điều khiển LED WS2812
const int ledPin = 4;
const int numPixels = 2000; // Số lượng LED WS2812

// Tham số hiệu ứng
const int pulseSpeed = 10; // Tốc độ dập dìu chậm hơn
const int colorRange = 25; // Khoảng ±10 cho màu sắc
const int minBrightness = 30; // Độ sáng thấp nhất (40%)

unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 60000; // Cập nhật dữ liệu mỗi 60 giây

unsigned long lastPulseTime = 0;
 unsigned long pulseInterval = 100; // Tốc độ của hiệu ứng dập dìu chậm lại

int pulseDirection = 1; // 1 để tăng độ sáng, -1 để giảm độ sáng
int brightness = minBrightness*3; // Giá trị độ sáng hiện tại khởi đầu từ 40%

int baseRedValue, baseGreenValue, baseBlueValue;
bool weatherDataUpdated = false;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(numPixels, ledPin, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  // Cấu hình chân LED RGB cơ bản
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  // Cấu hình LED WS2812
  strip.begin();
  strip.show(); // Khởi tạo tất cả LED tắt

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
  lastUpdateTime = millis();

      fetchWeatherData();
    weatherDataUpdated = true;
}

void loop() {
  unsigned long currentMillis = millis();

  // Cập nhật dữ liệu thời tiết mỗi 60 giây
  if (currentMillis - lastUpdateTime >= updateInterval) {
    lastUpdateTime = currentMillis;
    fetchWeatherData();
    weatherDataUpdated = true;
  }

  // Hiệu ứng dập dìu cho cả LED RGB và LED WS2812
  if (weatherDataUpdated) {
    if (currentMillis - lastPulseTime >= pulseInterval) {
      pulseInterval = random(10,1000);
      lastPulseTime = currentMillis;

      // Cập nhật độ sáng
      brightness += pulseDirection * random(-20,20); // Thay đổi độ sáng chậm hơn
      if (brightness >= 255 || brightness <= minBrightness) {
        pulseDirection *= -1;
        brightness = constrain(brightness, minBrightness, 255); // Đảm bảo độ sáng không thấp hơn 40%
      }

      // Thay đổi màu sắc với hiệu ứng dập dìu
      int redValue = baseRedValue + random(-colorRange, colorRange + 1);
      int greenValue = baseGreenValue + random(-colorRange, colorRange + 1);
      int blueValue = baseBlueValue + random(-colorRange, colorRange + 1);

      // Đảm bảo giá trị màu nằm trong khoảng 0 đến 255
      redValue = constrain(redValue, 0, 255);
      greenValue = constrain(greenValue, 0, 255);
      blueValue = constrain(blueValue, 0, 255);

      // In ra giá trị màu và độ sáng hiện tại
      Serial.print("LED RGB Value - R: ");
      Serial.print(redValue);
      Serial.print(" G: ");
      Serial.print(greenValue);
      Serial.print(" B: ");
      Serial.print(blueValue);
      Serial.print(" | Brightness: ");
      Serial.println(brightness);

      // Cập nhật LED RGB cơ bản
      analogWrite(redPin, (redValue * brightness) / 255);
      analogWrite(greenPin, (greenValue * brightness) / 255);
      analogWrite(bluePin, (blueValue * brightness) / 255);

      // Cập nhật LED WS2812
      for (int i = 0; i < numPixels; i++) {
        strip.setPixelColor(i, strip.Color(
          (redValue * brightness) / 255,
          (greenValue * brightness) / 255,
          (blueValue * brightness) / 255
        ));
      }
      strip.show();
    }
  }
}

void fetchWeatherData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String serverPath = "http://api.openweathermap.org/data/2.5/weather?lat=" + String(lat) + "&lon=" + String(lon) + "&appid=" + apiKey + "&units=metric";

    http.begin(serverPath);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String payload = http.getString();
      Serial.println(payload);

      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);

      String weather = doc["weather"][0]["main"];

      Serial.print("Weather Condition: ");
      Serial.println(weather);


// weather = "Thunderstorm";

      // Điều chỉnh màu sắc cơ bản dựa trên tình trạng thời tiết
      if (weather == "Clear") {
        baseRedValue = 255;
        baseGreenValue = 235;
        baseBlueValue = 0; // Vàng: Nắng
      } else if (weather == "Clouds") {
        baseRedValue = 192;
        baseGreenValue = 192;
        baseBlueValue = 192; // Xám: Có mây
      } else if (weather == "Rain") {
        baseRedValue = 0;
        baseGreenValue = 0;
        baseBlueValue = 255; // Xanh dương: Mưa
      } else if (weather == "Drizzle") {
        baseRedValue = 0;
        baseGreenValue = 191;
        baseBlueValue = 255; // Xanh nhạt: Mưa phùn
      } else if (weather == "Thunderstorm") {
        baseRedValue = 128;
        baseGreenValue = 0;
        baseBlueValue = 128; // Tím: Dông bão
      } else if (weather == "Snow") {
        baseRedValue = 255;
        baseGreenValue = 255;
        baseBlueValue = 255; // Trắng: Tuyết
      } else if (weather == "Mist") {
        baseRedValue = 173;
        baseGreenValue = 216;
        baseBlueValue = 230; // Xanh lá nhạt: Sương mù
      } else if (weather == "Smoke") {
        baseRedValue = 139;
        baseGreenValue = 69;
        baseBlueValue = 19; // Nâu xám: Khói
      } else if (weather == "Haze") {
        baseRedValue = 255;
        baseGreenValue = 165;
        baseBlueValue = 0; // Cam nhạt: Sương mù nhẹ
      } else if (weather == "Dust") {
        baseRedValue = 210;
        baseGreenValue = 180;
        baseBlueValue = 140; // Vàng nhạt: Bụi
      } else if (weather == "Fog") {
        baseRedValue = 169;
        baseGreenValue = 169;
        baseBlueValue = 169; // Xám đậm: Sương mù dày đặc
      } else if (weather == "Sand") {
        baseRedValue = 244;
        baseGreenValue = 164;
        baseBlueValue = 96; // Vàng nhạt: Cát
      } else if (weather == "Ash") {
        baseRedValue = 105;
        baseGreenValue = 105;
        baseBlueValue = 105; // Xám đen: Tro bụi núi lửa
      } else if (weather == "Squall") {
        baseRedValue = 46;
        baseGreenValue = 139;
        baseBlueValue = 87; // Xanh lục đậm: Gió giật
      } else if (weather == "Tornado") {
        baseRedValue = 255;
        baseGreenValue = 0;
        baseBlueValue = 0; // Đỏ: Lốc xoáy
      } else {
        baseRedValue = 255;
        baseGreenValue = 255;
        baseBlueValue = 255; // Mặc định: Trắng nếu thời tiết không xác định
      }
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
}
