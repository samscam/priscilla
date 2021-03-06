#include "BTPreferencesService.h"

#define TAG "BT-PREFS"

BTPreferencesService::BTPreferencesService(NimBLEServer *server, QueueHandle_t prefsChangedQueue){

    pservice = server->createService("28C65464-311E-4ABF-B6A0-D03B0BAA2815");

    // The available separator animations
    // JSON fragment
    availableSeparatorAnimationsCharacteristic = pservice->createCharacteristic(
        "9982B160-23EF-42FF-9848-31D9FF21F490",
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::READ_ENC);

    availableSeparatorAnimationsCharacteristic->setValue(std::string("[\"None\",\"Blink\",\"Fade\"]"));


    separatorAnimationsGlue = new PreferencesGlue<std::string>("2371E298-DCE5-4E1C-9CB2-5542213CE81C",
        "sep_anim",
        pservice,
        prefsChangedQueue,
        "clocklet",
        "Blink");


    availableTimeStyles = pservice->createCharacteristic(
        "698D2B57-5B54-48D7-A483-1AB4660FBAF9",
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::READ_ENC);
    availableTimeStyles->setValue(std::string("[\"24 Hour\",\"12 Hour\",\"Decimal\"]"));

    timeStyleGlue = new PreferencesGlue<std::string>("AE35C2DE-7D36-4699-A5CE-A0FA6A0A5483",
        "time_style",
        pservice,
        prefsChangedQueue,
        "clocklet",
        "24 Hour");

    brightnessGlue = new PreferencesGlue<float_t>("8612F6ED-AA92-45A7-8B46-166F600BC53D",
        "brightness",
        pservice,
        prefsChangedQueue,
        "clocklet",
        0.5f);
    
    autoBrightnessGlue = new PreferencesGlue<bool>("A56AE45C-EB82-4182-9B9C-C91F509C91D5",
        "autoBrightness",
        pservice,
        prefsChangedQueue,
        "clocklet",
        true);

    pservice->start();
}

