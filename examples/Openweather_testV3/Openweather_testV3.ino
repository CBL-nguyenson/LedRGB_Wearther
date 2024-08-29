#include "Config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

enum State_
{
  Weather_Mode,
  Randbow_Mode,
  ThingSpeak_Mode
};
uint8_t Set_State_mode = Randbow_Mode;

void fetchWeatherData();

// Thông tin WiFi
// const char *ssid = "HshopLTK";
// const char *password = "HshopLTK@2311";
// const char *ssid = "CTY CP XAY DUNG 676";
// const char *password = "congtyxaydung676";

const char *ssid = "SEAN_LAPTOP_4896";
const char *password = "123456789";

// const char *ssid = "goctinh kafe";
// const char *password = "ditheobongmattroi";

// Thông tin API
float lat = 11.948036748747358;                     // Tọa độ hiện tại
float lon = 108.43815802610034;                     // Tọa độ hiện tại
String apiKey = "1e572ba239e0349a69a29be7466a3b6c"; // API key mặc định

// Chân điều khiển LED RGB cơ bản
const int redPin = 20;
const int greenPin = 21;
const int bluePin = 10;
// Chân điều khiển LED WS2812
const int ledPin = 4;
const int numPixels = 6000; // Số lượng LED WS2812
int hue = 0;

int redValue = 0;
int greenValue = 0;
int blueValue = 0;
int brightness = 0;
int Control_redValue = 127;
int Control_greenValue = 127;
int Control_blueValue = 127;
int Control_brightness = 255;
int baseRedValue, baseGreenValue, baseBlueValue;

// Tham số hiệu ứng
const int brightness_range = 40; // Khoảng ±40 cho màu sắc
const int colorRange = 35;       // Khoảng ±25 cho màu sắc
const int minBrightness = 20;    // Độ sáng thấp nhất

const unsigned long updateInterval = 60000 * 5; // Cập nhật dữ liệu mỗi 60 giây
unsigned long lastUpdateTime = 0;

int pulseDirection = 2; // 1 để tăng độ sáng, -1 để giảm độ sáng

bool weatherDataUpdated = false;

bool Is_changes_data_colour = false;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(numPixels, ledPin, NEO_GRB + NEO_KHZ800);

//------------------------------------OTA------------------------------------
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

bool Is_Setup_OTA = false;
void Setup_OTA()
{
  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname("Bri_weather");

  // No authentication by default
  ArduinoOTA.setPassword("Bri_weather");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]()
                     {
              String type;
              if (ArduinoOTA.getCommand() == U_FLASH)
                type = "sketch";
              else  // U_SPIFFS
                type = "filesystem";

              // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
              Serial.println("Start updating " + type); })
      .onEnd([]()
             { Serial.println("\nEnd"); })
      .onProgress([](unsigned int progress, unsigned int total)
                  { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); })
      .onError([](ota_error_t error)
               {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed"); });

  ArduinoOTA.begin();
  Is_Setup_OTA = true;
}
//---------------------------------------------------------------------------

void setup()
{
  Serial.begin(115200);

  // Cấu hình chân LED RGB cơ bản
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  digitalWrite(redPin, LOW);
  digitalWrite(greenPin, LOW);
  digitalWrite(bluePin, LOW);

  // Cấu hình LED WS2812
  strip.begin();
  strip.show(); // Khởi tạo tất cả LED tắt
                //-------------------------------------------------------------------------------------------------------
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    analogWrite(redPin, 200);
    analogWrite(greenPin, 0);
    analogWrite(bluePin, 0);
    delay(5000);

    ESP.restart();
  }

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Setup_OTA();
  //-------------------------------------------------------------------------------------------------------
  Serial.println("Connected to WiFi");
  lastUpdateTime = millis();

  fetchWeatherData();
  weatherDataUpdated = true;
}

