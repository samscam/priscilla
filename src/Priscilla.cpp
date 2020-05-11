#include "Priscilla.h"
#include "tests.h"
#include "settings.h"
#include "Messages.h"
#include "Displays/Display.h"
#include "Location/LocationSource.h"
#include "Location/LocationManager.h"
#include <WiFi.h>

#include "Provisioning/Provisioning.h"
#include "FirmwareUpdates/FirmwareUpdates.h"
#include <Preferences.h>
#include "Weather/Rainbows.h"

#include "Loggery.h"
#include "TimeThings/NTP.h"

#include "rom/uart.h"

// CONFIGURATION  --------------------------------------



// Time zone adjust (in MINUTES from utc)
int32_t tzAdjust = 0;
int32_t secondaryTimeZone = 330; // Mumbai is +5:30

// ----------- Display

// #if defined(RAINBOWDISPLAY)

// #include "Displays/RGBDigit.h"
// RGBDigit *display = new RGBDigit();

// #elif defined(MATRIX)

#include "Displays/Matrix.h"
Matrix *display = new Matrix();

// #elif defined(EPAPER)

// #include "Displays/Epaper.h"
// Display *display = new EpaperDisplay();

// #endif


//Adafruit7 display = Adafruit7();

// #include "Displays/DebugDisplay.h"
// Display *display = new DebugDisplay();

// ----------- RTC

#if defined(TIME_GPS)
#include "TimeThings/GPSTime.h"
RTC_GPS rtc = RTC_GPS();

#elif defined(TIME_DS3231)
RTC_DS3231 rtc = RTC_DS3231();

#elif defined(TIME_ESP32)
#include "TimeThings/ESP32Rtc.h"
RTC_ESP32 rtc = RTC_ESP32();

#endif

#if defined(LOCATION_GPS)
LocationSource locationSource = rtc;
#else
LocationManager *locationManager;
#endif


// ---------- Networking

WiFiClientSecure secureClient; // << https on esp32
// WiFiClient client; // <<  plain http, and https on atmelwinc


// ---------- WEATHER CLIENT
// #include "weather/met-office.h"
// WeatherClient *weatherClient = new MetOffice(client); // << It's plain HTTP

#include "weather/darksky.h"
DarkSky *weatherClient = new DarkSky(secureClient);


// RAINBOWS

Rainbows rainbows;


// SETUP  --------------------------------------
void setup() {
  delay(2000);
  
  Serial.begin(115200);
  LOGMEM;
  // Say hello on Serial port
  Serial.println("");
  Serial.println("   ------------    ");
  Serial.println("  /            \\   ");
  Serial.println("  --------------   ");
  Serial.println(" |              |   ");
  Serial.println(" | ! CLOCKLET ! |   ");
  Serial.println(" |              |   ");
  Serial.println("  --------------   \n");

  Serial.printf("Firmware Version: %s\n",VERSION);

  // Read things from preferences...
  Preferences preferences = Preferences();
  preferences.begin("clocklet", false);

  uint32_t serial = preferences.getUInt("serial");
  Serial.printf("Serial number: %d\n",serial);

  uint32_t hwrev = preferences.getUInt("hwrev");
  Serial.printf("Hardware revision: %d\n",hwrev);

  // Quickie migration thing - will improve this!!
  String swmigrev = preferences.getString("swmigrev","0.0.0");

  if (swmigrev != VERSION){
    Serial.printf("Previously running: %s\n",swmigrev.c_str());
    preferences.putString("swmigrev",VERSION);
    Serial.println("MIGRATED!");
  }

  String owner = preferences.getString("owner");
  Serial.printf("Owner: %s\n",owner.c_str());

  preferences.end();

  Serial.println("");
  LOGMEM;

  analogReadResolution(12);

  // Randomise the random seed - Not sure if this is random enough
  // We don't actually need to do this if the wireless subsystems are active
  uint16_t seed = analogRead(A0);
  randomSeed(seed);
  Serial.println((String)"Seed: " + seed);

  // seed the brightness
  for (int i = 0; i<10 ; i++){
    currentBrightness();
    delay(100);
  }

  Serial.println("Clock starting!");
  Wire.begin();

  display->setup();
  display->setBrightness(currentBrightness());

  LOGMEM;

  // DISPLAY A GREETING
  display->setStatusMessage("part");
  delay(1000);
  display->setStatusMessage("time");
  delay(1000);
  String greeting = String("Hello "+owner);
  display->displayMessage(greeting.c_str(), rando);

  // Uncomment to run various display tests:
  displayTests(display);  
  
  LOGMEM;

  WiFi.begin();

  LOGMEM;
  // Checks for provisioning
  bool isProvisioned = isAlreadyProvisioned();

  if (isProvisioned){
    if (waitForWifi(6000)){
      display->displayMessage("Everything is awesome", good);
    } else {
      display->displayMessage("Network is pants", bad);
      display->setDeviceState(noNetwork);
    }
  } else {
    display->displayMessage("I need your wifi", bad);
    display->setDeviceState(bluetooth);
    startProvisioning();
  }
  // startProvisioning();
  
  LOGMEM;
  locationManager = new LocationManager();
  if (!locationManager->hasSavedLocation()){
    display->displayMessage("Where am I", bad);
    display->setDeviceState(noLocation);
  } else {
    rainbows.setLocation(locationManager->getLocation());
  }

  rtc.begin();

}

