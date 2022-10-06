#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#define DEVICE_ID "01A03"
#define BTN D5
#define LIGHT A0
#define LIGHT_LED D6
#define POWER_LED D4
#define IN1 D7
#define IN2 D2
#define IN3 D8
#define IN4 D1

const char *SSID = "mqtt";
const char *PASSPHRASE = "12345678";

const char *BROKER = "81.71.138.9";
const int PORT = 1883;
const char *USER = "admin";
const char *PASSWORD = "public";
const char *SUB_TOPIC = "feeding-times";
const char *PUB_TOPIC = "feeding-res";
const char *ERROR_TOPIC = "error-occur";

#endif
