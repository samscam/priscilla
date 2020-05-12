#include "Tests.h"

#include <RTClib.h>


void runTests(){

}

void displayTests(Display *display){
    
    display->graphicsTest();

    display->setStatusMessage("ABCD");
    delay(3000);
    display->setStatusMessage("EFGH");
    delay(3000);
    display->setStatusMessage("IJKL");
    delay(3000);
    display->setStatusMessage("MNO");
    delay(3000);
    display->setStatusMessage("PQRS");
    delay(3000);
    display->setStatusMessage("TUVW");
    delay(3000);
    display->setStatusMessage("XYZ");
    delay(3000);
    display->setStatusMessage("FAST");
    delay(3000);
    runFast(display);

}


void runFast(Display *display){
    
    DateTime time = DateTime(1978,12,28,0,0,0);
    DateTime endTime = DateTime(1978,12,29,0,0,0);
    while (time.unixtime() < endTime.unixtime()){
        display->setTime(time);
        display->frameLoop();
        time = time + TimeSpan(60);
    }

}


// #include <FastLED.h>
//
// #define FPS 60
//
// void runDemo(){
//
//     // Temperature Colours
//

//
//
//     runDemo();
//     // rain
//     // fract8 chance = 255;
//     // for (int16_t speed = 0; speed < 50; speed += 1){
//     //   for (int f = 0; f<60; f++){
//     //     updateBrightness();
//     //     fillDigits_rainbow(true, speed);
//     //
//     //     setDigits("wind");
//     //     FastLED.show();
//     //     FastLED.delay(1000/FPS);
//     //   }
//     //   Serial.println(speed);
//     // }
// /*
//     for (int16_t chance = 0; chance < 255; chance = chance + 16){
//       for (int f = 0; f<60; f++){
//         updateBrightness();
//         fillDigits_rainbow(true, 0.0);
//         addRain(chance,CRGB::Blue);
//         setDigits("rain");
//         FastLED.show();
//         FastLED.delay(1000/FPS);
//       }
//       Serial.println(chance);
//     }
//
//     for (int16_t chance = 0; chance < 255; chance = chance + 16){
//       for (uint8_t f = 0; f<120; f++){
//         updateBrightness();
//         fillDigits_rainbow(true,0.0);
//         addRain(chance,CRGB::White);
//         setDigits("hail");
//         FastLED.show();
//         FastLED.delay(1000/FPS);
//       }
//     }
//
//     for (int16_t chance = 0; chance < 255; chance = chance + 16){
//       for (uint8_t f = 0; f<120; f++){
//         updateBrightness();
//         fillDigits_rainbow(true,0.0);
//         addSnow(chance);
//         setDigits("snow");
//         FastLED.show();
//         FastLED.delay(1000/FPS);
//       }
//     }
//     */
// }
