#include "BTNetworkService.h"


#define MAX_NETS 128


#include <esp_wifi.h>
#include <esp_log.h>
#include "Loggery.h"

#define TAG "BTNetworkService"

#define CH_CURRENTNETWORK_UUID "BEB5483E-36E1-4688-B7F5-EA07361B26A8"
#define CH_AVAILABLENETWORKS_UUID "AF2B36C7-0E65-457F-A8AB-B996B656CF32"
#define CH_JOINNETWORK_UUID "DFBDE057-782C-49F8-A054-46D19B404D9F"


NetworkScanTask::NetworkScanTask(BLECharacteristic *availableNetworks, BLE2902 *dec_availableNetworks_2902) : Task("NetworkScan", 3072,  5){
    this->setCore(1);
    ch_availableNetworks = availableNetworks;
    _dec_availableNetworks_2902 = dec_availableNetworks_2902;
}

void NetworkScanTask::run(void *data){
    delay(3000);
    for (;;){
        if (_dec_availableNetworks_2902 -> getNotifications()){
            _performWiFiScan();
            if ( !WiFi.isConnected() ) WiFi.begin();
            delay(10000);
        } else {
            delay(500);
        }
        
    }
}

void NetworkScanTask::_performWiFiScan(){
    LOGMEM;
    
    bool runningScan = (WiFi.scanNetworks(true,true,false) == WIFI_SCAN_RUNNING);
    int networkCount = 0;

    while (runningScan){
        int16_t scanComplete = WiFi.scanComplete();
        
        if (scanComplete == WIFI_SCAN_FAILED) {
            return;
        }

        if (scanComplete >= 0){
            networkCount = scanComplete;
            runningScan = false;
        }

        delay(10);
    }


    for (int i=0;(i<networkCount && i<MAX_NETS);i++){
        StaticJsonDocument<512> doc;
        NetworkInfo netInfo = {0};
        netInfo.index = i;

        WiFi.getNetworkInfo(netInfo.index,netInfo.ssid,netInfo.enctype,netInfo.rssi,netInfo.bssid,netInfo.channel);

        ESP_LOGI(TAG,"Found %s (ch %d rssi %d)\n",netInfo.ssid,netInfo.channel,netInfo.rssi);

        _encodeNetInfo(doc,netInfo);
        String outputStr = "";
        serializeJson(doc,outputStr);

        uint len = outputStr.length()+1;
        char availableJson[len];
        outputStr.toCharArray(availableJson,len);
        ESP_LOGI(TAG,"%s",availableJson);
        ch_availableNetworks->setValue(availableJson);
        ch_availableNetworks->notify(true);
        delay(200);
    }

    LOGMEM;
}


void NetworkScanTask::_encodeNetInfo(JsonDocument &doc, NetworkInfo netInfo){
    // JsonObject obj = doc.createNestedObject();
    doc["ssid"]=netInfo.ssid;
    doc["enctype"]=netInfo.enctype;
    doc["channel"]=netInfo.channel;
    doc["rssi"]=netInfo.rssi;
    doc["bssid"]=_mac2String(netInfo.bssid);
}

// This bit is horrible - really something else should be doing the job of catching wifi status updates and distributing them around via a queue
BTNetworkService *btNetworkServiceInstance;
void wifiEventCb(WiFiEvent_t event)
{
    // callback needs to definitely be on the right processor too!!!
    if (btNetworkServiceInstance){
        btNetworkServiceInstance->wifiEvent(event);
    }
}

