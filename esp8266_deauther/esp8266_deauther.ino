/*
   ===========================================
      Copyright (c) 2018 Stefan Kremser
             github.com/spacehuhn
   ===========================================
 */

extern "C" {
  // Please follow this tutorial:
  // https://github.com/spacehuhn/esp8266_deauther/wiki/Installation#compiling-using-arduino-ide
  // And be sure to have the right board selected
  #include "user_interface.h"
}
#include <EEPROM.h>

#include <ArduinoJson.h>
#if ARDUINOJSON_VERSION_MAJOR != 5
// The software was build using ArduinoJson v5.x
// version 6 is still in beta at the time of writing
// go to tools -> manage libraries, search for ArduinoJSON and install the latest version 5
#error Please upgrade/downgrade ArduinoJSON library to version 5!
#endif

#include "oui.h"
#include "language.h"
#include "functions.h"
#include "Settings.h"
#include "Names.h"
#include "SSIDs.h"
#include "Scan.h"
#include "Attack.h"
#include "CLI.h"
#include "A_config.h"

#include "LED.h"

// Run-Time Variables //
LED led;
Settings settings;
Names    names;
SSIDs    ssids;
Accesspoints accesspoints;
Stations     stations;
Scan   scan;
Attack attack;
CLI    cli;

uint32_t autosaveTime = 0;
uint32_t currentTime  = 0;

bool booted = false;

void setup() {
    // for random generator
    randomSeed(os_random());

    // start serial
    Serial.begin(115200);
    Serial.println();

    // start SPIFFS
    prnt(SETUP_MOUNT_SPIFFS);
    prntln(SPIFFS.begin() ? SETUP_OK : SETUP_ERROR);

    // Start EEPROM
    EEPROM.begin(4096);

    // auto repair when in boot-loop
    uint8_t bootCounter = EEPROM.read(0);

    if (bootCounter >= 3) {
        prnt(SETUP_FORMAT_SPIFFS);
        SPIFFS.format();
        prntln(SETUP_OK);
    } else {
        EEPROM.write(0, bootCounter + 1); // add 1 to the boot counter
        EEPROM.commit();
    }

    // get time
    currentTime = millis();

    // load settings
    settings.load();

    // load everything else
    names.load();
    ssids.load();
    cli.load();

    // create scan.json
    scan.setup();

    // set channel
    setWifiChannel(settings.getChannel());

    // dis/enable serial command interface
    if (settings.getCLI()) {
        cli.enable();
    } else {
        prntln(SETUP_SERIAL_WARNING);
        Serial.flush();
        Serial.end();
    }

    // STARTED
    prntln(SETUP_STARTED);

    // version
    prntln(settings.getVersion());

    // setup LED
    led.setup();
}

void loop() {
    currentTime = millis();

    led.update();    // update LED color
    attack.update(); // run attacks
    cli.update();    // read and run serial input
    scan.update();   // run scan
    ssids.update();  // run random mode, if enabled

    // auto-save
    if (settings.getAutosave() && (currentTime - autosaveTime > settings.getAutosaveTime())) {
        autosaveTime = currentTime;
        names.save(false);
        ssids.save(false);
        settings.save(false);
    }

    if (!booted) {
        // reset boot counter
        EEPROM.write(0, 0);
        EEPROM.commit();
        booted = true;
#ifdef HIGHLIGHT_LED
#endif // ifdef HIGHLIGHT_LED
    }
}
