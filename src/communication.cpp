#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "RemoteDebug.h" //https://github.com/JoaoLopesF/RemoteDebug
#include "solarmon.h"

extern WiFiClient wifiClient;
extern PubSubClient client;
const long TIMEOUT = 10000L;
int debugFlag = 0;
static RemoteDebug *commDebug; // static -> private - for only this file

void communication_SetRemoteDebug(RemoteDebug *Debug) {
commDebug = Debug;
}
int readport(uint8_t *recv_buf)
{
  uint8_t inByte;
  int  byteCnt = 0;
  long time_now = millis();
  long time_prev = time_now;
  memset(recv_buf, 0, SIZE_OF_REPLY);
  while ((InverterPort.available() == 0)&&((time_now - time_prev) < 5000l)){
    //wait for one second for inverter to respond
    time_now = millis();
  };
  commDebug->printf("Reading from Port \n");
  do
  {
    InverterPort.readBytes(&inByte,1);
    //if (InverterPort.readBytes(&inByte,1) = 1) // read byte by byte: Serial.read returns int :(
    //commDebug->printf("readport: %03d: %02X >>> (%c)\n",byteCnt, inByte, inByte);
    recv_buf[byteCnt++] = inByte;
  } while ( (inByte != '\n') && (inByte != '\r') && (byteCnt < 150));

  if ((recv_buf[0] == '^') && (recv_buf[1] == '0'))
  {
    commDebug->println("Command Refused \n");
    strcpy((char *) recv_buf, "Command Refused");
    return (-2);
  }
  if ((recv_buf[0] != '^') || (recv_buf[1] != 'D'))
  {
    commDebug->printf("Response corrupted: %s\n", recv_buf);
    strcpy((char *) recv_buf, "Response corrupted ");
    return (-1);
  }
  //commDebug->printf("%d --->>%s\n", n, recv_buf);
  uint16_t recCRC = (recv_buf[byteCnt-3] << 8) + recv_buf[byteCnt-2] ;
  uint32_t caclCRC = cal_crc_half(recv_buf, byteCnt-3);
  if (caclCRC != recCRC) // TODO: this work around as CRC is not addind up for some commands
  {
    commDebug->printf(" CRC (%04X) Failed in Received message len %d %02X%02X :::: %04X\n",
                              caclCRC,byteCnt, recv_buf[byteCnt-3], recv_buf[byteCnt-2], recCRC );
    commDebug->printf("CRC : %s\n", recv_buf); //return FALSE;
  }
  recv_buf[byteCnt-3]= '\0'; //terminate the string before CRC
  return TRUE;
}
// Sending byte by byte as Inverter DSP does not like stream
int sendport(uint8_t *tmpbuffer)
{
  int n, j, i = 0;
  int status = FALSE;
  int len = strlen((char *)tmpbuffer);
  uint8_t t;
  commDebug->printf("enter write  %s  with len %d\n", tmpbuffer, len);

  do // send one char at a time for slow CPU on inverter
  {
    t = tmpbuffer[i++];
    n = InverterPort.write(&t, 1);
    for (j = 0; j < 300; j++)
      ;                                   //need to adjust for best response
  } while ((t != '\r') && (i < len + 1)); //
  if (n < 0){
    commDebug->printf("writeof (%d) bytes failed!\n", len + 1);
    commDebug->println("write failed");
    }
  else
  {
    status = TRUE;
    commDebug->printf(" %d Bytes Sent successfully to  staus %d\n", i, n);
    commDebug->println("write completed");
  }
  //InverterPort.flush(); // clean any remaining data and wait for transmission to complete
  return status;
}
int sendCommand(const char *cmd)
{
  uint8_t sendStr[30];
  uint8_t cmdLen = strlen((char *)cmd);
  uint16_t cmdCrc = cal_crc_half((uint8_t *)cmd, cmdLen);
  while (InverterPort.available()>0)
    InverterPort.read(); //arduino has no defined method to clear incomming buffer
                         // So manually clean it 
  memset(sendStr, 0, 30);
  sprintf((char *)sendStr, "%s%c%c\r", cmd, (uint8_t)(cmdCrc >> 8), (uint8_t)(cmdCrc & 0xFF));
  commDebug->printf("sendCommand: %02u bytes %s to send---CRC %04X\n", strlen((char *)sendStr), sendStr, cmdCrc);
  return (sendport(sendStr));
}
uint16_t cal_crc_half(uint8_t *pin, uint8_t len)
{
  uint16_t crc;
  uint8_t da;
  uint8_t *ptr;
  uint8_t bCRCHign;
  uint8_t bCRCLow;
  uint16_t crc_ta[16] =
      {
          0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
          0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef};
  ptr = pin;
  crc = 0;
  while (len-- != 0)
  {
    da = ((uint8_t)(crc >> 8)) >> 4;
    crc <<= 4;
    crc ^= crc_ta[da ^ (*ptr >> 4)];
    da = ((uint8_t)(crc >> 8)) >> 4;
    crc <<= 4;
    crc ^= crc_ta[da ^ (*ptr & 0x0f)];
    ptr++;
  }
  bCRCLow = crc;
  bCRCHign = (uint8_t)(crc >> 8);
  if (bCRCLow == 0x28 || bCRCLow == 0x0d || bCRCLow == 0x0a)
  {
    bCRCLow++;
  }
  if (bCRCHign == 0x28 || bCRCHign == 0x0d || bCRCHign == 0x0a)
  {
    bCRCHign++;
  }
  crc = ((uint16_t)bCRCHign) << 8;
  crc += bCRCLow;
  return (crc);
}
int sendMQTTmessage(const char *topic_str, const char *msg)
{
  int mqttStatus = 5;
  char topic[100];
  sprintf(topic, "%s/%s/state",HA_SENSOR_TOPIC, topic_str);
  //int qos = 1;
  //int retained = 0;
  mqttStatus = client.publish(topic, msg);
  if (mqttStatus == false)
  {
    commDebug->printf("Failed to publish message \'%s\':\'%s\', return code %d\n", topic, msg, mqttStatus);
    //perror ("sendMQTT");
  }
  return mqttStatus;
}
/*
    Registering in Hass https://www.home-assistant.io/docs/mqtt/discovery/

    Configuration Parameters:
     
    topic: homeassistant/sensor/voltronics_<topic_str>/config
    Payload: 
    {
      "name": "<toppic_str>", 
      "unit_of_measurement": "<unit_name>",
      "unique_id": "toppic_str",
      "state_topic": "homeassistant/sensor/<toppic_str>/state",
      "icon": "mdi:<icon_name>",
      "retain": true
    }
*/
int registerTopic(const char *topic_str, const char *unit_name, const char *icon_name, int isStop)
{
  int mqttStatus;
  char topic[100];
  char message[500];
  sprintf(topic, "%s/voltronic_%s/config",HA_SENSOR_TOPIC, topic_str);
  if (isStop == 1)
    sprintf(message, "{ \n\
                        \"name\": \"%s/%s\",\n\
                        \"unit_of_measurement\": \"%s\",\n\
                        \"unique_id\": \"%s\",\n\
                        \"state_topic\": \"%s/state\",\n\
                        \"icon\": \"mdi:%s\",\n\
                        \"retain: false\n\
                      }\n",
            HA_SENSOR_TOPIC,topic_str, unit_name, topic_str, topic_str,icon_name);
  else
    sprintf(message, " ");
  //commDebug->printf("topic: %s\n", topic);
  //commDebug->printf("message: %s\n", message);
  //int qos = 1;
  int retained = 0;
  //int msglen = strlen(message);
  mqttStatus = client.publish(topic, message, retained);
  if (!mqttStatus)
  {
    commDebug->printf("%d Failed to publish message \'%s\': \'%s\', return code %d\n", strlen(message), topic, message, mqttStatus);
    //perror ("sendMQTT");
  }
  return mqttStatus;
}

