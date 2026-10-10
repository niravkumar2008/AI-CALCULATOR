// Host stand-in: a Wi-Fi scan that always finds three networks.
#pragma once
#include <cstdint>
enum wifi_mode_t { WIFI_OFF = 0, WIFI_STA = 1, WIFI_AP = 2 };
enum wl_status_t { WL_IDLE_STATUS = 0, WL_CONNECTED = 3, WL_DISCONNECTED = 6 };
struct HostWiFi {
  bool mode(wifi_mode_t) { return true; }
  int16_t scanNetworks() { return 3; }
  int32_t RSSI(int i) { return i == 0 ? -52 : -70 - i; }
  void scanDelete() {}
  wl_status_t status() { return WL_DISCONNECTED; }
};
extern HostWiFi WiFi;