// LOOP  --------------------------------------


unsigned long lastRandomMessageTime = millis();
unsigned long nextMessageDelay = 1000 * 60 * 2;

unsigned long lastHourlyUpdate = 0;
unsigned long lastDailyUpdate = 0;

DateTime lastTime = 0;

enum Precision {
  minutes, seconds, subseconds
};

Precision precision = subseconds;
// #if defined(RAINBOWDISPLAY)
// Precision precision = subseconds;
// #else
// Precision precision = minutes;
// #endif

bool didDisplay = false;


void loop() {

  display->setBrightness(currentBrightness());

  // Check for touches...
  if (detectTouchPeriod() > 500){
    display->displayMessage("That tickles",rando);
  }
  if (detectTouchPeriod() > 5000){
    startProvisioning();
    display->setDeviceState(bluetooth);
    display->displayMessage("Bluetooth is on",good);
  }
  if (detectTouchPeriod() > 10000){
    display->displayMessage("Keep holding for restart",bad);
  }
  if (detectTouchPeriod() > 15000){
    ESP.restart();
  }

  #if defined(BATTERY_MONITORING)
      float voltage = batteryVoltage();
      display->setBatteryLevel(batteryLevel(voltage));

      if (voltage < cutoffVoltage){
        
        espShutdown();
      }

  #endif

  #if defined(TIME_GPS)
    rtc.loop(); //<< needed on the GPS rtc to wake it :/
  #endif

  // This should always be UTC
  DateTime time = rtc.now();

  bool needsDaily = false;

  // Check for major variations in the time > 1 minute
  // This often happens after a (delayed) NTP sync or when GPS gets a fix
  // TimeSpan timeDiff = time - lastTime;
  // if (timeDiff.totalseconds() > 60 || timeDiff.totalseconds() < -60 ) {
  //   needsDaily = true;
  // }

  if ( time.unixtime() > lastDailyUpdate + (60 * 60 * 24)) {
    needsDaily = true;
  }

  //Daily update
  if (needsDaily){
    updatesDaily();
    time = rtc.now();
    lastDailyUpdate = time.unixtime();
  }

  // Hourly updates
  if ( time.unixtime() > lastHourlyUpdate + (60 * 60)){
    updatesHourly();
    time = rtc.now();
    lastHourlyUpdate = time.unixtime();
  }

  if (millis() > lastRandomMessageTime + nextMessageDelay){
    Serial.println("Random message");
    const char* message = randoMessage();
    display->displayMessage(message, rando);
    display->displayMessage(message, rando);
    display->displayMessage(message, rando);
    lastRandomMessageTime = millis();
    nextMessageDelay = 1000 * 60 * random(5,59);
  }

  time = rtc.now();
  // Minutes precision updates
  // Will fail when starting at zero :/
  switch (precision) {
    case minutes:
      if (time.minute() != lastTime.minute()){
        displayTime(time);
        lastTime = time;
        didDisplay = true;
        display->frameLoop();
      }
      break;
    case seconds:
      if (time.second() != lastTime.second()){
        displayTime(time);
        lastTime = time;
        didDisplay = true;
        display->frameLoop();
      }
      break;
    case subseconds:
      display->setRainbows(rainbows.rainbowProbability(time));
      displayTime(time);
      didDisplay = true;
      display->frameLoop();
      break;
  }




  time = rtc.now();

  if (didDisplay){
    switch (precision) {
        case minutes:
        sensibleDelay( (59 - time.second() ) * 1000 );
        break;
      case seconds:
        sensibleDelay(900); // This should really be the time to the next second boundary - latency
        break;
      case subseconds:
        sensibleDelay(1000/FPS);
        break;
    }
    didDisplay = false;
  // } else {
  //   delay(10);
  }

}