// Initialise network related service and characteristics
BTNetworkService::BTNetworkService(BLEServer *pServer, QueueHandle_t networkChangedQueue, QueueHandle_t networkStatusQueue){
    _networkChangedQueue = networkChangedQueue;
    _networkStatusQueue = networkStatusQueue;

    btNetworkServiceInstance = this;



    ESP_LOGI(TAG, "Starting network service %s",SV_NETWORK_UUID);
    sv_network = pServer->createService(SV_NETWORK_UUID);

    ch_currentNetwork = sv_network->createCharacteristic(
                                            CH_CURRENTNETWORK_UUID,
                                            BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE
                                        );
    ch_currentNetwork->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED);

    ch_availableNetworks = sv_network->createCharacteristic(
                                            CH_AVAILABLENETWORKS_UUID,
                                            BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE
                                        );
    ch_availableNetworks->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED);


    ch_joinNetwork = sv_network->createCharacteristic(
                                            CH_JOINNETWORK_UUID,
                                            BLECharacteristic::PROPERTY_WRITE
                                        );
    ch_joinNetwork->setAccessPermissions(ESP_GATT_PERM_WRITE_ENCRYPTED);
    ch_joinNetwork->setCallbacks(this);

    dec_availableNetworks_2902 = new BLE2902();
    dec_availableNetworks_2902->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
    ch_availableNetworks->addDescriptor(dec_availableNetworks_2902);

    // // ch_currentNetwork->addDescriptor(p2902Descriptor);
    
    // ch_joinNetwork->addDescriptor(p2902Descriptor);
    /*
        * Authorized permission to read/write descriptor to protect notify/indicate requests
        */

    sv_network->start();


}

void BTNetworkService::onConnect(){

    _networkScanTask = new NetworkScanTask(ch_availableNetworks, dec_availableNetworks_2902);
    _networkScanTask->start();

    _wifiEvent = WiFi.onEvent(wifiEventCb);
    _updateCurrentNetwork();

}

void BTNetworkService::onDisconnect(){
    _networkScanTask->stop();
    delete(_networkScanTask);

    WiFi.removeEvent(_wifiEvent);
}


void BTNetworkService::onWrite(BLECharacteristic* pCharacteristic) {



    LOGMEM;
    std::string msg = pCharacteristic->getValue();
    ESP_LOGI(TAG,"BLE received: %s\n", msg.c_str());
    // ESP_LOGI(LOG_TAG, "BLE received: %s", msg.c_str());

    StaticJsonDocument<512> doc;
    deserializeJson(doc,msg);
    
    // Fish out and validate the SSID
    const char * ssid = doc["ssid"];
    if (strlen(ssid) == 0 || strlen(ssid) > 32) {
        ESP_LOGI(TAG,"Invalid ssid length");
        return;
    }

    // If we have no psk... let's see if it's an open network...
    if (doc["psk"].isNull()){
        WiFi.begin(ssid);
        return;
    }


    // Validate the PSK
    // FUDGE - this also depends on the security type
    // For WPA2-PSK we are on an 8-63 char ascii phrase or 64 hex digits
    const char * psk = doc["psk"];
    if (strlen(psk) < 8 || strlen(psk) > 64) {
        ESP_LOGI(TAG, "BLE received: %s", msg.c_str());
        return;
    }

    // Stop any in-progress scans...
    _shouldScan = false;

    // Attempt to connect to the new network !!!!
    WiFi.disconnect();
    delay(1000);
    WiFi.begin(ssid,psk);

}


void BTNetworkService::_updateCurrentNetwork(){
    
    StaticJsonDocument<512> doc;

    bool configured = false;

    wifi_config_t wifi_cfg;
    if (esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_cfg) == ESP_OK) {
        const char *ssid = (const char *) wifi_cfg.sta.ssid;
        if (strlen(ssid)) {
            configured = true;
            doc["ssid"]=ssid;
        }
    }
    
    doc["configured"] = configured;
    doc["status"]=(int)WiFi.status();
    doc["connected"]=WiFi.isConnected();
    doc["channel"]=WiFi.channel();
    doc["ip"]=WiFi.localIP().toString();
    doc["rssi"]=WiFi.RSSI();

    String outputStr = "";
    serializeJson(doc,outputStr);
    uint len = outputStr.length()+1;
    char json[len];
    outputStr.toCharArray(json,len);

    ch_currentNetwork->setValue(json);
    ch_currentNetwork->notify(true);
}



