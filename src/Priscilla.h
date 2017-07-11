#ifndef PRISCILLA
#define PRISCILLA

#include <Arduino.h>

#include <SPI.h>
#include <WiFi101.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <RTClib.h>


#include "weather.h"
#include "network.h"
#include "display.h"
#include "colours.h"



#define FPS 60


// Function declarations

void performUpdates();

void showTime();
void updateRTCTimeFromNTP();

void sendNTPpacket(IPAddress& address);




#endif