void displayTime(DateTime utcTime){
    DateTime displayTime;
    // adjust for timezone and DST
    displayTime = utcTime + TimeSpan(dstAdjust(utcTime) * 3600);
    displayTime = displayTime + TimeSpan(tzAdjust * 60);
    
    display->setTime(displayTime);

    // secondary time
    displayTime = utcTime; //+ TimeSpan(dstAdjust(time) * 3600); -- no dst in india
    displayTime = displayTime + TimeSpan(secondaryTimeZone * 60);
    display->setSecondaryTime(displayTime,"Mumbai");
}

void sensibleDelay(int milliseconds){
  // #ifdef RAINBOWDISPLAY
    FastLED.delay(milliseconds);
  // #else
  //   Serial.print("Sleeping for: ");
  //   Serial.println(milliseconds);
  //   #if defined(ESP32)
  //     espSleep(milliseconds);
  //   #else
  //     delay(milliseconds);
  //   #endif
  // #endif
}

// MARK: UPDATE CYCLE ---------------------------------------

void updatesHourly(){
  
  if (isProvisioningActive()){
    return;
  }

  LOGMEM;
  Serial.println("Hourly update");
  if (locationManager -> hasSavedLocation()){
    if (reconnect()) {
      weatherClient -> setLocation(locationManager -> getLocation());
      weatherClient -> setTimeHorizon(12);
      weatherClient -> fetchWeather();
      display->setWeather(weatherClient->horizonWeather);
      rainbows.setWeather(weatherClient->rainbowWeather);
      LOGMEM;
    }

  } else {
    display->displayMessage("Where am I", bad);
  }
}

void updatesDaily(){

  Serial.println("Daily update");
  LOGMEM;
  #if defined(TIMESOURCE_NTP)
  if (reconnect()) {
    DateTime ntpTime;
    if (timeFromNTP(ntpTime)){
      rtc.adjust(ntpTime);
      // display->displayMessage("Synchronised with NTP", good);
    } else {
      // display->displayMessage("No sync", bad);
    }
  }
  #endif
  generateDSTTimes(rtc.now().year());

  if (isProvisioningActive()){
    return;
  }

  // Firmware updates

  
  LOGMEM;

  if (reconnect()) {
    FirmwareUpdates *firmwareUpdates = new FirmwareUpdates;
    Preferences preferences = Preferences();
    preferences.begin("clocklet", true);
    bool staging = preferences.getBool("staging",false);
    if (firmwareUpdates->checkForUpdates(staging)){
      if (firmwareUpdates->updateAvailable){
        display->displayMessage("Updating Firmware", rando);
        display->setStatusMessage("wait");
        if (!firmwareUpdates->startUpdate()){
          display->displayMessage("Update failed... sorry",bad);
        }
      }
    } else {
      ESP_LOGI("CORE","Update check failed");
    }
    preferences.end();

    delete firmwareUpdates;
    Serial.println("Firmware update done");
    LOGMEM;
  }
}

