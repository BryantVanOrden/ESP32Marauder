// csi_integration.h - glue between ESP32 Marauder and the CSI Sense feature.
// Marauder calls start() when the "WiFi CSI Sense" menu item is selected, and
// loop() each iteration while that scan mode is active.
#pragma once

namespace CsiInteg {
  void start();   // bring up CSI capture + BLE (+ OTA when WiFi associates)
  void loop();    // service CSI -> BLE notify + status + OTA
  void stop();
}
