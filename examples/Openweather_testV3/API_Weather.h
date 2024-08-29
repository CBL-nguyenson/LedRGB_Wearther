#ifndef API_Weather__H
#define API_Weather__H
#include "kxnTask.h"

DEFINE_TASK_STATE(API_Weather){
    khoiApp_STATE_NOTHING};

CREATE_TASK(khoiApp)

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

// Thông tin WiFi
// char *ssid = "HshopLTK";
// char *password = "HshopLTK@2311";
char *ssid = "CTY CP XAY DUNG 676";
char *password = "congtyxaydung676";
// Thông tin API
float lat = 11.948036748747358;                     // Tọa độ hiện tại
float lon = 108.43815802610034;                     // Tọa độ hiện tại
String apiKey = "1e572ba239e0349a69a29be7466a3b6c"; // API key mặc định

// Chân điều khiển LED RGB cơ bản
int redPin = 20;
int greenPin = 21;
int bluePin = 10;

int redValue = 0;
int greenValue = 0;
int blueValue = 0;
int brightness = 0;

int Control_redValue = 127;
int Control_greenValue = 127;
int Control_blueValue = 127;
int Control_brightness = 255;

// Chân điều khiển LED WS2812
int Neopixel_Pin = 4;
int numPixels = 6000; // Số lượng LED WS2812

// Tham số hiệu ứng
int brightness_range = 40; // Khoảng ±40 cho màu sắc
int colorRange = 35;       // Khoảng ±25 cho màu sắc
int minBrightness = 20;    // Độ sáng thấp nhất (30%)

unsigned long updateInterval = 60000 * 5; // Cập nhật dữ liệu mỗi 60 giây
unsigned long lastUpdateTime = 0;

unsigned long lastPulseTime = 0;
unsigned long pulseInterval = 100; // Tốc độ của hiệu ứng dập dìu chậm lại

int pulseDirection = 2; // 1 để tăng độ sáng, -1 để giảm độ sáng

int baseRedValue, baseGreenValue, baseBlueValue;
bool weatherDataUpdated = false;

bool Is_changes_data_colour = false;

void Set_pin_R(uint8_t redPin_)
{
    this->redPin = redPin_;
}
void Set_pin_G(uint8_t greenPin_)
{
    this->greenPin = greenPin_;
}
void Set_pin_B(uint8_t bluePin_)
{
    this->bluePin = bluePin_;
}
void Set_pin_Neopixel(uint8_t Neopixel_)
{
    this->Neopixel_Pin = Neopixel_;
}

void setup()
{
    Adafruit_NeoPixel strip = Adafruit_NeoPixel(numPixels, Neopixel_Pin, NEO_GRB + NEO_KHZ800);
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
                brightness = ain(brightness, minBrightness, 255); // Đảm bảo độ sáng không thấp hơn 40%
            }

            // Thay đổi màu sắc với hiệu ứng dập dìu
            redValue = baseRedValue + random(-colorRange, colorRange + 1);
            greenValue = baseGreenValue + random(-colorRange, colorRange + 1);
            blueValue = baseBlueValue + random(-colorRange, colorRange + 1);

            // Đảm bảo giá trị màu nằm trong khoảng 0 đến 255
            redValue = ain(redValue, 0, 255);
            greenValue = ain(greenValue, 0, 255);
            blueValue = ain(blueValue, 0, 255);

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
                        Control_brightness += 2;
                    }
                    else
                    {
                        Control_brightness -= 2;
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

                Control_redValue = ain(Control_redValue, 0, 255);
                Control_greenValue = ain(Control_greenValue, 0, 255);
                Control_blueValue = ain(Control_blueValue, 0, 255);
                Control_brightness = ain(Control_brightness, minBrightness, 255);

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

                if (redValue == Control_redValue && blueValue == Control_blueValue && greenValue == Control_greenValue)
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

            // weather = "Squall";

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

END

#endif