int sensorInit(int isStop)
{
  int status = FALSE;
  status = registerTopic(  "Inverter_mode", "", "solar-power", isStop);
  status = registerTopic(  "AC_grid_voltage","V","power-plug", isStop);
  status = registerTopic(  "AC_grid_frequency", "Hz", "current-ac", isStop);
  status = registerTopic(  "AC_out_voltage","V","power-plug", isStop);
  status = registerTopic(  "AC_out_frequency","Hz","current-ac", isStop);
  status = registerTopic(  "PV_in_voltage","V","solar-panel-large", isStop);
  status = registerTopic(  "PV_in_power","W","solar-panel-large", isStop);
  status = registerTopic(  "SCC_voltage","V","current-dc", isStop);
  status = registerTopic(  "Load_pct","%","brightness-percent", isStop);
  status = registerTopic(  "Load_watt","W","chart-bell-curve", isStop);
  status = registerTopic(  "Load_va","VA","chart-bell-curve", isStop);
  status = registerTopic(  "Heatsink_temperature","C","details", isStop);
  status = registerTopic(  "MPPT_temperature","C","details", isStop);
  status = registerTopic(  "Battery_capacity","%","battery-outline", isStop);
  status = registerTopic(  "Battery_voltage","V","battery-outline", isStop);
  status = registerTopic(  "Battery_charge_current","A","current-dc", isStop);
  status = registerTopic(  "Battery_discharge_current","A","current-dc", isStop);
  status = registerTopic(  "Load_status_on","","power", isStop);
  status = registerTopic(  "AC_Power_dir","","power", isStop);
  status = registerTopic(  "DC_AC_dir","","power", isStop);
  status = registerTopic(  "SCC_charge_on","","power", isStop);
  /*
  status = registerTopic(  "AC_charge_on","","power", isStop);
  status = registerTopic(  "Battery_recharge_voltage","V","current-dc", isStop);
  status = registerTopic(  "Battery_under_voltage","V","current-dc", isStop);
  status = registerTopic(  "Battery_bulk_voltage","V","current-dc", isStop);
  status = registerTopic(  "Battery_float_voltage","V","current-dc", isStop);
  status = registerTopic(  "Max_grid_charge_current","A","current-ac", isStop);
  status = registerTopic(  "Max_charge_current","A","current-ac", isStop);
  */
  status = registerTopic(  "Out_source_priority","","grid",isStop);
  status = registerTopic(  "Charger_source_priority","","solar-power", isStop);
  status = registerTopic(  "wifi_strength","dBM","wifi", isStop);
  status = registerTopic(  "raw_command","","", isStop);
  return status;
}