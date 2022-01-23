// Copyright (c) 2022 amish
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <strings.h>
#include <time.h>

#include "solarmon.h"
const char *inverterStatus[] = {
    "Power on mode",
    "Standby mode",
    "Bypass mode",
    "Battery mode",
    "Fault mode",
    "Hybrid mode" };

int getInverterTime(void)
{
  char tmpBuf[30], protocol[3];
  //int year, mon, day, hr, min, sec;
  if (!sendCommand("^P005PI"))
    return FALSE;
  if (!readport((uint8_t *)tmpBuf))
    return FALSE;
  debugPort.printf("%s\n", tmpBuf);
  sscanf(tmpBuf, "^D005%2s", protocol);
  if (!sendCommand("^P004T"))
    return FALSE;
  if (!readport((uint8_t *)tmpBuf))
    return FALSE;
  /*
  debugPort.printf("%s\n", tmpBuf);
  sscanf(tmpBuf,"^D017%04d%02d%02d%02d%02d%02d",&year,&mon,&day,&hr,&min,&sec);
  printf("\n\tInverter Time : %04d/%02d/%02d     %02d:%02d:%02d\t\t\t\tProtocol:%s\n",year,mon,day,hr,min,sec, protocol);
*/
  return true;
}
int getInverterStatus(void)
{  
  char tmpBuf[SIZE_OF_REPLY];
  if (sendCommand("^P006MOD") != TRUE)
    return FALSE;
  debugPort.println("sent ^P006MOD");
  if (readport((uint8_t *)tmpBuf) != TRUE)
    return FALSE;
  debugPort.printf("%s\n", tmpBuf);
  if (!strstr(tmpBuf, "^D005"))
    return FALSE;
  sendMQTTmessage( "Inverter_mode", inverterStatus[tmpBuf[6]-'0']);
  return TRUE;
}