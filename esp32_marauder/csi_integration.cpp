// csi_integration.cpp - glue between ESP32 Marauder and the CSI Sense feature.
// ESP32-S2 has no Bluetooth, so CSI is streamed over the UART (to the Flipper,
// which bridges it to the phone over the Flipper's own BLE). Control frames
// (incl. WiFi credentials) arrive over the same UART.
#include "csi_integration.h"

#include <Arduino.h>
#include "csi_config.h"
#include "csi_sense.h"
#include "csi_uart.h"

static bool     s_started = false;
static uint32_t s_lastStatus = 0;

static void onCtrl(uint8_t cmd, const uint8_t* payload, uint8_t len) {
  switch (cmd) {
    case CMD_THRESHOLD:   if (len >= 1) CsiSense::setMotionThreshold(payload[0]); break;
    case CMD_MODE:        if (len >= 1) CsiSense::setMode(payload[0]);            break;
    case CMD_RECALIBRATE: CsiSense::resetBaseline();                             break;
    case CMD_CHANNEL:     if (len >= 1) CsiSense::setChannel(payload[0]);        break;
    case CMD_SET_SSID: {
      char s[33]; uint8_t n = (len < 32) ? len : 32; memcpy(s, payload, n); s[n] = 0;
      CsiSense::setCredentials(s, nullptr);
      break;
    }
    case CMD_SET_PASS: {
      char p[64]; uint8_t n = (len < 63) ? len : 63; memcpy(p, payload, n); p[n] = 0;
      CsiSense::setCredentials(nullptr, p);
      break;
    }
    case CMD_CONNECT:     CsiSense::applyCredentials();                          break;
    default: break;
  }
}

void CsiInteg::start() {
  // CSI mode owns the radio while active; normal Marauder WiFi scans are paused.
  UartLink::begin(UART_BAUD);
  CsiSense::begin(DEFAULT_MODE, DEFAULT_CHANNEL);
  s_started = true;
}

void CsiInteg::loop() {
  if (!s_started) return;
  UartLink::poll(onCtrl);

  CsiResult r;
  if (CsiSense::poll(r)) UartLink::sendCsi(r);

  uint32_t now = millis();
  if (now - s_lastStatus >= STATUS_PERIOD_MS) {
    s_lastStatus = now;
    char json[160];
    snprintf(json, sizeof(json),
             "{\"mode\":\"%s\",\"ch\":%u,\"rate\":%.1f,\"rssi\":%d,\"sub\":%u,\"cal\":%s}",
             CsiSense::mode() == 1 ? "active" : "passive",
             CsiSense::channel(), CsiSense::sampleRate(), CsiSense::lastRssi(),
             CsiSense::subcarriers(), CsiSense::calibrated() ? "true" : "false");
    UartLink::sendStatus(json);
  }
}

void CsiInteg::stop() { s_started = false; }
