#pragma once

#include <HTTPClient.h>
#include <WiFiClientSecure.h>

class FirmwareUpdates {
    public:
    FirmwareUpdates();

    /**
     * Checks for firmware updates
     * 
     * Fetches release metadata from github and compares
     * the latest remote version to the local version
     * 
     * Returns true if there were no errors, false on error
     * 
     * will set the `updateAvailable` flag if there is anything new
     * and the (private) `downloadURL` variable
     */

    bool checkForUpdates();

    /**
     * Starts the update process
     * ... run checkForUpdate before calling this
     * on sucessful completion, the system will be rebooted
     * OTHERWISE it will return false
     */

    bool startUpdate();

    /**
     * indicates whether an update is available after calling `checkForUpdates()`
     */

    bool updateAvailable;


    private:

    bool _processOTAUpdate(const char* url);
    char _downloadURL[2048];
    bool _getWithRedirects(HTTPClient ** httpsptr, WiFiClientSecure ** clientptr, const char* url, int depth = 0);

};