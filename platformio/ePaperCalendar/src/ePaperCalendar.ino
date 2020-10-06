#include <GxEPD2_BW.h>
#include <Fonts/Org_01.h>
#include <Fonts/FreeMonoBold9pt7b.h>


#include <WiFi.h>
#include <HTTPClient.h> // used to get the calendar events
#include "time.h"// used for retrieving time from an NTP server

#include <EEPROM.h>


// base class GxEPD2_GFX can be used to pass references or pointers to the display instance as parameter, uses ~1.2k more code
// enable or disable GxEPD2_GFX base class
#define ENABLE_GxEPD2_GFX 0

GxEPD2_BW<GxEPD2_213_B73, GxEPD2_213_B73::HEIGHT> ePaperDisplay{
  GxEPD2_213_B73{/*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4}
}; // GDEH0213B73

const int MIN_HEIGHT{6};
const int BATT_PIN{35};
const unsigned long LONG_DEEP_SLEEP_MINS{60};
const unsigned long SHORT_DEEP_SLEEP_MINS{15};
// waitinf for WiFi and then HTTP can take a while or never end, limit the overall wait to avoid draining the battery
const unsigned long MAX_WAIT_MILLIS{60000};

// define the number of bytes we want to access
const size_t EEPROM_SIZE{1};
const int EEPROM_ADDRESS{0};
const uint8_t SCREEN_ROTATED{123};

const char* SSID{"***"};
const char* PASSWORD{"***"};

/* TIME KEEPING */
const char* NTP_SERVER{"ch.pool.ntp.org"};
const long  GMT_OFFSET_SECS{3600};
const int   DAYLIGHT_OFFSET_SECS{0};

/* Screen Orientation*/
const int BUTTON_PIN{39};


HTTPClient http;
const char* CALENDAR_URL{"https://script.google.com/macros/s/***"};
const char* ROOT_CA{\
"-----BEGIN CERTIFICATE-----\n" \
"MIIESjCCAzKgAwIBAgINAeO0mqGNiqmBJWlQuDANBgkqhkiG9w0BAQsFADBMMSAw\n" \
"HgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMjETMBEGA1UEChMKR2xvYmFs\n" \
"U2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjAeFw0xNzA2MTUwMDAwNDJaFw0yMTEy\n" \
"MTUwMDAwNDJaMEIxCzAJBgNVBAYTAlVTMR4wHAYDVQQKExVHb29nbGUgVHJ1c3Qg\n" \
"U2VydmljZXMxEzARBgNVBAMTCkdUUyBDQSAxTzEwggEiMA0GCSqGSIb3DQEBAQUA\n" \
"A4IBDwAwggEKAoIBAQDQGM9F1IvN05zkQO9+tN1pIRvJzzyOTHW5DzEZhD2ePCnv\n" \
"UA0Qk28FgICfKqC9EksC4T2fWBYk/jCfC3R3VZMdS/dN4ZKCEPZRrAzDsiKUDzRr\n" \
"mBBJ5wudgzndIMYcLe/RGGFl5yODIKgjEv/SJH/UL+dEaltN11BmsK+eQmMF++Ac\n" \
"xGNhr59qM/9il71I2dN8FGfcddwuaej4bXhp0LcQBbjxMcI7JP0aM3T4I+DsaxmK\n" \
"FsbjzaTNC9uzpFlgOIg7rR25xoynUxv8vNmkq7zdPGHXkxWY7oG9j+JkRyBABk7X\n" \
"rJfoucBZEqFJJSPk7XA0LKW0Y3z5oz2D0c1tJKwHAgMBAAGjggEzMIIBLzAOBgNV\n" \
"HQ8BAf8EBAMCAYYwHQYDVR0lBBYwFAYIKwYBBQUHAwEGCCsGAQUFBwMCMBIGA1Ud\n" \
"EwEB/wQIMAYBAf8CAQAwHQYDVR0OBBYEFJjR+G4Q68+b7GCfGJAboOt9Cf0rMB8G\n" \
"A1UdIwQYMBaAFJviB1dnHB7AagbeWbSaLd/cGYYuMDUGCCsGAQUFBwEBBCkwJzAl\n" \
"BggrBgEFBQcwAYYZaHR0cDovL29jc3AucGtpLmdvb2cvZ3NyMjAyBgNVHR8EKzAp\n" \
"MCegJaAjhiFodHRwOi8vY3JsLnBraS5nb29nL2dzcjIvZ3NyMi5jcmwwPwYDVR0g\n" \
"BDgwNjA0BgZngQwBAgIwKjAoBggrBgEFBQcCARYcaHR0cHM6Ly9wa2kuZ29vZy9y\n" \
"ZXBvc2l0b3J5LzANBgkqhkiG9w0BAQsFAAOCAQEAGoA+Nnn78y6pRjd9XlQWNa7H\n" \
"TgiZ/r3RNGkmUmYHPQq6Scti9PEajvwRT2iWTHQr02fesqOqBY2ETUwgZQ+lltoN\n" \
"FvhsO9tvBCOIazpswWC9aJ9xju4tWDQH8NVU6YZZ/XteDSGU9YzJqPjY8q3MDxrz\n" \
"mqepBCf5o8mw/wJ4a2G6xzUr6Fb6T8McDO22PLRL6u3M4Tzs3A2M1j6bykJYi8wW\n" \
"IRdAvKLWZu/axBVbzYmqmwkm5zLSDW5nIAJbELCQCZwMH56t2Dvqofxs6BBcCFIZ\n" \
"USpxu6x6td0V7SvJCCosirSmIatj/9dSSVDQibet8q/7UK4v4ZUN80atnZz1yg==\n" \
"-----END CERTIFICATE-----\n"};





void setup() {
  // 1. Serial initialisation
  Serial.begin(115200);
  Serial.println("Starting...");


  // 2. Screen rotation in Flash memory
  // check if the Button is pressed on startup, in which case turn the display by 180degs
  bool rotated{false};
  // saved status
  EEPROM.begin(EEPROM_SIZE);
  uint8_t eepromData{EEPROM.read(EEPROM_ADDRESS)};
  Serial.printf("Read from Flash: %d\n", eepromData);


  pinMode(BUTTON_PIN, INPUT_PULLUP);
  // pin pulled high -> when pressed we get a 0
  bool isButtonPressed{ !digitalRead(BUTTON_PIN) };
  // if button is pressed -> TOGGLE the rotation & SAVE
  if(isButtonPressed) {
    // 0 means there was a problem
    if(eepromData) {
      // TOGGLE
      if(eepromData == SCREEN_ROTATED) {
        eepromData = 100;
      } else {
        eepromData = SCREEN_ROTATED;
      }
      
      // try to SAVE
      EEPROM.write(EEPROM_ADDRESS, eepromData);
      if(EEPROM.commit()) {
        Serial.printf("Updated eepromData: %d\n", eepromData);
      } else {
        Serial.println(" ERROR, can't write data to FLASH memory");
      }
    } else {
      Serial.println(" ERROR, can't read data from FLASH memory !");
    }
  }
  rotated = eepromData == SCREEN_ROTATED;
  Serial.printf("Using rotated: %d\n", rotated);


  // 2. Check if the known network is present
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  int netCount{WiFi.scanNetworks()};
  bool foundKnownSSID{false};
  Serial.println("WiFi networks:");
  for (int i = 0; i < netCount; i++) {
    Serial.println(WiFi.SSID(i));
    
    if(WiFi.SSID(i) == SSID) foundKnownSSID = true;
  }
  Serial.printf("SSID %s found: %d\n\n", SSID, foundKnownSSID);

  // we can't do much if we have no WiFi, so go quickly to sleep to save energy
  bool updateComplete{false};
  if(foundKnownSSID) {
    // 3. init the ePaper display
    initDisplay(rotated);

    // 4. BATTERY monitoring
    ePaperDisplay.setCursor(10, 10);
    ePaperDisplay.print(String(battVoltage(), 1) + " V");
    ePaperDisplay.display(true); // partial update

    const unsigned long startTime{millis()};

    // 5. WiFi
    Serial.println("Starting WiFi...");
    int i{100};
    while ((WiFi.status() != WL_CONNECTED) && (millis() - startTime < MAX_WAIT_MILLIS)) {
      Serial.printf("WiFi conn status: %d\n", WiFi.status());
      if(i > 20) {
        WiFi.disconnect(true);
        WiFi.begin(SSID, PASSWORD);
        i = 0;
      }
      
      blinkDot();
      
      i++;
    }
    Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());


    // 6. TIME
    ePaperDisplay.setCursor(60, 10);
    configTime(GMT_OFFSET_SECS, DAYLIGHT_OFFSET_SECS, NTP_SERVER);
    tm timeInfo{};
    if(getLocalTime(&timeInfo)) {
      ePaperDisplay.print(&timeInfo, "%a %d %b %H:%M");
    } else {
      ePaperDisplay.print("NO Time from NTP");
    }
    ePaperDisplay.display(true); // partial update


    // 7. CALENDAR events
    int httpCode = -111;
    while((httpCode <= 0) && (millis() - startTime < MAX_WAIT_MILLIS)) {
      http.setFollowRedirects(followRedirects_t::HTTPC_STRICT_FOLLOW_REDIRECTS); //necessary as Google Script Webapp URL will redirect
      http.begin(CALENDAR_URL, ROOT_CA);
      httpCode = http.GET();
      if(httpCode > 0) {
        String payload{http.getString()};
        Serial.printf("HTTP response code: %d\n", httpCode);

        displayEvents(payload);

        updateComplete = true;
      } else {
        Serial.printf("Error on HTTP request, code: %d\n", httpCode);
        ePaperDisplay.printf(" HTTP err: %d", httpCode);
        ePaperDisplay.display(true); // partial update
      }

      http.end();
    }
  }

  unsigned long deepSleepMins {SHORT_DEEP_SLEEP_MINS};
  if(updateComplete) {
    // only sleep properly if everything went fine...
    deepSleepMins = LONG_DEEP_SLEEP_MINS;

    // 8. one final full update of the display and then turn it off
    ePaperDisplay.display(false); // FULL update
    ePaperDisplay.powerOff();
  }

  // 9. SLEEP
  Serial.printf("Going to a well deserved SLEEP for %lu mins!\n\n\n", deepSleepMins);
  ESP.deepSleep(deepSleepMins * 60 * 1000 * 1000);  // this is in Micros
}



void loop() {
  // NOTHING given that it goes to SLEEP at the end of setup() and then when it resets it starts from scratch
}


void initDisplay(bool rotated) {
  delay(100);  // it interferes with Serial. so give time to the latter to finish
  // use 115200 for 1st param or similar if we want Serial diagnostic
  // 2nd parametre avoids full re-init when waking from deep sleep
  ePaperDisplay.init(0, false);
  if(rotated) {
    ePaperDisplay.setRotation(3); // rotated = rotate 90degs CW
  } else {
    ePaperDisplay.setRotation(1); // normal = rotate 90degs CCW
  }
  ePaperDisplay.setTextColor(GxEPD_BLACK);
  ePaperDisplay.setFullWindow();
  ePaperDisplay.fillScreen(GxEPD_WHITE);
  ePaperDisplay.setFont(&Org_01);
  delay(10);
}

void blinkDot() {
    // below 6 lines all they do is BLINK a dot... :)
    ePaperDisplay.fillRect(ePaperDisplay.width()-6, MIN_HEIGHT, 5, 5, GxEPD_WHITE);
    ePaperDisplay.display(true); // partial update
    delay(200);
    ePaperDisplay.fillRect(ePaperDisplay.width()-6, MIN_HEIGHT, 5, 5, GxEPD_BLACK);
    ePaperDisplay.display(true); // partial update
    delay(200);
}



void displayEvents(String serverPayload) {
  char buff[serverPayload.length()];
  serverPayload.toCharArray(buff, serverPayload.length());
  
  ePaperDisplay.setFont(&FreeMonoBold9pt7b);

  char *line;
  line = strtok(buff, "\n");
  for (int i=0; line != NULL; i++) {
    ePaperDisplay.setCursor(0, 27 + 16*i);
    // limit the event/line characters so that it doesn't go on next line
    ePaperDisplay.printf("%.22s", line);

    line = strtok(NULL, "\n");
  }
}



/**
 * The value measured is between 0 and 4095. 
 * However it's not perfectly linear. 
 * There's a voltage divider with 2 100KOhms resistors, and the PIN is connected between them -> the actual Battery voltage is 2x what we measure
 * Tried to use battVoltage = (rawValue / 4095 * 3.3) * 2 but got inconsistent values that didn't match measurement.
 * Therefore I sampled readings for voltages between 3V and 4.5V and did a linear regression -> came up with the right "a" and "b"
 */
float battVoltage() {
  int rawValue {analogRead(BATT_PIN)};

  return 0.40931 + 0.00159 * rawValue;
}
