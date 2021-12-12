
// #solarmon.h
//srarted on 5/3/2019
#ifndef SOLARMON_H__
#define SOLARMON_H__

#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include "WiFiClientSecureBearSSL.h"
#include "RemoteDebug.h"  //https://github.com/JoaoLopesF/RemoteDebug
#include <Arduino.h>


#define debugPort Debug
//#define debugPort Serial1
#define InverterPort Serial
//#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define MQTT_RAW_CMD_TOPIC    "homeassistant/raw_command"
#define MQTT_RAW_REPLY_TOPIC  "homeassistant/raw_command/reply"
#define HA_SENSOR_TOPIC       "homeassistant/sensor"

const int SIZE_OF_REPLY = 130;
static RemoteDebug Debug;
static const int enDay = 1;
static const int enMon = 2;
static const int enYear = 3;

static const int TRUE = 1;
static const int FALSE = -1;

static const char command[][10] = {
    "^P005PI",
    "^P005GS",
    "^P005ET",
    "^P009EY",
    "^P011EM",
    "^P013ED"};

uint16_t cal_crc_half(uint8_t *pin, uint8_t len);
int sendCommand(const char *cmd);
int openport(void);
int readport(uint8_t *recv_buf);
int sendMQTTmessage(const char *topic_str, const char *msg);
int sensorInit(int init_uninit);
int getInverterMode(void);
int getInverterStatus(void);
int getInverterTime(void);
int generalStatusDisplay(void);
float energyGenerated(struct tm *tm, const int type);
int ratedInformation();
//static void debugPrint(const char *format, ...){};

#endif //SOLARMON_H__