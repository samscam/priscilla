#include "display.h"
#include "p.h"
#include "settings.h"

FASTLED_USING_NAMESPACE

// LDR pins
int lightPin = 0;
static const float min_brightness = 5;
static const float max_brightness = 150;

CRGB leds[NUM_LEDS];


// ----------- RANDOM MESSAGES

const char* messages[] = {
  "ball boy",
  "bread for people",
  "bread for babies",
  "bread for ducks",
  "you must obey the clock",
  "you must obey the egg timer",
  "you must obey things in this corner",
  "commit",
  "woop woop time to rub bellies woop woop",
  "all your base are belong to us",
  "accuracy is guaranteed",
  "do the ham dance",
  "striving to be less shite",
  "just a baby fart",
};

#define numMessages (sizeof(messages)/sizeof(char *)) //array size


void initDisplay(){
    FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

    initRain();
    analogReadResolution(12);
}

void randoMessage(){
  int messageIndex = random(0,numMessages-1);
  const char* randoMessage = messages[messageIndex];

  // Do it three times
  for (int i=0;i<3;i++){
    CHSV colour = CHSV(random8(), 255, 255);
    scrollText(randoMessage,colour);
  }
}

void scrollText(const char *stringy){
  scrollText(stringy, CRGB::Green);
}

void scrollText_fail(const char *stringy){
  scrollText(stringy, CRGB::Red);
}

void scrollText(const char *stringy, CRGB colour){
  Serial.println(stringy);

  char charbuffer[DIGIT_COUNT] = { 0 };
  int origLen = strlen(stringy);
  int extendedLen = origLen + DIGIT_COUNT;
  char res[extendedLen];
  memset(res, 0, extendedLen);
  memcpy(res,stringy,origLen);

  int i;
  for ( i = 0; i < extendedLen ; i++ ) {
    fill_solid(leds,NUM_LEDS,colour);

    for ( int d = 0; d < DIGIT_COUNT ; d++ ) {
      if (d == 3) {
        charbuffer[d] = res[i];
      } else {
        charbuffer[d] = charbuffer[d+1];
      }

      setDigit(charbuffer[d], d);

    }

    FastLED.show();
    FastLED.delay(200);

  }


}

// MARK: DISPLAY THINGS --------------------------------------


// Remember if the colon was drawn on the display so it can be blinked
// on and off every second.
bool blinkColon = false;

void displayTime(const DateTime& time, weather weather){
  fillDigits_rainbow(false, weather.windSpeed);

  float precip = weather.precipChance * 100;
  float minTmp = weather.minTmp;

  precip = precip - 25;
  precip = precip < 0 ? 0 : precip;
  fract8 rainRate = (precip * 255) / 75.0;

  if (rainRate > 0) {
    // p("Rain %f - %f - %d\n",anal,precip,rainRate);
    switch (weather.precipType) {
      case Snow:
        addSnow(rainRate);
        break;
      case Rain:
        addRain(rainRate, CRGB::Blue);
        break;
      case Sleet:
        addRain(rainRate, CRGB::White);
        break;
    }
  }

  if (minTmp<=0){
    addFrost();
  }

  if (weather.thunder){
    addLightening();
  }

  maskTime(time);

  blinkColon = (time.second() % 2) == 0;
  setDot(blinkColon,1);

  FastLED.show();

}

void maskTime(const DateTime& time){
  int digit[4];

  int h = time.hour();
  digit[0] = h/10;                      // left digit
  digit[1] = h - (h/10)*10;             // right digit

  int m = time.minute();
  digit[2] = m/10;
  digit[3] = m - (m/10)*10;

  for (int i = 0; i<4 ; i++){
    setDigit(digit[i], i); // show on digit 0 (=first). Color is rgb(64,0,0).
  }
}

uint8_t brightness;

const int readingWindow = 10;
float readings[readingWindow] = {4096.0f};
int readingIndex = 0;

void updateBrightness(){
  readings[readingIndex] = analogRead(lightPin);
  readingIndex++;
  if (readingIndex == readingWindow) { readingIndex = 0; }
  float sum = 0;
  for (int loop = 0 ; loop < readingWindow; loop++) {
    sum += readings[loop];
  }
  float lightReading = sum / readingWindow;

  float bRange = max_brightness - min_brightness;

  brightness = (lightReading * bRange / 4096.0f) + min_brightness;
  // Serial.println(brightness);
  FastLED.setBrightness(brightness);
}

//   0
// 5   1
//   6
// 4   2
//   3   7

const byte _numberMasks[10] = {
  B11111100,//0
  B01100000,//1
  B11011010,//2
  B11110010,//3
  B01100110,//4
  B10110110,//5
  B10111110,//6
  B11100000,//7
  B11111110,//8
  B11110110,//9
};

void setDigit(int number, int digit){
  byte mask = _numberMasks[number];
  setDigitMask(mask,digit);
}

const byte _charMasks[36] = {
  B11101110, // a
  B00111110, // b
  B00011010, // c
  B01111010, // d
  B10011110, // e
  B10001110, // f
  B11110110, // g
  B00101110, // h
  B01100000, // i
  B01110000, // j
  B01101111, // k
  B00011100, // l
  B00101011, // m crap
  B00101010, // n
  B00111010, // o
  B11001110, // p
  B11100110, // q
  B00001010, // r
  B10110110, // s
  B00011110, // t
  B00111000, // u
  B00110000, // v crap
  B00111001, // w crap
  B00110011, // x crap
  B01110110, // y
  B11011010, // z
};