void BTNetworkService::wifiEvent(WiFiEvent_t event){
    
    ESP_LOGD(TAG,"[WiFi-event] event: %d\n", event);
    _updateCurrentNetwork();

    switch (event) {
        case SYSTEM_EVENT_WIFI_READY: 
            ESP_LOGD(TAG,"WiFi interface ready");
            break;
        case SYSTEM_EVENT_SCAN_DONE:
            ESP_LOGD(TAG,"Completed scan for access points");
            break;
        case SYSTEM_EVENT_STA_START:
            ESP_LOGD(TAG,"WiFi client started");
            break;
        case SYSTEM_EVENT_STA_STOP:
            ESP_LOGD(TAG,"WiFi clients stopped");
            break;
        case SYSTEM_EVENT_STA_CONNECTED:
            ESP_LOGD(TAG,"Connected to access point");
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGD(TAG,"Disconnected from WiFi access point");
            break;
        case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
            ESP_LOGD(TAG,"Authentication mode of access point has changed");
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            ESP_LOGD(TAG,"Obtained IP address: %s",WiFi.localIP().toString());
            break;
        case SYSTEM_EVENT_STA_LOST_IP:
            ESP_LOGD(TAG,"Lost IP address and IP address is reset to 0");
            break;
        case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
            ESP_LOGD(TAG,"WiFi Protected Setup (WPS): succeeded in enrollee mode");
            break;
        case SYSTEM_EVENT_STA_WPS_ER_FAILED:
            ESP_LOGD(TAG,"WiFi Protected Setup (WPS): failed in enrollee mode");
            break;
        case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
            ESP_LOGD(TAG,"WiFi Protected Setup (WPS): timeout in enrollee mode");
            break;
        case SYSTEM_EVENT_STA_WPS_ER_PIN:
            ESP_LOGD(TAG,"WiFi Protected Setup (WPS): pin code in enrollee mode");
            break;
        case SYSTEM_EVENT_AP_START:
            ESP_LOGD(TAG,"AP WiFi access point started");
            break;
        case SYSTEM_EVENT_AP_STOP:
            ESP_LOGD(TAG,"AP WiFi access point  stopped");
            break;
        case SYSTEM_EVENT_AP_STACONNECTED:
            ESP_LOGD(TAG,"AP Client connected");
            break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            ESP_LOGD(TAG,"AP Client disconnected");
            break;
        case SYSTEM_EVENT_AP_STAIPASSIGNED:
            ESP_LOGD(TAG,"AP Assigned IP address to client");
            break;
        case SYSTEM_EVENT_AP_PROBEREQRECVED:
            ESP_LOGD(TAG,"AP Received probe request");
            break;
        case SYSTEM_EVENT_GOT_IP6:
            ESP_LOGD(TAG,"IPv6 is preferred");
            break;
        case SYSTEM_EVENT_ETH_START:
            ESP_LOGD(TAG,"Ethernet started");
            break;
        case SYSTEM_EVENT_ETH_STOP:
            ESP_LOGD(TAG,"Ethernet stopped");
            break;
        case SYSTEM_EVENT_ETH_CONNECTED:
            ESP_LOGD(TAG,"Ethernet connected");
            break;
        case SYSTEM_EVENT_ETH_DISCONNECTED:
            ESP_LOGD(TAG,"Ethernet disconnected");
            break;
        case SYSTEM_EVENT_ETH_GOT_IP:
            ESP_LOGD(TAG,"Ethernet Obtained IP address");
            break;
        default: break;
    }
        
}





String _mac2String(uint8_t * bytes)
{
  String s;
  int len = sizeof(bytes);
  for (byte i = 0; i < len; ++i)
  {
    char buf[3];
    sprintf(buf, "%2X", bytes[i]);
    s += buf;
    if (i < len-1) s += ':';
  }
  return s;
}
