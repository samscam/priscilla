
#include "BlueStuff.h"
#include "esp_bt_device.h"

#include <Preferences.h>

#include "BLE2902.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <esp_err.h>
#include <nvs.h>

#include <esp_log.h>
#include "Loggery.h"

#include "../ClockletSystem.h"


#define TAG "BLUESTUFF"


BlueStuff::BlueStuff(QueueHandle_t preferencesChangedQueue,
            QueueHandle_t networkChangedQueue,
            QueueHandle_t networkStatusQueue, LocationManager *locationManager) : Task("UpdateScheduler", 5000,  5) {
    this->setCore(1); // Run it on core one
    _preferencesChangedQueue =  preferencesChangedQueue;
    _networkChangedQueue = networkChangedQueue;
    _networkStatusQueue = networkStatusQueue;
    _locationManager = locationManager;
}

void BlueStuff::run(void *data) {

    startBlueStuff();

    // Wait for ever...
    TickType_t xFrequency = pdMS_TO_TICKS(1000);
    TickType_t xLastWakeTime = xTaskGetTickCount();
    for (;;){
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }

}

void BlueStuff::startBlueStuff(){
    LOGMEM;

    ESP_LOGI(TAG,"Starting BLE work!");

    uint16_t hwrev = clocklet_hwrev();
    uint16_t caseColour = clocklet_caseColour();
    uint32_t serial = clocklet_serial();

    if (isnan(serial)){
        serial = 0;
    }
    
    const char * shortName = "Clocklet";
    
    LOGMEM;

    char * deviceName;
    asprintf(&deviceName,"%s #%d",shortName,serial);
    BLEDevice::init(deviceName);
    
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
    
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(this);
    

    // Add the Generic Attribute Service 0x1801
    // - and 
    // This is for informing clients that the services have changed and that
    // the client should try and discover the services from scratch.
    // This would be better if we knew whether the services have actually changed
    // since the given client last connected. But for now we are going to say yes every time.

    // Good explanation: https://punchthrough.com/attribute-caching-in-ble-advantages-and-pitfalls/
    // Another one: https://www.oreilly.com/library/view/getting-started-with/9781491900550/ch04.html#gatt_service
    // What I would have hoped would be that the library knew how to do this
    // But no!

    // But then... is this right???

    sv_GAS = pServer->createService(BLEUUID((uint16_t)0x1801));
    ch_ServiceChanged = sv_GAS->createCharacteristic(BLEUUID((uint16_t)0x2A05),BLECharacteristic::PROPERTY_INDICATE);
    BLE2902* p2902Descriptor = new BLE2902();
    p2902Descriptor->setIndications(true);
    ch_ServiceChanged->addDescriptor(p2902Descriptor);
    sv_GAS->start();
    
    _locationService = new BTLocationService(_locationManager,pServer);
    _networkService = new BTNetworkService(pServer, _networkChangedQueue, _networkStatusQueue);
    _preferencesService = new BTPreferencesService(pServer, _preferencesChangedQueue);
    _technicalService = new BTTechnicalService(pServer);

    // BLE Advertising

    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    
    BLEAdvertisementData advertisementData;
    BLEAdvertisementData scanResponseData;

    advertisementData.setName(deviceName);
    advertisementData.setPartialServices(BLEUUID(SV_NETWORK_UUID));

    char mfrdataBuffer[10];

    const char vendorID[2] = { 0x02, 0xE5 };
    memcpy(mfrdataBuffer, vendorID, 2);

    mfrdataBuffer[2] = (hwrev >> 0) & 0xFF;
    mfrdataBuffer[3] = (hwrev >> 8) & 0xFF;

    mfrdataBuffer[4] = (caseColour >> 0) & 0xFF;
    mfrdataBuffer[5] = (caseColour >> 8) & 0xFF;

    mfrdataBuffer[6] = (serial >> 0) & 0xFF;
    mfrdataBuffer[7] = (serial >> 8) & 0xFF;
    mfrdataBuffer[8] = (serial >> 16) & 0xFF;
    mfrdataBuffer[9] = (serial >> 24) & 0xFF;

    auto s = std::string(mfrdataBuffer,sizeof(mfrdataBuffer));
    scanResponseData.setManufacturerData(s);

    pAdvertising->setScanResponseData(scanResponseData);
    pAdvertising->setAdvertisementData(advertisementData);

    // Mythical settings that help with iPhone connections issue - don't seem to make any odds
    pAdvertising->setMinPreferred(0x06);  
    pAdvertising->setMinPreferred(0x12);
    pAdvertising->setScanResponse(true);
    pAdvertising->start();

    LOGMEM;
}

void BlueStuff::stopBlueStuff(){
    
}

void BlueStuff::onConnect(BLEServer* server) {
    ESP_LOGI(TAG,"Bluetooth client connected");
    
    // Force service changed indication for the whole table
    char val[4] = {0x00,0x00,0xFF,0xFF};
    ch_ServiceChanged->setValue(val);
    ch_ServiceChanged->indicate();
    delay(500);
    _networkService->onConnect();
    
}

void BlueStuff::onDisconnect(BLEServer* server) {
    ESP_LOGI(TAG,"Bluetooth client disconnected");
    _networkService->onDisconnect();
}





esp_err_t BlueStuff::_app_prov_is_provisioned(bool &provisioned)
{
    provisioned = false;


    if (nvs_flash_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init NVS");
        return ESP_FAIL;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init wifi");
        return ESP_FAIL;
    }

    /* Get WiFi Station configuration */
    wifi_config_t wifi_cfg;
    if (esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_cfg) != ESP_OK) {
        return ESP_FAIL;
    }

    if (strlen((const char *) wifi_cfg.sta.ssid)) {
        provisioned = true;
        ESP_LOGI(TAG, "Found ssid %s",     (const char*) wifi_cfg.sta.ssid);
        ESP_LOGI(TAG, "Found password %s", (const char*) wifi_cfg.sta.password);
    }
    
    return ESP_OK;
}


bool BlueStuff::isAlreadyProvisioned(){

    bool provisioned;
    
    if (_app_prov_is_provisioned(provisioned) != ESP_OK) {
        ESP_LOGE(TAG, "Error getting device provisioning state");
        return false;
    }
    return provisioned;
}