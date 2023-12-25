#include <Arduino.h>
#include <EEPROM.h>
#include "Configuration.h"

configuration_t configuration;

uint8_t EEPROM_initAndCheckFactoryReset() {
  Log.noticeln("Configuration size: %i", sizeof(configuration_t));
  
  EEPROM.begin(sizeof(configuration_t) + EEPROM_FACTORY_RESET + 1);
  uint8_t resetCounter = EEPROM.read(EEPROM_FACTORY_RESET);

  Log.noticeln("Factory reset counter: %i", resetCounter);
  Log.noticeln("EEPROM length: %i", EEPROM.length());

  // Bump reset counter
  EEPROM.write(EEPROM_FACTORY_RESET, resetCounter + 1);
  EEPROM.commit();

  return resetCounter;
}

void EEPROM_clearFactoryReset() {
  EEPROM.write(EEPROM_FACTORY_RESET, 0);
  EEPROM.commit();
}

void EEPROM_saveConfig() {
  Log.infoln("Saving configuration to EEPROM");
  EEPROM.put(EEPROM_CONFIGURATION_START, configuration);
  EEPROM.commit();
}

void EEPROM_loadConfig() {

  configuration = {};
  EEPROM.get(EEPROM_CONFIGURATION_START, configuration);

  Log.noticeln("Configuration loaded: %s", configuration._loaded);

  if (strcmp(configuration._loaded, "jaisor")) {
    // blank
    Log.infoln("Blank configuration, loading defaults");
    strcpy(configuration._loaded, "jaisor");
    strcpy(configuration.name, DEVICE_NAME);
    #ifdef LED
      configuration.ledMode = 0;
      configuration.ledCycleModeMs = LED_CHANGE_MODE_SEC * 1000;
      configuration.ledDelayMs = 10;
      configuration.ledBrightness = LED_BRIGHTNESS;
      configuration.ledStripSize = LED_STRIP_SIZE;
      configuration.psLedBrightness = 1.0f;
      configuration.psStartHour = 0;
      configuration.psEndHour = 0;
    #endif
     #ifdef WIFI
      strcpy(configuration.ntpServer, NTP_SERVER);
      configuration.gmtOffset_sec = NTP_GMT_OFFSET_SEC;
      configuration.daylightOffset_sec = NTP_DAYLIGHT_OFFSET_SEC;
    #endif
  }

#ifdef LED
  if (isnan(configuration.ledBrightness)) {
    Log.verboseln("NaN brightness");
    configuration.ledBrightness = LED_BRIGHTNESS;
  }
  if (isnan(configuration.ledMode)) {
    Log.verboseln("NaN ledMode");
    configuration.ledMode = 0;
  }
  if (isnan(configuration.ledCycleModeMs)) {
    Log.verboseln("NaN ledCycleModeMs");
    configuration.ledCycleModeMs = 0;
  }
  if (isnan(configuration.ledDelayMs)) {
    Log.verboseln("NaN ledDelayMs");
    configuration.ledDelayMs = 10;
  }
  if (isnan(configuration.ledStripSize)) {
    Log.verboseln("NaN ledStripSize");
    configuration.ledStripSize = LED_STRIP_SIZE;
  }
  if (isnan(configuration.psLedBrightness)) {
    Log.verboseln("NaN power-save brightness");
    configuration.psLedBrightness = 1.0;
  }
  if (isnan(configuration.psStartHour)) {
    Log.verboseln("NaN power-save start hour");
    configuration.psStartHour = 0;
  }
  if (isnan(configuration.psEndHour)) {
    Log.verboseln("NaN power-save end hour");
    configuration.psEndHour = 0;
  }
#endif

#ifdef WIFI
  String wifiStr = String(configuration.wifiSsid);
  for (auto i : wifiStr) {
    if (!isAscii(i)) {
      Log.verboseln("Bad SSID, loading default: %s", wifiStr.c_str());
      strcpy(configuration.wifiSsid, "");
      break;
    }
  }
#endif

  Log.noticeln("Device name: %s", configuration.name);
}

void EEPROM_wipe() {
  Log.warningln("Wiping configuration!");
  for (uint16_t i = 0; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
}

#ifdef LED
static float currentLedBrightness = 0;
static unsigned long tsLedBrightnessUpdate = 0;

bool isInsideInterval(int i, int8_t s, int8_t e) {
  if (s <= e) {
    return i>=s && i>e;
  } else {
    return ((i>=s && i<24) || (i>=0 && i<e));
  }
}

float CONFIG_getLedBrightness(bool force) {
  currentLedBrightness = configuration.ledBrightness;
  #ifdef WIFI
  // Check on power save mode about once per minute
  if (configuration.psLedBrightness < 1.0f && (configuration.psStartHour || configuration.psEndHour) && (force || millis() - tsLedBrightnessUpdate > 60000)) {
    tsLedBrightnessUpdate = millis();
    struct tm timeinfo;
    bool timeUpdated = getLocalTime(&timeinfo);
    if (timeUpdated && isInsideInterval(timeinfo.tm_hour, configuration.psStartHour, configuration.psEndHour)) {
        currentLedBrightness = currentLedBrightness * configuration.psLedBrightness;
        if (currentLedBrightness != configuration.ledBrightness) {
          Log.infoln("Current LED brightness is '%D' compared to default '%D'", currentLedBrightness, configuration.ledBrightness);
        }
    }
  }
  #endif
  return currentLedBrightness;
}
#endif