DateTime dstStart;
DateTime dstEnd;

void generateDSTTimes(uint16_t year){
  // European DST rules:
  // last sunday in march at 01:00 utc
  DateTime eom = DateTime(year, 3, 31);
  int lastSun = 31 - (eom.dayOfTheWeek() % 7);
  dstStart = DateTime(year, 3, lastSun , 1);

  // last sunday in october at 01:00 utc
  DateTime eoo = DateTime(year, 10, 31);
  lastSun = 31 - eoo.dayOfTheWeek();
  dstEnd = DateTime(year, 10, lastSun , 1);

  Serial.println(dstStart.unixtime());
  Serial.println(dstEnd.unixtime());

  // Making this work anywhere is going to be more complex.
}

uint16_t dstAdjust(DateTime time){
  if (time.unixtime() >= dstStart.unixtime() && time.unixtime() < dstEnd.unixtime() ) {
    return 1;
  } else {
    return 0;
  }
}

// MARK: BRIGHTNESS SENSING -------------------------

int lightPin = 36;
const int readingWindow = 10;
uint16_t readings[readingWindow] = {1024};
int readingIndex = 0;

float currentBrightness(){
  readings[readingIndex] = analogRead(lightPin);
  readingIndex++;
  if (readingIndex == readingWindow) { readingIndex = 0; }
  uint16_t sum = 0; // The sum can be a 16 bit integer because
  // 4096 goes into 2^16 - 16 times and we are doing 10 readings
  for (int loop = 0 ; loop < readingWindow; loop++) {
    sum += readings[loop];
  }

  uint16_t lightReading = sum / (uint16_t)readingWindow;
  return lightReading / 4096.0f;

}

// MARK: TOUCH SENSITIVITY ------------------------
long startTouchMillis = 0;

/// Returns the number of ms which the user has been touching the device
long detectTouchPeriod(){
  int touchValue = touchRead(27);

  // if (touchValue < 43){ // touch threshold is a mess
  //   if (!startTouchMillis){
  //     startTouchMillis = millis();
  //   }
  //   return millis() - startTouchMillis;
  // } else {
  //   startTouchMillis = 0;
  //   return 0;
  // }

}

// MARK: POWER MANAGEMENT -------------------------

#if defined(BATTERY_MONITORING)

float batteryLevel(float voltage){
  float v = (voltage - cutoffVoltage) / (maxVoltage - cutoffVoltage);
  v = fmax(0.0f,v); // make sure it doesn't go negative
  return fmin(1.0f, v); // cap the level at 1
}

float batteryVoltage(){
  float reading = analogRead(BATTERY_PIN);
  return (reading / 4095.0f) * 2.0f * 3.3f * 1.1;
}

#endif


#if defined(ESP32)
void espSleep(int milliseconds){
  
  stopWifi();

  #if defined(TIME_GPS)
  rtc.sleep();
  #endif
  Serial.println("SLEEP");
  uart_tx_wait_idle(0);
  uint64_t microseconds = milliseconds * 1000;

  esp_err_t err = esp_sleep_enable_timer_wakeup( microseconds );
  if (err == ESP_ERR_INVALID_ARG){
    Serial.println("Sleep timer wakeup invalid");
    return;
  }
  err = esp_light_sleep_start();

  if (err == ESP_ERR_INVALID_STATE){
    Serial.println("Trying to sleep: Invalid state error");
  }
  Serial.println("AWAKE");
}

void espShutdown(){
  // display->setStatusMessage("LOW BATTERY");
  // Serial.println("LOW BATTERY shutting down");
  // esp_deep_sleep_start();
}
#endif