void loop()
{
  if (Is_Setup_OTA)
  {
    ArduinoOTA.handle();
  }
  // Cập nhật dữ liệu thời tiết mỗi 60 giây
  if (millis() - lastUpdateTime >= updateInterval)
  {
    lastUpdateTime = millis();
    fetchWeatherData(); // Nhận data
  }

  switch (Set_State_mode)
  {
  case Weather_Mode:
    // Hiệu ứng dập dìu cho cả LED RGB và LED WS2812
    if (Is_changes_data_colour)
    {
      Change_colour(redValue, greenValue, blueValue, brightness);
    }
    else
    {
      // Cập nhật độ sáng
      brightness += pulseDirection * random(-brightness_range, brightness_range); // Thay đổi độ sáng chậm hơn
      if (brightness >= 255 || brightness <= minBrightness)
      {
        pulseDirection *= -1;
        brightness = constrain(brightness, minBrightness, 255); // Đảm bảo độ sáng không thấp hơn 40%
      }

      // Thay đổi màu sắc với hiệu ứng dập dìu
      redValue = baseRedValue + random(-colorRange, colorRange + 1);
      greenValue = baseGreenValue + random(-colorRange, colorRange + 1);
      blueValue = baseBlueValue + random(-colorRange, colorRange + 1);

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
      Is_changes_data_colour = true;
    }

    break;

  case Randbow_Mode:

    float r, g, b;
  
      hue += 1;
      if (hue > 360)
      {
        hue = 0;
      }
      HSVtoRGB(hue, 1.0, 1.0, r, g, b);
      // Is_changes_data_colour = true;
      Change_colour(r * 255, g * 255, b * 255, random(200, 255));
    
    break;

  default:
    break;
  }
}

/*Hàm này chuyển đổi từ không gian màu HSV sang RGB.
H là Hue (màu sắc), giá trị từ 0 đến 360.
S là Saturation (độ bão hòa), giá trị từ 0 đến 1.
V là Value (độ sáng), giá trị từ 0 đến 1.
r, g, b là các giá trị đầu ra RGB, từ 0 đến 1. */
void HSVtoRGB(float H, float S, float V, float &r, float &g, float &b)
{
  float C = V * S; // Chroma
  float X = C * (1 - abs(fmod(H / 60.0, 2) - 1));
  float m = V - C;

  if (H >= 0 && H < 60)
  {
    r = C;
    g = X;
    b = 0;
  }
  else if (H >= 60 && H < 120)
  {
    r = X;
    g = C;
    b = 0;
  }
  else if (H >= 120 && H < 180)
  {
    r = 0;
    g = C;
    b = X;
  }
  else if (H >= 180 && H < 240)
  {
    r = 0;
    g = X;
    b = C;
  }
  else if (H >= 240 && H < 300)
  {
    r = X;
    g = 0;
    b = C;
  }
  else
  {
    r = C;
    g = 0;
    b = X;
  }

  r += m;
  g += m;
  b += m;
}

/*Hàm Change_colour dùng để thay đổi màu sắc của led RGb theo dạng liner*/
void Change_colour(int redValue, int greenValue, int blueValue, int brightness)
{
  if (redValue != Control_redValue)
  {
    if (redValue > Control_redValue)
    {
      Control_redValue += 1;
    }
    else
    {
      Control_redValue -= 1;
    }
  }
  //------------------------------------------------------------------------------------
  if (blueValue != Control_blueValue)
  {
    if (blueValue > Control_blueValue)
    {
      Control_blueValue += 1;
    }
    else
    {
      Control_blueValue -= 1;
    }
  }
  //------------------------------------------------------------------------------------
  if (greenValue != Control_greenValue)
  {
    if (greenValue > Control_greenValue)
    {
      Control_greenValue += 1;
    }
    else
    {
      Control_greenValue -= 1;
    }
  }
  //------------------------------------------------------------------------------------
  if (brightness != Control_brightness)
  {
    if (brightness > Control_brightness)
    {
      Control_brightness += 2;
    }
    else
    {
      Control_brightness -= 2;
    }
  }
  //------------------------------------------------------------------------------------
  Serial.print("LED SET - R: ");
  Serial.print(baseRedValue);
  Serial.print(" G: ");
  Serial.print(baseGreenValue);
  Serial.print(" B: ");
  Serial.print(baseBlueValue);
  Serial.print(" || ");

  Serial.print("LED RGB Value Control - R: ");
  Serial.print(Control_redValue);
  Serial.print(" G: ");
  Serial.print(Control_greenValue);
  Serial.print(" B: ");
  Serial.print(Control_blueValue);
  Serial.print(" | Brightness: ");
  Serial.println(Control_brightness);

  Control_redValue = constrain(Control_redValue, 0, 255);
  Control_greenValue = constrain(Control_greenValue, 0, 255);
  Control_blueValue = constrain(Control_blueValue, 0, 255);
  Control_brightness = constrain(Control_brightness, minBrightness, 255);

  analogWrite(redPin, (Control_redValue * Control_brightness) / 255);
  analogWrite(greenPin, (Control_greenValue * Control_brightness) / 255);
  analogWrite(bluePin, (Control_blueValue * Control_brightness) / 255);

  // Cập nhật LED WS2812
  for (int i = 0; i < numPixels; i++)
  {
    strip.setPixelColor(i, strip.Color(
                               (Control_redValue * Control_brightness) / 255,
                               (Control_greenValue * Control_brightness) / 255,
                               (Control_blueValue * Control_brightness) / 255));
  }
  strip.show();

  if (redValue == Control_redValue && blueValue == Control_blueValue && greenValue == Control_greenValue && brightness == Control_brightness)
  {
    Is_changes_data_colour = false;
  }
}

