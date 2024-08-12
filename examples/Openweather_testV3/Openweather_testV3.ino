#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// Thông tin WiFi
// const char *ssid = "HshopLTK";
// const char *password = "HshopLTK@2311";
const char *ssid = "SEAN_LAPTOP_4896";
const char *password = "123456789";

// Thông tin API
float lat = 11.948036748747358;                     // Tọa độ hiện tại
float lon = 108.43815802610034;                     // Tọa độ hiện tại
String apiKey = "1e572ba239e0349a69a29be7466a3b6c"; // API key mặc định

// Chân điều khiển LED RGB cơ bản
const int redPin = 20;
const int greenPin = 21;
const int bluePin = 10;

int redValue = 0;
int greenValue = 0;
int blueValue = 0;
int brightness = 0;

// int Check_redValue = 0;
// int Check_greenValue = 0;
// int Check_blueValue = 0;
// int Check_brightness = 255;

int Control_redValue = 255;
int Control_greenValue = 255;
int Control_blueValue = 255;
int Control_brightness = 0;

// Chân điều khiển LED WS2812
const int ledPin = 4;
const int numPixels = 2000; // Số lượng LED WS2812

// Tham số hiệu ứng
const int brightness_range = 40;    // Khoảng ±40 cho màu sắc
const int colorRange = 25;    // Khoảng ±25 cho màu sắc
const int minBrightness = 30; // Độ sáng thấp nhất (30%)

const unsigned long updateInterval = 60000*5; // Cập nhật dữ liệu mỗi 60 giây
unsigned long lastUpdateTime = 0;

unsigned long lastPulseTime = 0;
unsigned long pulseInterval = 100; // Tốc độ của hiệu ứng dập dìu chậm lại

int pulseDirection = 1; // 1 để tăng độ sáng, -1 để giảm độ sáng

int baseRedValue, baseGreenValue, baseBlueValue;
bool weatherDataUpdated = false;

bool Is_changes_data_colour = false;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(numPixels, ledPin, NEO_GRB + NEO_KHZ800);

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
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname("myesp32_weather");

  // No authentication by default
  ArduinoOTA.setPassword("esp32");

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

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  //-------------------------------------------------------------------------------------------------------
  Serial.println("Connected to WiFi");
  lastUpdateTime = millis();

  fetchWeatherData();
  weatherDataUpdated = true;
}

void loop()
{
  ArduinoOTA.handle();
  unsigned long currentMillis = millis();

  // Cập nhật dữ liệu thời tiết mỗi 60 giây
  if (currentMillis - lastUpdateTime >= updateInterval)
  {
    lastUpdateTime = currentMillis;
    fetchWeatherData(); // Nhận data
    weatherDataUpdated = true;
  }

  // Hiệu ứng dập dìu cho cả LED RGB và LED WS2812
  if (weatherDataUpdated)
  {
    if (currentMillis - lastPulseTime >= pulseInterval)
    {
      pulseInterval = random(10, 1000);
      lastPulseTime = currentMillis;

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


      while (Is_changes_data_colour)
      {
        // Cập nhật LED RGB cơ bản
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
            Control_brightness += 1;
          }
          else
          {
            Control_brightness -= 1;
          }
        }
        //------------------------------------------------------------------------------------

      Serial.print("LED RGB Value Control - R: ");
      Serial.print(Control_redValue);
      Serial.print(" G: ");
      Serial.print(Control_greenValue);
      Serial.print(" B: ");
      Serial.print(Control_blueValue);
      Serial.print(" | Brightness: ");
      Serial.println(Control_brightness);
      

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
    }
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

      // weather = "Rain";

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
}
