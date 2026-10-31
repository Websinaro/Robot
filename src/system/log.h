#pragma once
// Minimal logging. NEVER log tokens, credentials, Wi-Fi passwords or raw user speech.
#ifdef UNIT_TEST
  #include <stdio.h>
  #define WLOG(level, tag, fmt, ...) printf("[" level "][" tag "] " fmt "\n", ##__VA_ARGS__)
#else
  #include <Arduino.h>
  #define WLOG(level, tag, fmt, ...) \
    Serial.printf("[%lu][" level "][" tag "] " fmt "\n", (unsigned long)millis(), ##__VA_ARGS__)
#endif
#define LOGI(tag, fmt, ...) WLOG("I", tag, fmt, ##__VA_ARGS__)
#define LOGW(tag, fmt, ...) WLOG("W", tag, fmt, ##__VA_ARGS__)
#define LOGE(tag, fmt, ...) WLOG("E", tag, fmt, ##__VA_ARGS__)