void fetchWeatherData()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    String serverPath = "http://api.openweathermap.org/data/2.5/weather?lat=" + String(lat) + "&lon=" + String(lon) + "&appid=" + apiKey + "&units=metric";

    http.begin(serverPath);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0)
    {
      String payload = http.getString();
      Serial.println(payload);

      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);

      String weather = doc["weather"][0]["main"];

      Serial.print("Weather Condition: ");
      Serial.println(weather);

      // weather = "Clear";

      // Điều chỉnh màu sắc cơ bản dựa trên tình trạng thời tiết
      if (weather == "Clear")
      {
        baseRedValue = 255;
        baseGreenValue = 235;
        baseBlueValue = 0; // Vàng: Nắng
      }
      else if (weather == "Clouds")
      {
        baseRedValue = 192;
        baseGreenValue = 192;
        baseBlueValue = 192; // Xám: Có mây
      }
      else if (weather == "Rain")
      {
        baseRedValue = 0;
        baseGreenValue = 0;
        baseBlueValue = 255; // Xanh dương: Mưa
      }
      else if (weather == "Drizzle")
      {
        baseRedValue = 0;
        baseGreenValue = 191;
        baseBlueValue = 255; // Xanh nhạt: Mưa phùn
      }
      else if (weather == "Thunderstorm")
      {
        baseRedValue = 128;
        baseGreenValue = 0;
        baseBlueValue = 128; // Tím: Dông bão
      }
      else if (weather == "Snow")
      {
        baseRedValue = 255;
        baseGreenValue = 255;
        baseBlueValue = 255; // Trắng: Tuyết
      }
      else if (weather == "Mist")
      {
        baseRedValue = 173;
        baseGreenValue = 216;
        baseBlueValue = 230; // Xanh lá nhạt: Sương mù
      }
      else if (weather == "Smoke")
      {
        baseRedValue = 139;
        baseGreenValue = 69;
        baseBlueValue = 19; // Nâu xám: Khói
      }
      else if (weather == "Haze")
      {
        baseRedValue = 255;
        baseGreenValue = 165;
        baseBlueValue = 0; // Cam nhạt: Sương mù nhẹ
      }
      else if (weather == "Dust")
      {
        baseRedValue = 210;
        baseGreenValue = 180;
        baseBlueValue = 140; // Vàng nhạt: Bụi
      }
      else if (weather == "Fog")
      {
        baseRedValue = 169;
        baseGreenValue = 169;
        baseBlueValue = 169; // Xám đậm: Sương mù dày đặc
      }
      else if (weather == "Sand")
      {
        baseRedValue = 244;
        baseGreenValue = 164;
        baseBlueValue = 96; // Vàng nhạt: Cát
      }
      else if (weather == "Ash")
      {
        baseRedValue = 105;
        baseGreenValue = 105;
        baseBlueValue = 105; // Xám đen: Tro bụi núi lửa
      }
      else if (weather == "Squall")
      {
        baseRedValue = 46;
        baseGreenValue = 139;
        baseBlueValue = 87; // Xanh lục đậm: Gió giật
      }
      else if (weather == "Tornado")
      {
        baseRedValue = 255;
        baseGreenValue = 0;
        baseBlueValue = 0; // Đỏ: Lốc xoáy
      }
      else
      {
        baseRedValue = 255;
        baseGreenValue = 255;
        baseBlueValue = 255; // Mặc định: Trắng nếu thời tiết không xác định
      }
    }
    else
    {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
  weatherDataUpdated = true;
}
