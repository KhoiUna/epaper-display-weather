#include "DEV_Config.h"
#include "utility/EPD_12in48b.h"
#include "EPD.h"
#include "GUI_Paint.h"
#include <stdlib.h>
#include <WiFi.h>
#include <Arduino_JSON.h>
#include <HTTPClient.h>
#include <DateTime.h>
#include <string.h>
#include "conf.h"

const char* ssid = "Resnet";
const char* password = "unalions";

//Your Domain name with URL path or IP address with path
String serverName = "https://api.openweathermap.org/data/2.5/weather?appid=" + apiKey + "&zip=35632&units=metric";

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
unsigned long timerDelay = 15 * 60 * 1000;

// Declare Image cache for ePaper
UBYTE *Image = NULL;
UWORD xsize = 1304, ysize = 984 / 2; //1304 x 492// Not enough memory to allocate 1304*984 of memory
UDOUBLE Imagesize = ((xsize % 8 == 0)? (xsize / 8 ): (xsize / 8 + 1)) * ysize;

void setupDateTime() {
  DateTime.setTimeZone("CST6CDT");
  DateTime.begin();
  if (!DateTime.isTimeValid()) {
    Serial.println("Failed to get time from server.");
  }
}

void connectWifi() {
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println("-----------------------------------------");
}

void setup() {
  Serial.begin(115200); 
  connectWifi();
  
  // SPI initialization
  DEV_ModuleInit();
  DEV_TestLED();
  EPD_12in48B_Init();
  EPD_12in48B_Clear();

  // Apply for an Image cache
  if((Image = (UBYTE *)malloc(Imagesize)) == NULL) {
      printf("Failed to apply for black memory...\r\n");
      while(1);
  }

  setupDateTime();
}

void loop() {
  // Update the ePaper every 15 minutes based on timerDelay
  if ((millis() - lastTime) >= timerDelay or lastTime == 0) {
    connectWifi();
    
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;
      http.begin(serverName);
      
      // Send HTTP GET request
      int httpResponseCode = http.GET();
      
      if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();

        // Parse JSON data received from API
        JSONVar myObject = JSON.parse(payload);
        if (JSON.typeof(myObject) == "undefined") {
          Serial.println("Parsing input failed!");
        }

        // Start drawing on Image
        #if 1
          // Select Image
          printf("Paint_NewImage\r\n");
          Paint_NewImage(Image, 1304, 492, 0, WHITE);
          Paint_Clear(WHITE);
          
          printf("Drawing data on Image...\r\n\n");

          int startY = 100;
          int marginY = 50;

          if (!DateTime.isTimeValid()) {
            Paint_DrawString_EN(300, startY, "Weather Report | Cannot get time from server", &Font24, BLACK, WHITE);
          } else {
            String d = "Weather Report | " + DateTime.toString();
            char date[d.length() + 1];
            strcpy(date, d.c_str());
            Paint_DrawString_EN(300, startY, date, &Font24, BLACK, WHITE);
          }
          
          Paint_DrawString_EN(300, startY + marginY, "Location:", &Font24, WHITE, BLACK);
          Paint_DrawString_EN(540, startY + marginY, myObject["name"], &Font24, WHITE, BLACK);
          Paint_DrawString_EN(740, startY + marginY, myObject["sys"]["country"], &Font24, WHITE, BLACK);

          Paint_DrawString_EN(300, startY + marginY * 2, "Weather: ", &Font24, WHITE, BLACK);
          Paint_DrawString_EN(540, startY + marginY * 2, myObject["weather"][0]["description"], &Font24, WHITE, BLACK);

          Paint_DrawString_EN(300, startY + marginY * 3, "Temperature:", &Font24, WHITE, BLACK);
          Paint_DrawNum(540, startY + marginY * 3, myObject["main"]["temp"], &Font24, WHITE, BLACK);
          Paint_DrawString_EN(580, startY + marginY * 3, "C", &Font24, WHITE, BLACK);
          
          Paint_DrawString_EN(300, startY + marginY * 4, "Feels like:", &Font24, WHITE, BLACK);
          Paint_DrawNum(540, startY + marginY * 4, myObject["main"]["feels_like"], &Font24, WHITE, BLACK);
          Paint_DrawString_EN(580, startY + marginY * 4, "C", &Font24, WHITE, BLACK);
        #endif

        // Start displaying Image on ePaper
        printf("Display Image on EPD_Display\r\n\n");
        EPD_12in48B_SendBlack1(Image);
        //EPD_12in48B_SendBlack2(Image);
        EPD_12in48B_TurnOnDisplay();
        
        // Set ePaper to sleep
        free(Image);
        Image = NULL;
        printf("Go to sleep...\r\n");
        EPD_12in48B_Sleep();
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }    
    // Update lastTime
    lastTime = millis();
  }
}
