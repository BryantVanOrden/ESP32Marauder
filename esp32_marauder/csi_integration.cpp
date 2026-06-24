// csi_integration.cpp - see csi_integration.h
#include "csi_integration.h"

#include <Arduino.h>
#include <WiFi.h>
#include "csi_config.h"
#include "csi_sense.h"
#include "csi_ble_link.h"
#include "csi_ota.h"

static bool     s_started = false;
static uint32_t s_lastStatus = 0;

// BLE control commands -> sensing engine (see PROTOCOL.md).
static void onCtrl(uint8_t cmd, uint8_t arg, bool hasArg) {
  switch (cmd) {
    case 0x01: if (hasArg) CsiSense::setMotionThreshold(arg); break;
    case 0x02: if (hasArg) CsiSense::setMode(arg);            break;
    case 0x03: CsiSense::resetBaseline();                     break;
    case 0x04: if (hasArg) CsiSense::setChannel(arg);         break;
    default: break;
  }
}

void CsiInteg::start() {
  // NOTE: CSI mode takes over the radio. While it runs, normal Marauder WiFi/BLE
  // activity is paused (this is a separate, exclusive scan mode).
  BleLink::begin(onCtrl);                       // guarded: only inits NimBLE if not already
  CsiSense::begin(DEFAULT_MODE, DEFAULT_CHANNEL);
  s_started = true;
}

void CsiInteg::loop() {
  if (!s_started) return;

  if (!Ota::running() && WiFi.status() == WL_CONNECTED) Ota::begin();
  Ota::loop();

  CsiResult r;
  if (CsiSense::poll(r)) BleLink::notifyCsi(r);

  uint32_t now = millis();
  if (now - s_lastStatus >= STATUS_PERIOD_MS) {
    s_lastStatus = now;
    char json[192];
    snprintf(json, sizeof(json),
             "{\"mode\":\"%s\",\"ch\":%u,\"rate\":%.1f,\"rssi\":%d,\"sub\":%u,\"cal\":%s,"
             "\"ip\":\"%s\",\"ota\":%s}",
             CsiSense::mode() == 1 ? "active" : "passive",
             CsiSense::channel(), CsiSense::sampleRate(), CsiSense::lastRssi(),
             CsiSense::subcarriers(), CsiSense::calibrated() ? "true" : "false",
             Ota::ip().c_str(), Ota::running() ? "true" : "false");
    BleLink::updateStatus(json);
  }
}

void CsiInteg::stop() { s_started = false; }