void setDigit(char character, int digit){
  byte mask;
  if (character < 48) {
    mask = B00000000;
    // It's a control char
  } else if (character >= 48 && character <= 57) {
    mask = _numberMasks[character - 48];
    // Numbers
  } else if (character >= 65 && character <= 90) {
    // Uppercase
    mask = _charMasks[character-65];
  } else if (character >= 97 && character <= 122) {
    // Lowercase
    mask = _charMasks[character-97];
  } else {
    // out of range
    mask = B00000000;
  }

  setDigitMask(mask,digit);
}



void setDigitMask(byte mask, int digit){
  int bit = 0;

  while (bit < 8) {
    if (mask & 0x01){
      //leds[ 7 - bit + (digit * DIGIT_SEGS) ] = CRGB::Crimson;
    } else {
      leds[ 7 - bit + (digit * DIGIT_SEGS) ] = CRGB::Black;
    }
    bit++;
    mask = mask >> 1;
  }
}

void setDot(bool state, int digit){
  leds[ 7 + (digit * DIGIT_SEGS)] = state ? CRGB::White : CRGB::Black ;
}

void setDigits(const char *string){
  for (int i=0;i<DIGIT_COUNT;i++){
    setDigit(string[i],i);
  }
}

float cycle = 0;
void fillDigits_rainbow(bool includePoints, float speed){
  // speed is in m/s
  // at 1 m/s cycle takes 51 seconds
  // at 10 m/s cycle takes 5.1 seconds
  cycle = cycle + ( (speed * 5.0f ) / (float)FPS );
  if (cycle > 255.0f) {
    cycle -= 255.0f;
  }
  uint8_t hue = cycle;

  //p("speed %f - cycle %f - hue %d \n",speed,cycle,hue);

  if (includePoints){
    CHSV cols[4*NUM_DIGITS];
    fill_rainbow( cols, 4*NUM_DIGITS, hue, 6);

    int mapping[] = {
      1,2,2,1,0,0,1,3
    };

    for (int i = 0; i < NUM_LEDS; i++){
      leds[i] = cols[ mapping[i%8] + ((i/8)*4) ];
    }

  } else {
    CHSV cols[3*NUM_DIGITS];

    fill_rainbow( cols, 3*NUM_DIGITS, hue, 8);

    int mapping[] = {
      1,2,2,1,0,0,1,2
    };

    for (int i = 0; i < NUM_LEDS; i++){
      leds[i] = cols[ mapping[i%8] + ((i/8)*3) ];
    }
  }



}

// Rain

CRGB rainLayer[NUM_LEDS];
int vsegs[4] = {1,2,4,5};
int allvsegs[ 4 * NUM_DIGITS ] = {0};

/// Creates the mapping in `allvsegs` to the vertical segments in `leds`
void initRain(){
  // Init rain
  int s = 0;

  for (int digit = 0; digit < NUM_DIGITS ; digit++) {
    for (int vseg = 0 ; vseg < 4 ; vseg++){
      int digStep = DIGIT_SEGS * digit;
      int val = vsegs[vseg] + digStep;
      allvsegs[s] = val;
      //p("digit %d - vseg %d - digStep %d - val %d\n",digit,vseg,digStep,val);
      s++;
    }
  }
}

void addRain( fract8 chanceOfRain, CRGB colour)
{
  for(int i = 0; i < NUM_LEDS; i++) {
    rainLayer[i].nscale8(230);
  }
  if( random8() < chanceOfRain) {
    int segnum = random8(4 * NUM_DIGITS);

    rainLayer[ allvsegs[segnum] ] = colour;
  }
  nscale8_video(leds, NUM_LEDS, 255 - (chanceOfRain * 0.75));

  for(int i = 0; i < NUM_LEDS; i++) { leds[i] += rainLayer[i] ; }
}

// Snow

void addSnow( fract8 chanceOfSnow ) {
  for(int i = 0; i < NUM_LEDS; i++) {
    rainLayer[i].nscale8_video(252);
  }
  if( random8() < chanceOfSnow) {
    int segnum = random8(NUM_LEDS);
    rainLayer[ segnum ] = CRGB::White;
  }
  nscale8_video(leds, NUM_LEDS, 255 - chanceOfSnow);

  for(int i = 0; i < NUM_LEDS; i++) { leds[i] += rainLayer[i] ; }
}

// Frost

void addFrost(){
  CRGB frostLayer[NUM_LEDS];
  fill_solid(frostLayer, NUM_LEDS, CRGB::Black);
  // We want the bottom row and the next one up...

  for (int d = 0; d < NUM_DIGITS; d++){
    // Serial.println((d*8) + 3);

    frostLayer[ (d*8) + 3 ] = CHSV(0,0,200);
    frostLayer[ (d*8) + 2 ] = CHSV(0,0,90);
    frostLayer[ (d*8) + 4 ] = CHSV(0,0,90);
  }

  //nblend(leds, frostLayer, NUM_LEDS,  80);
  for(int i = 0; i < NUM_LEDS; i++) { leds[i] += frostLayer[i] ; }
}

// Lightening

CRGB lighteningLayer[NUM_LEDS];
void addLightening(){
  for(int i = 0; i < NUM_LEDS; i++) {
    lighteningLayer[i].nscale8(150);
  }
  if(random8() == 1) {
    fill_solid(lighteningLayer, NUM_LEDS, CRGB::White);
  }
  for(int i = 0; i < NUM_LEDS; i++) { leds[i] += lighteningLayer[i] ; }
}
