#include "Priscilla.h"

// CONFIGURATION  --------------------------------------

// Time zone adjust
int utcAdjust = 1 * 3600;

// Set to false to display time in 12 hour format, or true to use 24 hour:
#define TIME_24_HOUR      true


// ----------- RTC

// Create display and RTC_DS3231 objects.  These are global variables that
// can be accessed from both the setup and loop function below.

RTC_DS3231 rtc = RTC_DS3231();

// ----------- TIME SERVER

unsigned int localPort = 2390;      // local port to listen for UDP packets

// IPAddress timeServer(130, 88, 202, 49);//ntp2a.mcc.ac.uk
//IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
//IPAddress timeServer(194,35,252,7); //chronos.csr.net
IPAddress timeServer(129, 250, 35, 251); //some other one from the pool
// char timeServerPool[] = "pool.ntp.org";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;

// A variable for the current weather type
int currentWeather = 31;

int randoMinute = random(0,59);

// A variable for the current set of colours

Colour currentColours[5];



// SETUP  --------------------------------------

void setup() {
  delay(4000);
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
 while (!Serial) {
   ; // wait for serial port to connect. Needed for native USB port only
 }
  randomSeed(analogRead(0));

  Serial.println("Clock starting!");

  initDisplay();

  memcpy(currentColours,RAINBOW,5*3);

  scrollText("everything is awesome");

  showTime();

  setupWifi();


  // Get the time from NTP.
  updateRTCTimeFromNTP();

  showTime();


  int weather = fetchWeather();
  if (weather >= 0) {
    currentWeather = weather;
  }
  memcpy(currentColours,weatherTypeColours[currentWeather],5*3);

  showTime();

}

// LOOP  --------------------------------------

void loop() {

  updateBrightness();

  DateTime time = rtc.now();

  if (time.second() == 0) {

    if (time.minute() == 0) {
      randoMinute = random(0,59);


      // Get the time from NTP.
      updateRTCTimeFromNTP();

      int weather = fetchWeather();
      if (weather >= 0) {
        currentWeather = weather;
      }
      memcpy(currentColours,weatherTypeColours[currentWeather],5*3);
    }

    if (time.minute() == randoMinute){
      randoMessage();
    }
  }


  showTime();

  // Pause for a second for time to elapse.  This value is in milliseconds
  // so 1000 milliseconds = 1 second.
  delay(1000);


  // Loop code is finished, it will jump back to the start of the loop
  // function again!
}

// MARK: UPDATE CYCLE ---------------------------------------

void performUpdates(bool forceAll){
    // Get the time from NTP.
    updateRTCTimeFromNTP();

    // Update weather
    int currentWeather = fetchWeather();
    memcpy(currentColours,weatherTypeColours[currentWeather],5*3);
}


void showTime(){
  Serial.println("Show time");
  DateTime time = rtc.now();
  Serial.println("YES");
  displayTime(time, currentColours);
}


// MARK: TIME SYNC STUFF --------------------------------------


void updateRTCTimeFromNTP(){
  if ( !connectWifi() ){
    return;
  }

  Serial.println("\nStarting connection to server...");
  Udp.begin(localPort);

  unsigned long timeout = 2000;
  int maxRetries = 4;
  int retries = 0;

  unsigned long startMillis = millis();
  unsigned long timech = millis();
  sendNTPpacket(timeServer); // send an NTP packet to a time server
  // wait to see if a reply is available

  int packet = 0;

  while( packet == 0 ){
    packet = Udp.parsePacket();
    if (millis() - timech >= timeout) {
      retries ++;
      if (retries >= maxRetries ) {
        Serial.print("Didn't get a packet back... skipping sync...");
        scrollText_fail("failed to update ntp");
        return;
      }
      timech = millis();
      sendNTPpacket(timeServer);
    }
  }

  Serial.print("Roundtrip Time:");
  Serial.println(millis() - startMillis);

  Serial.println("packet received");
  // We've received a packet, read the data from it
  Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

  //the timestamp starts at byte 40 of the received packet and is four bytes,
  // or two words, long. First, esxtract the two words:

  unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
  unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
  // combine the four bytes (two words) into a long integer
  // this is NTP time (seconds since Jan 1 1900):
  unsigned long secsSince1900 = highWord << 16 | lowWord;
  Serial.print("Seconds since Jan 1 1900 = " );
  Serial.println(secsSince1900);

  // now convert NTP time into everyday time:
  Serial.print("Unix time = ");
  // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
  const unsigned long seventyYears = 2208988800UL;
  // subtract seventy years:
  unsigned long epoch = secsSince1900 - seventyYears;
  // print Unix time:
  Serial.println(epoch);


  // print the hour, minute and second:
  Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
  Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
  Serial.print(':');
  if ( ((epoch % 3600) / 60) < 10 ) {
    // In the first 10 minutes of each hour, we'll want a leading '0'
    Serial.print('0');
  }
  Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
  Serial.print(':');
  if ( (epoch % 60) < 10 ) {
    // In the first 10 seconds of each minute, we'll want a leading '0'
    Serial.print('0');
  }
  Serial.println(epoch % 60); // print the second


  rtc.adjust(DateTime(epoch) + utcAdjust);

  Serial.print("Time to adjust time:");
  Serial.println(millis() - startMillis);

  scrollText("synchronised");
}


// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress& address)
{
  Serial.println("Sending NTP packet");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)

  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}