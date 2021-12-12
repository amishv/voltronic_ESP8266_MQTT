#include <stdio.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include "RemoteDebug.h" //https://github.com/JoaoLopesF/RemoteDebug
#include "solarmon.h"
#include "passwd.h"

ESP8266WiFiMulti wifiMulti;
WiFiClient wifiClient;
PubSubClient client;

const char *ssid = SSID;
const char *password = WiFi_KEY;
const char *ssid2 = SSID2;
const char *password2 = WiFi_KEY2;
const uint32_t connectTimeoutMs = 5000;
const char *hostname = CLIENTID;
const char *api_passwd = API_PASS ;
const char mqtt_hostname[] = MQTT_HOST; //replace this with IP address of machine
const uint8_t startingDebugLevel = debugPort.ANY;
void communication_SetRemoteDebug(RemoteDebug *Debug);
//HardwareSerial   debugPort = Serial;
//HardwareSerial   InverterPort = Serial1;
//PubSub call back crashes on calling external/library functions
static char mqttCmdBuf[SIZE_OF_REPLY]; // avoiding dynamic allocation in the ESP processor
static char mqttRecBuf[SIZE_OF_REPLY]; // avoiding malloc/calloc in the ESP processor
static int isMqttCmd = false;

void callback(const char *topic, byte *payload, unsigned int length)
{
  if (!isMqttCmd)
  {
    debugPort.println("Message Arrived on");
    debugPort.println(topic);

    debugPort.print("Message:");
    unsigned int i;
    for (i = 0; i < length; i++)
    {
      debugPort.print((char)payload[i]);
      mqttCmdBuf[i] = (char)payload[i];
    }
    // mqttCmdBuf[i] = '\r';
    isMqttCmd = true; // set flag for further processing of signal
    debugPort.println();
    debugPort.println("-----------------------");
  }
}
void setupOTA()
{
  ArduinoOTA.onStart([]()
                     {
                       String type;
                       if (ArduinoOTA.getCommand() == U_FLASH)
                       {
                         type = "sketch";
                       }
                       else
                       { // U_FS
                         type = "filesystem";
                       }

                       // NOTE: if updating FS this would be the place to unmount FS using FS.end()
                       debugPort.println("Start updating " + type);
                     });
  ArduinoOTA.onEnd([]()
                   { debugPort.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { debugPort.printf("Progress: %u%%\r", (progress / (total / 100))); });
  ArduinoOTA.onError([](ota_error_t error)
                     {
                       debugPort.printf("Error[%u]: ", error);
                       if (error == OTA_AUTH_ERROR)
                       {
                         debugPort.println("Auth Failed");
                       }
                       else if (error == OTA_BEGIN_ERROR)
                       {
                         debugPort.println("Begin Failed");
                       }
                       else if (error == OTA_CONNECT_ERROR)
                       {
                         debugPort.println("Connect Failed");
                       }
                       else if (error == OTA_RECEIVE_ERROR)
                       {
                         debugPort.println("Receive Failed");
                       }
                       else if (error == OTA_END_ERROR)
                       {
                         debugPort.println("End Failed");
                       }
                     });
  ArduinoOTA.begin();
}

void setup()
{
  //debugPort.begin(9600); // For Logger, enable if using serial logging
  debugPort.println("Booting");
  InverterPort.begin(2400); // for Inverter
  // Register multi WiFi networks
  wifiMulti.addAP(ssid, password);
  wifiMulti.addAP(ssid2, password2);
  client.setClient(wifiClient);
  WiFi.mode(WIFI_STA);
 // WiFi.begin(ssid, password);
  while (wifiMulti.run(connectTimeoutMs) != WL_CONNECTED)
  {
    debugPort.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  setupOTA();
  // Hostname defaults to esp8266-[ChipID]
  WiFi.setHostname(hostname);
  //starting over wifi
  debugPort.begin(hostname, startingDebugLevel);

  debugPort.println("Ready");
  debugPort.print(hostname);
  debugPort.print("IP address: ");
  debugPort.println(WiFi.localIP());
  //implement rest of the code
  client.setServer(mqtt_hostname, 1883); //default port for mqtt is 1883
  client.setBufferSize(500);             // for large mqtt messages
  client.setCallback(callback);
  while (!client.connected())
  {
    debugPort.println("Connecting to MQTT...");

    if (client.connect(CLIENTID))
    {
      debugPort.println("connected");
    }
    else
    {

      debugPort.print("failed with state ");
      debugPort.print(client.state());
      delay(10000); //wait for watchdog reset
    }
  }
  client.publish("homeassistant/test", "Hello from ESP8266");
  if (client.subscribe(MQTT_RAW_CMD_TOPIC, 0))
    debugPort.println("Subscribed to topic");
  // required to enable WIFI remotedebug in communication.h
  communication_SetRemoteDebug(&debugPort);
  // Register the senor on HA MQTT
  sensorInit(1);
}

//this function reconnect wifi as well as broker if connection gets disconnected.
void reconnect()
{
  int status;
  while (!client.connected())
  {
    status = WiFi.status();
    if (status != WL_CONNECTED)
    {
      wifiMulti.run(connectTimeoutMs);
      while (WiFi.status() != WL_CONNECTED)
      {
        delay(500);
        debugPort.println(".");
      }
      debugPort.println("Connected to AP");
    }
    debugPort.print("Connecting to Broker …");
    debugPort.print(mqtt_hostname);

    if (client.connect(CLIENTID, NULL, NULL))
    {
      client.subscribe(MQTT_RAW_CMD_TOPIC, 0);
      debugPort.println("[DONE]");
    }
    else
    {
      debugPort.println(" : retrying in 5 seconds]");
      delay(5000);
    }
  }
}

void loop()
{
  static long time_now = millis();
  static long time_prev = 0l;
  static int time_count = 0;
  static bool polling = true;
  int status;
  reconnect();
  ArduinoOTA.handle();
  client.loop();
  if (isMqttCmd) // if raw command is available on MQTT process it first
  {
    if (sendCommand(mqttCmdBuf) > 0)
    {
      status = readport((uint8_t *)mqttRecBuf);
      if (status != TRUE)
      {
        debugPort.println("Bad Response form inverter");
        debugPort.println(status);
        debugPort.println(mqttRecBuf);
      }
      else if (status == -2) //command refused
      {
        debugPort.println(mqttRecBuf);
        debugPort.println("Message to be published to MQTT");
        debugPort.printf("(%d): %s", strlen(mqttRecBuf), mqttRecBuf);
      }
      else
      {
        debugPort.println(mqttRecBuf);
        debugPort.println("Message to be published to MQTT");
        debugPort.printf("(%d): %s", strlen(mqttRecBuf), mqttRecBuf);
      }
    }
    else
    {
      debugPort.println("Unable to send command to inverter");
    }
    client.publish(MQTT_RAW_REPLY_TOPIC, mqttRecBuf);
    isMqttCmd = false;
    memset(mqttCmdBuf, 0, SIZE_OF_REPLY);
  } //no other commands if raw cammand is present
  else if ((time_now - time_prev) > 15000l)
  {
    time_prev = time_now;
    if (polling)
    {
      generalStatusDisplay();
      sprintf(mqttCmdBuf, "%d",WiFi.RSSI()); //reuse mqtt buffer as it will ne empty in single thread
      sendMQTTmessage("wifi_strength", mqttCmdBuf);
    }
    else
    {
      getInverterStatus();
      ratedInformation();
    }
    polling = !polling;
    time_count++;
    if (time_count % 20 == 0)
    {
      ; //sensorInit(1); //configure sensors every 5 min
    }
    debugPort.printf("in the debug loop -Amish\n");
  }
  time_now = millis();
  debugPort.handle();
}