#pragma once

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEService.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <BLE2902.h>
#include "Utilities/Task.h"

#define SV_NETWORK_UUID     "68D924A1-C1B2-497B-AC16-FD1D98EDB41F"


enum WifiConfigStatus {
    notConfigured,
    configured
};

struct NetworkInfo {
    uint8_t index;
    uint8_t enctype;
    int32_t rssi;
    uint8_t * bssid;
    int32_t channel;
    String ssid;
};

class NetworkScanTask: public Task {
public:
    NetworkScanTask(BLECharacteristic *availableNetworks,BLE2902 *dec_availableNetworks_2902);
    void run(void *data);
private:
    BLECharacteristic *ch_availableNetworks;
    BLE2902 *_dec_availableNetworks_2902;
    void _performWiFiScan();
    void _encodeNetInfo(JsonDocument &doc, NetworkInfo netInfo);
};

class BTNetworkService: public BLECharacteristicCallbacks  {
public:
    BTNetworkService(BLEServer *server, QueueHandle_t networkChangedQueue, QueueHandle_t networkStatusQueue);
    ~BTNetworkService() {};

    void wifiEvent(WiFiEvent_t event);
    void onWrite(BLECharacteristic* pCharacteristic);

    void onConnect();
    void onDisconnect();

private:

    void _updateCurrentNetwork();

    void _encodeNetInfo(JsonDocument &doc, NetworkInfo netInfo);


    bool _shouldScan = false;

    

    QueueHandle_t _networkChangedQueue;
    QueueHandle_t _networkStatusQueue;

    NetworkScanTask *_networkScanTask;

    // Network provisioning
    BLEService *sv_network;
    BLECharacteristic *ch_currentNetwork; // returns info on current network connection
    BLECharacteristic *ch_availableNetworks; // returns list of known and available networks
    BLE2902 *dec_availableNetworks_2902;

    BLECharacteristic *ch_removeNetwork; // takes an SSID of one of the known networks - removes it from the list preventing future connection
    BLECharacteristic *ch_joinNetwork; // Joins a network

    wifi_event_id_t _wifiEvent;
};

String _mac2String(uint8_t * bytes);

void wifiEventCb(WiFiEvent_t event);