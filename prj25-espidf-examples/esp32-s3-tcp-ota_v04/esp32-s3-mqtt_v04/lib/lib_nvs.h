// lib_nvs.h
// Version: Ver.1.0
// Author: mdex.co.kr
// Date: 2025-07-21

#ifndef _LIB_NVS_H_
#define _LIB_NVS_H_

#include <Preferences.h>
#include "libPrintRaw.h"

// --- NVS 키 상수 정의 ---
const char* NVS_NAMESPACE = "wifi_config";
const char* NVS_KEY_SSID = "ssid";
const char* NVS_KEY_PASS = "password";
const char* NVS_KEY_APPID = "app_id";
const char* NVS_KEY_TCP_IP = "tcp_ip";
const char* NVS_KEY_BOARD_ID = "board_id";

// --- 기본값 상수 정의 ---
const char* DEFAULT_SSID = "KT_GiGA_2G_Wave2_4587";
const char* DEFAULT_PASSWORD = "fab8ezx455";
const char* DEFAULT_APPID = "banana7";

// --- 변수 정의 ---
Preferences prefs;

// --- 함수 구현 ---
void saveTcpIpToNVS(const String& ip) {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putString(NVS_KEY_TCP_IP, ip);
    prefs.end();
    uart0_printf("[%8lu ms] [NVS] Saved TCP Server IP: %s\n", millis(), ip.c_str());
}

String loadTcpIpFromNVS() {
    prefs.begin(NVS_NAMESPACE, true);
    String ip = prefs.getString(NVS_KEY_TCP_IP, "");
    prefs.end();
    if (0 < ip.length()) {
        uart0_printf("[%8lu ms] [NVS] Loaded TCP Server IP: %s\n", millis(), ip.c_str());
    } else {
        uart0_printf("[%8lu ms] [NVS] No TCP Server IP found in NVS.\n", millis());
    }
    return ip;
}

void saveBoardIdToNVS(int id) {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putInt(NVS_KEY_BOARD_ID, id);
    prefs.end();
    uart0_printf("[%8lu ms] [NVS] Saved Board ID: %d\n", millis(), id);
}

int loadBoardIdFromNVS(int defaultId) {
    prefs.begin(NVS_NAMESPACE, true);
    int id = prefs.getInt(NVS_KEY_BOARD_ID, defaultId);
    prefs.end();
    uart0_printf("[%8lu ms] [NVS] Loaded Board ID: %d\n", millis(), id);
    return id;
}

#endif // _LIB_NVS_H_
