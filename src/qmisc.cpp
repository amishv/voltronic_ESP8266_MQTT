
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
long energyDay(struct tm *tm)
{
  char tmpBuf[20];
  int energyGen;
  sprintf(tmpBuf, "^P013ED20%02d%02d%02d", tm->tm_year - 100, tm->tm_mon + 1, tm->tm_mday);
  if (sendCommand(tmpBuf) != TRUE)
    return FALSE;
  bzero(tmpBuf, sizeof(tmpBuf));
  if (readport((uint8_t *)tmpBuf) != TRUE)
    return FALSE;
  sscanf(tmpBuf, "^D011%08d", &energyGen);
  debugPort.printf("Energy Response %s\n return %08d", tmpBuf, energyGen);
  return energyGen;
}
long energyMonth(struct tm *tm)
{
  char tmpBuf[20];
  int energyGen;
  sprintf(tmpBuf, "^P011EM20%02d%02d", tm->tm_year - 100, tm->tm_mon + 1);
  if (sendCommand(tmpBuf) != TRUE)
    return FALSE;
  bzero(tmpBuf, sizeof(tmpBuf));
  if (readport((uint8_t *)tmpBuf) != TRUE)
    return FALSE;
  debugPort.printf("Energy Response %s\n", tmpBuf);
  sscanf(tmpBuf, "^D011%d", &energyGen);
  return energyGen;
}

float energyGenerated(struct tm *tm, int type)
{
  char tmpBuf[20];
  int energyGen;
  if (type == enDay)
    sprintf(tmpBuf, "^P013ED%04d%02d%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
  else if (type == enMon)
    sprintf(tmpBuf, "^P011EM%04d%02d", tm->tm_year + 1900, tm->tm_mon + 1);
  else if (type == enYear)
    sprintf(tmpBuf, "^P009EY%04d", tm->tm_year + 1900);
  else
    return (FALSE);
  if (sendCommand(tmpBuf) != TRUE)
    return FALSE;
  bzero(tmpBuf, sizeof(tmpBuf));
  if (readport((uint8_t *)tmpBuf) != TRUE)
    return FALSE;
  sscanf(tmpBuf, "^D011%08d", &energyGen);
  debugPort.printf("Energy Response %s\n return %08d", tmpBuf, energyGen);
  return (float)energyGen / 1000;
}