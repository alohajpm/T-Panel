#include "Arduino.h"
#include "WiFi.h"
#include <HTTPClient.h>
#include <ArduinoHA.h>
#include <TFT_eSPI.h>
#include <TouchLib.h>

// Home Assistant Configuration
const char ha_broker[] = "YOUR_HA_BROKER_IP";  // Replace with your Home Assistant broker IP
const char ha_username[] = "YOUR_HA_USERNAME"; // Replace with your Home Assistant username
const char ha_password[] = "YOUR_HA_PASSWORD"; // Replace with your Home Assistant password

WiFiClient espClient;
HaClient haClient(espClient, ha_broker, ha_username, ha_password);

// Define a device entity
const char deviceName[] = "T-Panel Controller";
const char deviceManufacturer[] = "Your Manufacturer"; // Replace with your manufacturer
const char deviceModel[] = "T-Panel Model";       // Replace with your device model
const char deviceSoftwareVersion[] = "1.0.0";

HADevice device;
// Define a light entity
HALight light("livingroom_light", "Living Room Light");



// WiFi Configuration
#define WIFI_SSID "LilyGo-AABB"
#define WIFI_PASSWORD "xinyuandianzi"
#define WIFI_CONNECT_WAIT_MAX (30 * 1000)

void wifi_test(void)
{
    String text;
    int wifi_num = 0;

    Serial.println("\nScanning wifi");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    wifi_num = WiFi.scanNetworks();
    if (wifi_num == 0)
    {
        text = "\nWiFi scan complete !\nNo wifi discovered.\n";
    }
    else
    {
        text = "\nWiFi scan complete !\n";
        text += wifi_num;
        text += " wifi discovered.\n\n";

        for (int i = 0; i < wifi_num; i++)
        {
            text += (i + 1);
            text += ": ";
            text += WiFi.SSID(i);
            text += " (";
            text += WiFi.RSSI(i);
            text += ")";
            text += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " \n" : "*\n";
            delay(10);
        }
    }

    Serial.println(text);

    text = "Connecting to ";
    Serial.print("Connecting to ");
    text += WIFI_SSID;
    text += "\n";

    Serial.print(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint32_t last_tick = millis();
    uint32_t i = 0;
    bool is_smartconfig_connect = false;

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        text += ".";
        delay(100);

        if (millis() - last_tick > WIFI_CONNECT_WAIT_MAX)
        { /* Automatically start smartconfig when connection times out */
            text += "\nConnection timed out, start smartconfig";

            is_smartconfig_connect = true;
            WiFi.mode(WIFI_AP_STA);
            Serial.println("\r\n wait for smartconfig....");
            text += "\r\n wait for smartconfig....";
            text += "\nPlease use #ff0000 EspTouch# Apps to connect to the distribution network";
            WiFi.beginSmartConfig();

            while (1)
            {
                if (WiFi.smartConfigDone())
                {
                    Serial.println("\r\nSmartConfig Success\r\n");
                    Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
                    Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
                    text += "\nSmartConfig Success";
                    text += "\nSSID:";
                    text += WiFi.SSID().c_str();
                    text += "\nPSW:";
                    text += WiFi.psk().c_str();
                    last_tick = millis();
                    break;
                }
            }
        }
    }

    if (!is_smartconfig_connect)
    {
        text += "\nThe connection was successful ! \nTakes ";
        Serial.print("\nThe connection was successful ! \nTakes ");
        text += millis() - last_tick;
        Serial.print(millis() - last_tick);
        text += " ms\n";
        Serial.println(" ms\n");
    }
}


#define NTP_SERVER1 "pool.ntp.org"
#define NTP_SERVER2 "time.nist.gov"
#define GMT_OFFSET_SEC 8 * 3600 // Time zone setting function, written as 8 * 3600 in East Eighth Zone (UTC/GMT+8:00)
#define DAY_LIGHT_OFFSET_SEC 0  // Fill in 3600 for daylight saving time, otherwise fill in 0




// GUI Functionality Placeholder
TFT_eSPI tft = TFT_eSPI(); // Create object "tft"

void initializeGUI()
{
  tft.init();
  // Add code here to initialize the GUI library and components
  Serial.println("GUI Initialized");
}

void updateGUI() {
  // Add code here to update GUI elements based on Home Assistant state
  Serial.println("GUI Updated");
}

void setup() {
  Serial.begin(115200);

  // WiFi Setup
  wifi_test();

  // GUI Setup
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  initializeGUI();

  // Setup Home Assistant Device
  device.setName(deviceName);
  device.setManufacturer(deviceManufacturer);
  device.setModel(deviceModel);
  device.setSoftwareVersion(deviceSoftwareVersion);
  light.setIcon("mdi:lightbulb");
  light.setRetain(true);
  device.addEntity(light);
  // Home Assistant Setup
  haClient.begin();
  while (!haClient.connected()) {
    Serial.print("Connecting to Home Assistant...");
    haClient.loop();
    delay(1000);
  }
  Serial.println("Connected to Home Assistant.");
  light.registerEntity(&haClient);
  light.onCommand(onLightCommand);

}

void loop()
{
  haClient.loop();
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC);
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
      Serial.println("Failed to obtain time");
  }
  else {
      char timeStringBuff[50];
      strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
      tft.drawString(timeStringBuff, tft.width() / 2, 20, 2);  }
  tft.drawString("Home Assistant Controls", tft.width() / 2, 60, 3);

  // Draw light bulb rectangle
  int rectX = tft.width() / 2 - 25;
  int rectY = 100;
  int rectWidth = 50;
  int rectHeight = 80;
  uint16_t rectColor = light.getState() ? TFT_YELLOW : TFT_GRAY;
  tft.drawRect(rectX, rectY, rectWidth, rectHeight, rectColor);

  // Check for touch
  uint16_t touchX, touchY;
  if (tft.getTouch(&touchX, &touchY))
  {
    if (touchX >= rectX && touchX <= rectX + rectWidth &&
        touchY >= rectY && touchY <= rectY + rectHeight)
    {
      light.setState(!light.getState());
      light.setValue(light.getState());
    }

  }
  // Draw "Add Device" button
  int buttonX = tft.width() / 2 - 50;
  int buttonY = tft.height() - 60;
  int buttonWidth = 100;
  int buttonHeight = 40;
  tft.drawRect(buttonX, buttonY, buttonWidth, buttonHeight, TFT_BLUE);
  tft.drawString("Add Device", tft.width() / 2, buttonY + buttonHeight / 2, 2);

  // Check for touch on "Add Device" button
  if (tft.getTouch(&touchX, &touchY) &&
      touchX >= buttonX && touchX <= buttonX + buttonWidth &&
      touchY >= buttonY && touchY <= buttonY + buttonHeight) {
    Serial.println("Add Device button pressed");
  }

  delay(100); // Adjust delay as needed
}

void onLightCommand(bool state, uint8_t brightness) {
  Serial.printf("Light command: state=%d, brightness=%d\n", state, brightness);
  // Here you would add code to control the actual light hardware if you had one.
}